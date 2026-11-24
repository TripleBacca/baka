# 18. Virtual functions (implementation plan)

Status: **planned, not implemented**. This page is the design to follow when the feature
is built. Nothing here exists in the compiler yet.

## Scope

Single inheritance only, matching the language (`class D : public B`). Supported:

* `virtual` member functions and overriding in derived classes. An override has the
  same name and parameter types; `override` and `final` are accepted as contextual
  keywords and checked.
* virtual destructors;
* pure virtual functions (`virtual int area() = 0;`), which make a class abstract;
* dynamic dispatch for calls through pointers, references, and unqualified calls inside
  member functions. A call on an object whose dynamic type is known (a local, or a
  by-value parameter) may be devirtualized;
* `delete(p)` through a base pointer runs the most-derived destructor when the
  destructor is virtual.

Out of scope: multiple or virtual inheritance, RTTI (`typeid`, `dynamic_cast`), covariant
return types, qualified non-virtual calls `Base::f()` (the language has no `::`),
virtual functions in unions, and pointers to members.

## Language rules to enforce

1. `virtual` only on non-static member functions; never on constructors or in unions.
2. A member function overrides a base virtual function with the same name and the same
   (adjusted) parameter types, and is then virtual even without the keyword. The return
   type must be identical (no covariance), otherwise it is diagnosed.
3. `override` requires that the function overrides something; `final` forbids further
   overriding.
4. A class with a pure virtual function (declared, or inherited and not overridden) is
   abstract. You cannot create objects of it, pass or return it by value, or `new` it.
5. A virtual call from a constructor or destructor dispatches to the class being
   constructed or destroyed. The vptr protocol below provides this.
6. Braced aggregate initialization of a polymorphic class is rejected; it has a hidden
   member and needs a constructor.

## Design

### Object layout

* The first polymorphic class in a hierarchy gets a hidden `vptr` field at offset 0,
  pointer-sized and pointer-aligned. Derived classes reuse it. Single inheritance keeps
  the base at offset 0, so no `this` adjustment is ever needed.
* `Types::ComputeStorageLayout` reserves the slot before the first declared member and
  records `Layout::vptrOffset`. `ir::Type` gains the same optional offset so lowering and
  the verifier know about it, and the IR printer shows it in record definitions.

### Virtual tables

* Semantic: each polymorphic class gets an ordered list of slots. The base's slots are
  inherited, an override replaces its slot, and new virtual functions append. The slot
  number is stored on `Member` (`vtableSlot`). When the destructor is virtual it takes
  slot 0.
* IR: one internal read-only global per polymorphic class,
  `.vtable.<Class>: array<ptr<fn>, N>`. Its `initialValues` are `Function` operands; pure
  virtual slots point to a runtime `__baka_pure_virtual` that aborts. The backend already
  emits function relocations in globals (`data.cc`), and the interpreter already
  executes function values, so both are covered.

### Construction and destruction

* Constructor lowering (`Lowerer::construct` and constructor bodies) stores the class's
  vtable address into `vptr` after base construction and before member initializers and
  the body. Implicit constructors do the same, so a polymorphic class always has a
  non-trivial default constructor.
* Destructor lowering stores the class's own vtable address on entry, before the body.
  Destroying the base afterwards resets it again, giving rule 5.
* Record copies (IR `copy` of a record value) copy the vptr. Slicing a derived object
  into a base-class value must restore the base vtable, so the conversion that produces
  the base subobject value re-stores `vptr` in the destination.

### Calls

* Semantic call metadata records `virtualSlot` when the selected member is virtual and
  the call cannot be devirtualized. Overload resolution is unchanged; it still uses the
  static type.
* Lowering: `receiver` gives `this`. Then load `vptr`, offset by `slot * 8`, load the
  function pointer, cast it to the member's IR function type, and `call` it with `this`
  as the first argument. No new IR opcodes are needed.
* `delete(p)` with a virtual destructor loads slot 0 and calls it before deallocating.

### Optimization and backend

* The block-local optimizer already treats loads through pointers conservatively, so a
  vptr is re-read after any call or store. A later pass could devirtualize when the
  vtable global is stored and loaded in the same block.
* The MIPS backend needs nothing new: indirect calls already go through `$t9`.

## Implementation steps

1. Lexer and parser:
   * `virtual` keyword;
   * `override`/`final` as contextual identifiers after the parameter list;
   * the `= 0` pure specifier on member function declarations;
   * `IsVirtual`, `IsPure`, `IsOverride` and `IsFinal` on function nodes and member
     declarations, shown in AST printing.
2. Semantic types: `Member::{isVirtual, isPure, vtableSlot}` and
   `Type::{polymorphic, vtable}`; layout reserves the `vptr`.
3. Declarations: override matching, rules 1–4 and 6, and abstract-class checks at
   object creation, by-value use and `new`.
4. Calls: mark virtual call sites.
5. Lowering: vtable globals, the vptr stores in constructors and destructors, dispatch,
   virtual `delete`, and the slicing fix-up.
6. Runtime: `__baka_pure_virtual`. The IR interpreter throws; the MIPS runtime prints a
   message and aborts.
7. Tests:
   * semantic fixtures: `override` with no virtual base function, a `final` violation,
     abstract instantiation, a return-type mismatch;
   * IR/codegen fixtures: a shape hierarchy used through base pointers and references,
     virtual calls from constructors, virtual destructors with `delete`, arrays of base
     pointers, and slicing.

## Design decisions (planned)

| Decision | Why |
|---|---|
| A single vptr at offset 0 | With single inheritance this is the simplest correct layout; `this` never needs adjusting. |
| Vtables as ordinary IR globals of function pointers | Reuses existing relocation emission, interpreter support for function values, and indirect calls. No new IR concepts. |
| Set the vptr in constructors *and* destructors | Gives C++ semantics for virtual calls during construction and destruction without special cases at call sites. |
| No covariant returns or multiple inheritance | Both need `this`/result adjustment thunks, which this language doesn't otherwise need. |

## Risks

* The memberwise copy and slicing paths are spread across initialization plans and
  aggregate value lowering. Every place that copies a base subobject out of a derived
  object must restore the vptr.
* Constant folding and CSE must not treat the constructor's vptr store as dead. The
  existing rule that a store through a pointer forgets all facts already keeps it.
