# 14. Function overloading

How baka lets several functions share a name, and how a call is matched to exactly one of
them. Overloading is resolved entirely during **semantic analysis**. Once the right
declaration is chosen, every later stage just sees an ordinary call to one specific
function.

## Running example

```cpp
int    area(int s)        { return s * s; }        // (A)
int    area(int w, int h) { return w * h; }        // (B)
double area(double r)     { return 3.0 * r * r; }  // (C)

int main() {
    int a    = area(3);      // → (A)
    int b    = area(2, 5);   // → (B)
    double c = area(1.5);    // → (C)
    return a + b + (int)c;
}
```

The IR shows each call bound to a different function:

```
function @area.19: fn(i32)->i32
function @area.22: fn(i32, i32)->i32
function @area.24: fn(f64)->f64
  (call, %t1:i32, @area.19, _)
  (call, %t2:i32, @area.22, _)
  (call, %t3:f64, @area.24, _)
```

The rest of this page follows that program through the compiler.

---

## Step 1: declaring overloads (registration phase)

The symbol table stores, for each scope, a hash map from a **name to a list of symbol
IDs** (`name → [SymbolId, …]`). An overload set is simply that list holding more than one
function symbol.

When a function declaration is registered, the symbol table checks the existing entries
with the same name **in the same scope**:

| Situation | Result |
|---|---|
| No existing function with that name | New symbol; the list has one entry |
| Existing function with **different parameter types** (or a different variadic flag) | A new overload: a **separate symbol** appended to the list |
| Existing function with the **same parameter types** | A redeclaration of the same function: it must agree on return type, linkage and `static`, and only one of them may have a body. The declaration is recorded on the existing symbol. |
| Same parameters, different return type | Error: return types cannot distinguish overloads |
| A built-in (`printf`, `atoi`, …) redeclared with a different signature | Error: built-ins can't be overloaded |

Because parameter types are interned (see page 13), "different parameter types" is just
a comparison of two lists of integer type IDs.

For the example, the global scope ends up with `"area" → [19, 22, 24]`. Each symbol keeps
its own function type, `(int)->int`, `(int,int)->int` and `(double)->double`, plus its
default arguments and its body.

Function signatures are collected **before** any function body is examined, so a call
can see every overload in its scope, including ones defined later in the file.

## Step 2: name lookup at the call site

For `area(1.5)`, the checker first works out what the callee name `area` can refer to:

1. **Lookup** walks outward from the current scope, so block scopes come before function
   scope, which comes before global scope.
2. It **stops at the first scope that has the name** and returns *all* the matching
   symbols in that scope. So an inner declaration of `area` (a local variable, say) hides
   all the outer overloads; it does not merge with them.
3. Each function symbol becomes a **candidate**, which records:
   - its function type;
   - its symbol ID;
   - **how many arguments it requires.** That is its parameter count minus any trailing
     parameters with default arguments, counting only defaults declared before this call.
   - for methods, the owning class and member index.

Other callee forms produce candidates the same way:

| Callee | Candidates |
|---|---|
| `obj.m(...)` / `p->m(...)` | every method named `m` found by member lookup in the class and its bases |
| unqualified `m(...)` inside a method | members of the current class, but instance methods are rejected in a static method |
| `fp(...)` where `fp` is a function pointer | one candidate: the pointer's function type, with no defaults |
| `T(...)` where `T` is a class | the class's constructors (same algorithm, see below) |

A plain identifier that names more than one function **can't be used as a value** (for
example, `int (*fp)(int) = area;`). Outside a call it produces "overload set requires call
resolution".

## Step 3: choosing the overload (`selectOverload`)

The algorithm is deliberately simple. C++ ranks conversions; baka does not.

```
candidates = all candidates whose arity fits the argument count
             (required ≤ #args ≤ #params, or #args ≥ required for variadic)
             with duplicates removed (the same function reached twice)

exact = candidates where every argument's type == parameter type,
        ignoring top-level const

if exact has 1 element   → choose it
if exact has >1 elements → error: ambiguous

viable = candidates where every argument implicitly converts to its parameter
         (arithmetic conversions, null → pointer, adding const, derived* → base*,
          or one converting constructor for a class parameter);
         extra variadic arguments must be scalars

if viable is empty   → error: no viable overload for this call
if viable has 1      → choose it
if viable has >1     → error: ambiguous
```

