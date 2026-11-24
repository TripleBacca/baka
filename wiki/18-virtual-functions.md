# 18. Virtual functions

In baka the class-key decides dispatch. Every non-static method of a `class` is
virtual, Java style. A `struct` is a plain record whose calls are always resolved at
compile time. There is no `virtual`, `override` or `final` keyword.

Status: **implemented**.

## Using it

```cpp
class Shape {
public:
    int area() = 0;                    // pure: Shape is abstract
    int twice() { return 2 * area(); } // dispatches on the real object
};

class Square : public Shape {
public:
    int side;
    Square() { side = 3; }
    int area() { return side * side; } // overrides Shape::area
};

int measure(Shape& s) { return s.twice(); }

int main() {
    Square sq;
    Shape* p = &sq;
    return p->area() + measure(sq);    // 9 + 18
}
```

### Rules

| Rule | Behaviour |
|---|---|
| Which methods are virtual | Every non-static method of a `class`, including inherited ones. Constructors and static methods never are. Every class destructor is virtual. |
| Structs and unions | Never polymorphic: no vptr and no dispatch. A struct may not declare a pure method. |
| Separate hierarchies | A `class` derives only from a `class` and a `struct` only from a `struct` (page 15). |
| Overriding | A method overrides a base method with the same name, parameter types, variadic-ness and const-ness. The return type must be identical (no covariance). Any other method adds a new slot, which hides base overloads of the same name in the usual way. |
| Pure methods | `int area() = 0;` declares a method with no body. A class with a pure slot, declared or inherited and not overridden, is **abstract**. |
| Abstract classes | Cannot be used by value. This rules out objects, arrays, members, by-value parameters and results, `T(...)` and `new`. Pointers and references to them are fine. |
| Braced initialization | `C c = {1, 2}` is rejected for every class, because the hidden vptr needs a constructor. `C c = {}` default-constructs. |
| During construction and destruction | A virtual call reaches the class whose constructor or destructor is running, as in C++. Calling a pure method then traps with `pure virtual method called`. |
| `delete(p)` | Runs the destructor of the object's real class, so deleting through a base pointer is safe. |
| Value semantics | Copy-initializing a class object from a derived object (through `*p` or a reference) produces an object of the declared type, a slice. Assignment copies the value but never changes the destination's dynamic type. |

### Comparing classes

- `a == b` compares two class **values**: the dynamic classes must match, then every field
  of that class is compared (page 19). Vtable slot 1 holds the generated equality function.
- `a ==== b` / `a !=== b` compare only the **dynamic classes**, via the vptrs (page 20).

### Diagnostics

```
cannot use abstract class 'Shape' by value
cannot construct an abstract class
cannot allocate an abstract class
overriding method 'value' must have the same return type
only non-static class methods can be pure ('= 0')
a method declaration can only be followed by '= 0'
braced class construction is unsupported; use Type(arguments)
a class cannot inherit from a struct
```

### Differences from C++

- No keyword. Every method of a class is virtual, so a derived method that C++ would
  merely *hide* overrides here. Calls through a base pointer can reach it. Use a `struct`
  for non-polymorphic records.
- Even a class without methods carries the 8-byte vptr and gets a constructor.
- Derived fields are laid out after the padded base, not in its tail padding, so for
  `class A { int x; }; class B : public A { int y; };` `sizeof(B)` is 24 where g++ gives 16.
- No covariant returns, multiple or virtual inheritance, `typeid`/`dynamic_cast` or pointers to
  members. `====` / `!===` (not in C++) give an exact dynamic-class test.

## Design decisions

| Decision | Why |
|---|---|
| Class-key selects polymorphism | One rule instead of per-method keywords. Structs stay byte-compatible C records; classes always have a vptr. |
| Classes and structs cannot inherit from each other | Every polymorphic hierarchy is rooted in a class, so the vptr is at offset 0 throughout and `this` never needs adjusting. |
| Slot 0 is the destructor, always | Every class destructor is virtual, so the slot can never be claimed by an ordinary method. A trivially destructible class stores null there and `delete` skips the call. |
| Slot 1 is record equality, always | `a == b` on classes calls the dynamic class's generated `.eq.<Class>` through it (page 19). It must sit at the same index in every class of a hierarchy, so it is reserved like slot 0 and stays null unless the program compares that hierarchy. Methods start at slot 2. |
| Vtables are ordinary IR globals | `.vtable.<Class>` is a read-only `[N x ptr<void>]` of function relocations. The verifier, interpreter and MIPS data emitter already handled these, so no new IR concepts were needed. |
| Dispatch is plain IR | Load vptr, offset by `slot * 8`, load, cast to the method's signature, `call`. No new opcodes. Indirect calls already go through `$t9`. |
| Vptr stored by constructors *and* destructors | Gives the C++ behaviour during construction and destruction (rule above) without special cases at call sites. |
| Every store of a class value re-stamps the vptr | A class value may have been read from a derived object, so the new object always gets the vptr of its own static type. This covers initialization, temporaries and returned values. Assignment instead saves and restores the destination's vptr. By-value parameters are re-stamped on function entry. |
| Calls on named objects are devirtualized | A local, global or by-value parameter (or a field of one) has exactly its static type, so `obj.f()` calls directly. |
| Pure slots point at `__baka_pure_virtual` | A clear message and `abort()` instead of a jump through null. The IR interpreter reports it as an external call. |

