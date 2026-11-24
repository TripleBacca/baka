# 20. Dynamic class comparison: `====` and `!===`

`a ==== b` asks whether two objects have the same **dynamic** class, the class they were
actually built as, whatever their declared types say. `a !=== b` is exactly
`!(a ==== b)`. Neither operator exists in C++; they replace `typeid(a) == typeid(b)`.

```cpp
class Animal { public: int legs; };
class Dog : public Animal { };
class Cat : public Animal { };

Dog rex, fido;  Cat tom;
Animal& a = rex;  Animal& b = fido;  Animal& c = tom;
Animal* p = &tom;

a ==== b;       // true: both are Dogs
a !=== c;       // true: same declared type, different dynamic classes
p ==== rex;     // false now; true after p = &rex
rex ==== fido;  // true, decided at compile time
```

## `====` versus `==`

| | `a == b` (page 19) | `a ==== b` |
|---|---|---|
| Question asked | Are the two **values** equal? | Are the two objects of the same **class**? |
| Compares | Every field (classes: after checking the class) | Only the vptrs; fields are never read |
| Structs | Same struct type only | Error (a struct has no vptr) |
| Pointers | Address comparison, as in C | Compares the objects they point to |
| Example: two Dogs with different fields | `false` | `true` |

## Rules

### Operands

Each operand is one of:

- a **class lvalue**: a named object, a reference, `*p`, a member, an array element, a call
  that returns a reference, or `*this`;
- a **pointer to a class**: `p ==== q` compares `*p` and `*q`, and `this` works.

Forms can be mixed (`p ==== d`, `r ==== *q`), and the declared types may differ
(`Animal&` with `Dog`). The result is `bool`.

These are compile errors:

| Operand | Why |
|---|---|
| A struct, union, or pointer to one | Only classes carry a vptr. |
| `void*`, `Class**`, an integer | Not an object of a class. |
| A class **rvalue** (a function returning `Dog` by value) | Copying a class value gives the copy its declared class's vptr, so the answer would only ever reflect the declared type. |
| An **array** of classes | Arrays are not decayed to a pointer here, because `arr ==== x` silently comparing `arr[0]` would be surprising. Compare an element. |
| The **null pointer literal** `0` | It names no object. |

A null pointer at run time, including `(Animal*)0`, is **undefined**: there is no check,
exactly as for `*p`.

### Spelling and precedence

`!===` follows the `==`/`!=` convention: the negated operator has the same length, with
the first `=` replaced by `!`.

Both operators bind **tighter than `==`/`!=` and looser than `<` `>` `<=` `>=`** (like C++
`<=>`). Their operands are never `bool`, so this only turns expressions that would
otherwise be errors into meaningful ones:

| Expression | Means |
|---|---|
| `a ==== b == c` | `(a ==== b) == c` |
| `a == b ==== c` | `a == (b ==== c)` |
| `a ==== b != c ==== d` | `(a ==== b) != (c ==== d)`: "do both pairs match, or both differ?" |
| `a ==== b && c ==== d` | as written |

### During construction and destruction

While a base class's constructor or destructor runs, the object's vptr is the base's
(page 18), so the object is `====` to a plain base object:

```cpp
Base* probe;                       // points to a plain Base
class Base {
public:
    int during;
    Base() { during = probe != 0 && this ==== probe; }   // 1 even while building a Derived
};
```

A sliced copy (`Animal x = rex;`) is a real `Animal`, so `x ==== rex` is `false`.

### Compile-time folding

Some operands are known to have exactly their declared class ("exact"):

- a named object that is not a reference (including a by-value parameter),
- a field of an exact object,
- an element of a real array (`pack[i]` where `pack` is an array, not a pointer, and not
  an array parameter, which is a pointer).

| Operands | Code |
|---|---|
| Both exact | A constant: `true` if the declared classes are equal. No loads. |
| One exact (class `C`) | Load the other vptr and compare it with the address of `.vtable.C`. |
| Neither exact | Load both vptrs and compare them. |

Operands are **always evaluated**, left to right, even when the result is a constant:
`pack[i++] ==== rex` still increments `i`. Folding only removes the vptr loads; the
optimizer removes leftover pure address arithmetic.

