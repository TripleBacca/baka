# 15. Access modifiers

`public`, `protected` and `private` control which code may name a class member. Access
control is purely a **semantic-analysis** feature: it rejects programs, but a program
that passes produces exactly the same IR and assembly as it would without the labels.

Status: **implemented**.

## Using it

```cpp
class Account {
    int balance;                 // private: class members default to private
    int fee() { return 1; }

public:
    Account(int initial) { balance = initial; }
    void deposit(int amount) { balance = balance + amount - fee(); }
    int get() { return balance; }
};

class Shape {
protected:
    int sides;                   // usable by derived classes
public:
    Shape() { sides = 0; }
};

class Square : public Shape {    // public inheritance
public:
    Square() { sides = 4; }      // OK: protected member, own class
};

struct Pair {                    // struct members default to public
    int first;
private:
    int hidden;
};

int main() {
    Account a = Account(10);
    a.deposit(5);                // OK: public
    // a.balance = 0;            // error: 'balance' is private within this context
    return a.get();
}
```

### Rules

| Rule | Behaviour |
|---|---|
| Labels | `public:`, `protected:`, `private:` may appear any number of times in a `class`, `struct` or `union` body; each applies to the members after it. |
| Defaults | `class` members are private; `struct` and `union` members are public. The class-key of the **definition** decides. |
| Base specifier | `class D : public B`, `: protected B`, `: private B`. Without a keyword: private for `class`, public for `struct`. |
| Base class-key | A `class` may only derive from a `class` and a `struct` only from a `struct` (`a class cannot inherit from a struct`). Classes are polymorphic with a `vptr` at offset 0 (page 18); structs keep a plain C layout. |
| Inherited access | A member's access in a derived class is the stricter of its declared access and the inheritance access. A **private** member of a base is inaccessible in every derived class. |
| Who may use what | **public**: anyone. **private**: member functions, constructors and destructors of the class that declares it. **protected**: that class, plus members of derived classes, but only through an object of their own class (or a class derived from it). |
| Through an inaccessible base | If `B` is a private base of `D`, outside code cannot reach `B`'s members through a `D`, even public ones. |

### What is checked

* member access: `obj.m`, `ptr->m`;
* unqualified member names inside member functions, including inherited ones;
* method calls, checked on the overload actually selected;
* constructors: `T(args)`, `new(p, args)`, default construction of a declared object
  (`T t;`) and converting constructors chosen for arguments.

### Diagnostics

```
'balance' is private within this context
'p' is protected within this context
'x' is inaccessible within this context       (public in the base, hidden by private inheritance)
constructor of 'C' is private within this context
expected ':' after access specifier            (parse error)
```

### Not covered

* No `friend` declarations and no `using` re-exposure.
* Destructor access is not checked.
* Derived-to-base pointer conversions through a non-public base are not checked.
* Nested typedef members are only visible inside the class body, so they cannot be
  named from outside in the first place.

## Design decisions

| Decision | Why |
|---|---|
| Follow C++ defaults (`class` → private) | Matching the language people know matters more than keeping old code compiling. 25 existing fixtures that relied on public-by-default `class` members were updated to say `public:` (and `: public Base`), keeping their meaning. |
| Make `public`/`private`/`protected` real keywords | They are reserved in C++. Five lex golden files changed only because these three words now lex as keywords. |
| Record access per member, resolve the default in the semantic layer | The parser stores `Unspecified` when no label has been seen. The semantic layer applies the class/struct default based on the definition's class-key, which is only known there for forward-declared types. |
| Check against the *context class* | One rule serves every use site: the class whose member function contains the expression (none at namespace scope). |
| Walk the inheritance path (C++ `[class.access.base]`) | A member named through a derived class is accessible if its effective access at some level permits the context **and** every base step down to that level is itself accessible. The first version only checked levels and wrongly let outside code reach public members of a private base. |
| Check calls after overload resolution | C++ chooses the overload first and then checks access, so an inaccessible overload can still win and be reported. Each call candidate records the class it was named through (`naming`), checked once a candidate is selected. |
| Deduplicate identical diagnostics | Some expressions are typed twice (declaration-time evaluation and annotation). `Symbols::diagnose` now drops an exact repeat at the same location. |
| No IR change | Access is a compile-time permission; generated code is unaffected. |

## How it is implemented

| Stage | File | Change |
|---|---|---|
| Lexer | `src/types/token/token.hh`, `pretty.hh`, `src/lexer/keywords.hh` | `K_PUBLIC`, `K_PROTECTED`, `K_PRIVATE` |
| AST | `src/types/parser/ast/structBody.hh` | `AccessSpecifier` enum; `StructBodyNode` keeps a `MemberAccess` vector parallel to `Members` and a current label |
| AST | `struct.hh`, `class.hh` | `BaseAccess` on definitions; printed only when written, so old parse goldens are unchanged |
| Parser | `src/parser/structBody.cc` | `MatchAccessSpecifier()`; a label followed by `:` updates the current access |
| Parser | `src/parser/struct.cc`, `class.cc` | optional access keyword after `:` in the base specifier |
| Types | `src/semantic/types.hh/.cc` | `semantic::Access`; `Member::access`; `Type::baseAccess` set by `setBase` |
| Registration | `src/semantic/aggregates.cc` | `Resolve(written, kind)` turns the written label into an access, defaulting by class-key |
| Rule | `src/semantic/declarations.cc` | `accessible(naming, owner, index, context)` and `checkAccess(…)` (the diagnostic). `memberAccess` takes an optional context and checks single members. |
| Use sites | `members.cc`, `primary.cc`, `calls.cc`, `initializers.cc` | `.`/`->`, unqualified names, `checkSelectedAccess` after overload selection (methods, constructors, `new`, converting constructors), `checkDefaultConstructionAccess` for `T t;` |

The core algorithm, in `Declarations::accessible`:

1. Build the path from the naming class down to the class that declares the member.
2. Compute the member's effective access at each level, outward from the declaring
   class: declared access there; then `max(previous, baseAccess)` per step, or
   inaccessible once a private level is crossed.
3. Walk from the naming class inward. Accept if the access at that level permits the
   context class; otherwise stop as soon as the next base step is not accessible
   from the context.

## Tests

* `custom_tests/semantic/valid_access_modifiers.cpp` and seven `invalid_access_*.cpp`
  fixtures (private field/method/constructor, protected outside, private base member,
  private inheritance, protected through another object), listed in `cases.json`.
* `custom_tests/ir/26_access_modifiers.cpp`: private state with public accessors,
  protected members from a derived class, private inheritance used internally, struct
  defaults. It runs in the IR interpreter and, through the codegen suite, under QEMU
  with and without optimization.
* `custom_tests/parse/41_cpp_features.cpp` pins the printed AST.