## How it is implemented

| Layer | Where | What |
|---|---|---|
| Semantic types | `src/semantic/types.{hh,cc}` | `Member::{isVirtual, isPure, vtableSlot}`, `Type::vtable`, `Layout::vptrOffset`. The root class reserves 8 bytes at offset 0. `polymorphic()`, `abstract()`, `setVirtualTable()`. |
| Declarations | `src/semantic/declarations.cc` | `virtualTable()` builds slots when a class completes: inherit, replace on override, append. It also diagnoses return and const mismatches. `concrete()` rejects abstract types by value. |
| Pure specifier | `src/semantic/aggregates.cc` | A method declaration with initializer `0` becomes `isPure`. Any other initializer is an error. The parser needed no change. |
| Construction plans | `src/semantic/execution_plans.cc`, `initialization_plans.cc` | Classes always need construction. Braced lists always construct. |
| Abstract checks | `calls.cc` (`T(...)`, `new`), `declarations.cc` (objects, members, signatures) | |
| IR lowering | `src/ir/virtual.cc` | `buildVtables`, `storeVptr`, `assign`, `virtualCallee`, `exactType` |
| | `lifetimes.cc`, `functions.cc` | Vptr store after the base constructor, before members. Destructor entry re-stamp. By-value parameter re-stamp. |
| | `calls.cc`, `heap.cc`, `lowering.cc`, `operators.cc`, `addresses.cc` | Dispatch, `delete` through slot 0, vptr re-stamp in `store`, `assign` for `=`. |
| `==` and `====` | see pages 19 and 20 | Record equality (`.eq.<T>`, slot 1) and dynamic class comparison. |
| IR printing | `src/ir/printer.cc` | Root classes print `record(vptr, ...)`. |
| Runtime | `src/codegen/runtime.cc` | `__baka_pure_virtual` writes the message to stderr and calls `abort`. The name is reserved in `symbols.cc`. |

### Example lowering

`p->area()` with `Shape* p`, where `area` is slot 2 (slots 0 and 1 are the destructor and equality):

```
(cast,   %t4:ptr<ptr<ptr<void>>>, %t3:ptr<Shape>, _)   ; address of the vptr
(load,   %t5:ptr<ptr<void>>, %t4, _)                    ; vptr
(offset, %t6:ptr<ptr<void>>, %t5, 16:i64)              ; slot 2
(load,   %t7:ptr<void>, %t6, _)
(cast,   %t8:ptr<fn(ptr<Shape>)->i32>, %t7, _)
(param,  _, %t3:ptr<Shape>, _)
(call,   %t9:i32, %t8, _)
```

The constructor of `Square` calls `Shape`'s constructor, then stores
`&.vtable.Square` at offset 0, then initializes members and runs its body.

## Tests

- `custom_tests/semantic/`: `valid_virtual_functions.cpp`, `invalid_abstract_*.cpp`,
  `invalid_override_return_type.cpp`, `invalid_struct_pure_method.cpp`,
  `invalid_pure_specifier.cpp`, `invalid_braced_class.cpp`, and the class/struct base
  checks `invalid_class_from_struct.cpp` and `invalid_struct_from_class.cpp`.
- `custom_tests/ir/29_virtual_functions.cpp` runs in the interpreter and under qemu.
  It covers dispatch through pointers, references and implicit `this`, virtual calls
  in constructors and destructors, `delete` through a base pointer, slicing, assignment
  through a base reference, by-value parameters and returned temporaries.
- `custom_tests/codegen/29_virtual_dispatch.cpp` and `30_virtual_values.cpp` check
  printed output under qemu.
- Tests for `==` and `====` on classes are listed on pages 19 and 20.
- Several older fixtures used `class` as a plain brace-initialized aggregate. They now
  say `struct`, which is what those tests meant.