**Caveat:** a named object counts as exact even while a base constructor is still running
on it. Code reached from `Animal()` that names a global `Dog d` therefore sees
`d ==== otherDog` folded to `true`, while the vptr at that moment is still `Animal`'s.
Call devirtualization (page 18) has the same blind spot.

## Diagnostics

```
operands of '====' must be class objects or pointers to them
operands of '====' must be lvalues
operands of '====' cannot be arrays; compare an element
cannot compare the class of a null pointer
```

Each message uses the operator actually written (`====` or `!===`).

## Design decisions

| Decision | Why |
|---|---|
| Compare vptrs | Each class has exactly one vtable (page 18), so the vptr is an exact class tag. The test is at most two loads and a compare. |
| A new operator instead of RTTI | No `typeid` objects, no type names in the binary, no runtime library; the vtables already exist. |
| Pointers compare their pointees | `p ==== q` is the common case with polymorphic code. Comparing the pointers themselves is what `==` already does. |
| Class rvalues and arrays are errors | Both would silently answer a different question (the declared class, or `arr[0]`). |
| Tighter than `==` | Accepts more meaningful code and changes no valid expression. |
| Fold exact operands | Free for named objects, which are the most common operands in tests and examples. |

## How it is implemented

| Layer | Where | What |
|---|---|---|
| Lexer | `src/lexer/lexer.cc`, `src/lexer/operators.hh` | Operators are lexed by **longest match**: try the longest spelling in the table (at most `MaxOperatorLength` = 4 characters) and fall back to shorter ones. The old approach grew a lexeme one character at a time while every prefix was an operator, and could never reach `====` or `!===` because `===` and `!==` are not operators. Every other operator lexes as before. |
| Tokens and AST | `src/types/token`, `src/types/parser/ast/operators.hh` | `OP_STRICT_EQ` / `OP_STRICT_NEQ`. The AST ops sit outside the `OP_EQ..OP_GE` range that the semantic pass uses for arithmetic and pointer comparisons. |
| Precedence | `src/types/parser/ast/precedence_climbing.hh` | Level 9. Relational and every level above moved up by one (relational 10 … `.`/`->` 16). The parser only compares levels with each other, so the renumbering changes nothing else. |
| Semantic | `src/semantic/operators.cc` `checkStrictEquality` | Checks each operand without decay, in order: null literal, array, class (must be an lvalue), pointer to class, anything else. Result `bool`. |
| | `src/semantic/operator_conversions.cc` | Class operands stay lvalues so the vptr is read in place; pointer operands are loaded as values. |
| IR | `src/ir/virtual.cc` `strictEquality` | Evaluates both operands, then folds or loads vptrs as in the table above, and emits `eq` (`====`) or `ne` (`!===`). |
| | `src/ir/virtual.cc` `exactType` | Also recognizes elements of real arrays, looking through the array-to-pointer conversion the semantic pass inserts for indexing. This also lets `pack[i].method()` call directly. |

```
; a ==== b, both references (neither exact)
(load, %t1:ptr<Animal>, &a, _)
(load, %t2:ptr<Animal>, &b, _)
(cast, %t3:ptr<ptr<ptr<void>>>, %t1, _)     ; address of a's vptr
(load, %t4:ptr<ptr<void>>, %t3, _)
(cast, %t5:ptr<ptr<ptr<void>>>, %t2, _)
(load, %t6:ptr<ptr<void>>, %t5, _)
(eq,   %t7:u1, %t4, %t6)
```

## Tests

- `custom_tests/semantic/`: `invalid_strict_eq_{struct,pointer,void_pointer,double_pointer,
  rvalue,array,null}.cpp` and `valid_strict_equality.cpp` (pointers, `this`, mixed forms,
  `const` pointers, precedence).
- `custom_tests/ir/30_strict_equality.cpp` (interpreter and qemu): objects, base
  references, pointers and `this`, mixed pointer/object operands, slices, array elements,
  members, precedence, folding, and a folded operand with a side effect.
- `custom_tests/codegen/34_strict_equality.cpp` (printed output under qemu): heap pointers,
  a pointer re-pointed between classes, returned references, and the check inside a base
  constructor.