### The example, call by call

| Call | Argument types | Arity filter | Exact matches | Result |
|---|---|---|---|---|
| `area(3)` | `int` | A, C (B needs 2) | A | **A** |
| `area(2, 5)` | `int, int` | B | B | **B** |
| `area(1.5)` | `double` | A, C | C | **C** |

### Where this differs from C++

Because there is no "best conversion" ranking, any call that needs a conversion succeeds
only if **exactly one** overload is reachable by conversion:

```cpp
int    area(int s);
double area(double r);
char ch = 2;
area(ch);      // baka: "ambiguous overload resolution"
               // C++:  picks area(int) (integral promotion beats conversion)

int f(int x);
int f(long x);
short s = 1;
f(s);          // baka: ambiguous;  C++: picks f(int)
```

The fix in baka is to make one candidate exact, for example `area((int)ch)`.

Other rules:
- **Default arguments:** a candidate whose missing parameters all have visible defaults
  passes the arity filter. The defaults are appended later (step 4).
- **Variadic functions:** the declared parameters must match; the extra arguments must be
  scalars and receive C's default promotions.
- **Constructor conversions:** a class parameter may accept an argument through one
  converting constructor (`T(x)`), but not a chain of them. If two constructors could do
  it, that argument is ambiguous.
- **Constructors themselves** are overloaded the same way. `T(args)` gathers `T`'s
  constructors as candidates and runs the same `selectOverload`. A default constructor is
  chosen by calling it with zero arguments.

## Step 4: recording the decision

Once a candidate is chosen, annotation writes the result into the AST, so no later stage
has to repeat the lookup:

- **The callee identifier** is re-bound to the chosen symbol, and its display name
  becomes `area.24`.
- **The call node** stores the chosen function type, the symbol, and the owner class and
  member index for methods.
- **Argument conversions:** the converting constructor chosen for any argument is
  recorded, and later turned into an explicit conversion node on that argument. Ordinary
  conversions (`int` → `double`, …) are inserted as explicit cast nodes too.
- **A call plan** is published. It holds the chosen signature and the **complete argument
  list**, with default-argument expressions appended. Defaults are evaluated afresh at
  every call.

## Step 5: IR lowering

Lowering never looks at the overload set again:

- Every IR function's ID **is** its semantic symbol ID, so `area.24` in the IR is symbol 24.
- The call's callee lowers to a reference to function 24. A method call additionally
  passes the receiver as the hidden first `this` argument.
- Arguments are evaluated left to right from the call plan, then emitted as `param …`
  followed by `call`.

## Step 6: code generation

Each defined function gets the assembly symbol `name.id`, so the three overloads become
`area.19`, `area.22` and `area.24`.

- **No name mangling scheme is needed.** The unique symbol ID already distinguishes
  overloads, which is why baka doesn't need C++'s Itanium mangling (`_Z4areai`, …).
- The call becomes `dla $t9, area.24; jalr $t9`, exactly like a call to a function that
  isn't overloaded.
- **Library functions** keep their plain names (`printf`). That's safe because built-ins
  can't be overloaded.

---

## Summary

```
declaration:  "area" → [area.19 (int), area.22 (int,int), area.24 (double)]   one list per scope
call:         lookup "area" in nearest scope → candidates
              filter by arity (with visible defaults)
              unique exact match? → done
              else unique viable-by-conversion? → done
              else error (ambiguous / no viable overload)
record:       callee bound to area.24, conversions made explicit, defaults appended
IR / MIPS:    call @area.24  →  dla $t9, area.24 ; jalr $t9
```

| Question | Answer |
|---|---|
| Can overloads differ only by return type? | No, that's an error. |
| Do inner-scope declarations merge with outer overloads? | No. The nearest scope that has the name hides the rest. |
| Is there a promotion/conversion ranking like C++? | No. Only "unique exact match" or "unique viable", otherwise ambiguous. |
| Does `const` on a parameter distinguish overloads? | Top-level `const` doesn't; `const T*` vs `T*` does. |
| Can I take the address of an overloaded function? | Not by its bare name; it's only resolved in a call. |
| How do overloads get distinct assembly names? | Each is a separate symbol, and its ID becomes part of the name (`area.24`). |
| Are methods and constructors overloaded the same way? | Yes, they use the same `selectOverload`, with candidates found by member lookup. |
