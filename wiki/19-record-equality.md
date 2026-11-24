# 19. Record equality

`a == b` and `a != b` work on structs and classes. They compare **values, field by field**.
Pointers are unchanged: `p == q` still compares addresses.

## Using it

```cpp
struct Vec { double x; double y; };
Vec a = {1.0, 2.0}, b = {1.0, 2.0};
a == b;          // true
Vec& r = b;
a != r;          // false: a reference is compared like the object it names

class Animal { public: int legs; };
class Dog : public Animal { public: int tricks; };
Dog d1, d2;
Animal& x = d1; Animal& y = d2;
x == y;          // true only if both are Dogs and legs and tricks both match
```

### Which operands compile

| Operands (each an object, reference, `*p`, member, element or call result) | Result |
|---|---|
| The same struct type (`SB`/`SB&` with `SB`/`SB&`) | Fields compared |
| Two structs of different types, including base and derived | **Error** |
| A struct and a class | **Error** |
| The same class, or a base and a derived class | The dynamic classes must match, then all of that class's fields are compared |
| Two unrelated classes | `false` (`!=` gives `true`) |
| Anything containing a union, at any depth | **Error** |

`const` is ignored. `<`, `>`, `<=` and `>=` on records remain errors.

A struct has no runtime type, so the only comparison that makes sense for it is between
values of one type. With `SD sd; SB& b = sd;`, `b` might name a plain `SB` or the `SB` part
of an `SD`, and nothing at run time can tell which. To compare base parts, give both
operands the base type: `SB& a = sd; a == b`.

Classes carry their dynamic class in the vptr, so mixing base and derived is safe:

```cpp
class CB { public: int x; };
class CD : public CB { public: int y; };
CD cd;  CB plain;  CD other;
cd == plain;            // false: a CD is never equal to a plain CB
CB& r = other;  cd == r;  // compares x and y if other matches, because r really is a CD
```

### What each field kind compares

| Field | Rule |
|---|---|
| Integer, `bool`, `char`, enum | `==` on the value |
| `float` / `double` | IEEE `==`: `NaN != NaN`, `-0.0 == 0.0` |
| Pointer (data or function) | Address `==`; the pointee is never followed |
| Nested struct or class | Its own record equality. A member object always has exactly its declared type, so no vptr check is needed. |
| Array | Element by element |
| Base subobject | Compared first, then the class's own fields in declaration order |

Comparison stops at the first difference. A class's vptr is not a field and is never
compared as a value.

### Diagnostics

```
cannot compare unions with '=='
cannot compare 'SD' with 'SB': a struct compares only with the same struct type
cannot compare incomplete type 'X' with '=='
incompatible comparison operands          (record with a non-record, or record ordering)
```

## Design decisions

| Decision | Why |
|---|---|
| Field by field, never `memcmp` | Padding bytes hold arbitrary data, floating point needs IEEE equality, and the vptr is not a value. |
| One generated `.eq.<T>` function per compared type | Nested records and arrays reuse one function instead of repeating code. Small ones are inlined by the IR inliner (peak live ≤ 6), so a two-field struct comparison becomes straight-line code. |
| Classes check the dynamic class first | Two objects of different classes are never equal, even when their common base fields match. Comparing through `Animal&` then compares every field of the real class. |
| Vtable slot 1 holds equality | A dynamic comparison must call the real class's `.eq`, which is only known at run time. The slot must be at the same index in every class of a hierarchy, so it is reserved next to the destructor (slot 0). Methods start at slot 2. |
| Slot 1 is filled on demand | It stays null unless the program compares that hierarchy dynamically, so programs that never compare classes get no extra functions. |
| Structs must match exactly | A struct has no runtime type (see above). |
| Unrelated classes give `false`, not an error | Whether two class values compare should not depend on how the classes inherit; with single inheritance no object can be both, so `false` is exact. |
| Unions are rejected | Which member is active is unknown, and comparing every member is wrong. |
| Operands are read in place | A copy would re-stamp a class's vptr with its static type and hide the dynamic class. An rvalue operand is spilled to a fresh local, whose class is exactly its static type. |

## How it is implemented

| Layer | Where | What |
|---|---|---|
| Semantic | `src/semantic/operators.cc` `checkRecordEquality` | Type rules above, union and incomplete checks. Result `bool`, never a constant expression. |
| | `src/semantic/operator_conversions.cc` | Record operands are materialized as lvalues (no lvalue-to-rvalue copy). |
| | `src/semantic/declarations.cc` `virtualTable` | Reserves slot 1 (`EqualitySlot`); methods start at `FirstMethodSlot` (2). |
| IR | `src/ir/equality.cc` | `equalityFunction` declares `.eq.<T>` on first use; `lowerEqualities` emits the bodies after all other functions and fills slot 1; `recordEquality` lowers the operator. |
| | `src/ir/virtual.cc` | `buildVtables` leaves slot 1 null; `vtableAddress` gives a class's vptr value. |

### Lowering a class comparison

Each operand's address is computed left to right. An operand whose dynamic class is known
at compile time ("exact": a named non-reference object, a field of one, a by-value
parameter or an rvalue) needs no vptr load:

| Operands | Code |
|---|---|
| Both exact | Different classes: constant `false`. Same class: call `.eq.<C>` directly. |
| One exact (class `C`) | Load the other vptr, compare with `&.vtable.C`; if equal, call `.eq.C` directly. |
| Neither exact | Load both vptrs and compare; if equal, call through slot 1. |

```
(load,  %t5:ptr<ptr<void>>, vptr(a))      ; neither operand exact
(load,  %t6:ptr<ptr<void>>, vptr(b))
(eq,    %t7:u1, %t5, %t6)
(if_goto, _, %t7, Same)  (goto, _, Different, _)
Same:      load slot 1 of %t5, cast to fn(ptr<Animal>, ptr<Animal>)->u1, call with a, b
Different: result = 0
```

## Tests

- `custom_tests/semantic/`: `invalid_record_eq_union.cpp`, `invalid_record_eq_nested_union.cpp`,
  `invalid_record_eq_struct_base.cpp`, `invalid_record_eq_struct_class.cpp`,
  `invalid_record_ordering.cpp`, and `valid_record_equality.cpp`.
- `custom_tests/ir/31_record_equality.cpp` runs in the interpreter and under qemu. It covers
  garbage padding, `-0.0`, NaN, short and long arrays, nested records, references, rvalues,
  dynamic classes through references, exact vs reference, base vs derived objects, unrelated
  classes with a side-effecting operand, heap objects and `*this`.
- `custom_tests/codegen/35_record_equality.cpp` checks printed results under qemu, including
  abstract base pointers that are re-pointed.
