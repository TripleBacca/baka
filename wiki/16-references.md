# 16. References

Lvalue references (`T&`) give another name to an existing object. In baka they are
checked in **semantic analysis** and become plain pointers in the **IR**, so the
optimizer, interpreter and MIPS backend needed no changes.

Status: **implemented**.

## Using it

```cpp
void swap(int& a, int& b) { int t = a; a = b; b = t; }

int table[4];
int& slot(int i) { return table[i]; }        // reference result: usable as an lvalue

int twice(const int& v) { return v * 2; }

int main() {
    int x = 3, y = 7;
    swap(x, y);              // x == 7, y == 3
    int& r = x;              // r is another name for x
    r = r + 1;               // x == 8
    slot(1) = 5;             // writes table[1]
    slot(2) += slot(1);
    const int& c = x + 100;  // const& binds a temporary holding 108
    return twice(4) + twice(x + 1) + c;
}
```

### Where references may appear

* locals, globals and local statics: `int& r = x;`
* function parameters, including constructor parameters and parameters with defaults
* function results: `int& at(int i)`
* through typedefs: `typedef int& IntRef;`
* with pointers and arrays: `int*& p` (reference to a pointer), `int (&a)[3]`
  (reference to an array)

### Binding rules

| Destination | Accepts |
|---|---|
| `T&` | an **lvalue** of type `T`, or of a class derived from `T`. It cannot drop `const`: a `const T` object needs `const T&`. |
| `const T&` | the same, **plus** any expression convertible to `T`. The value is converted and stored in a temporary, and the reference binds to that temporary. |

* A reference must be initialized (parameters are initialized by the call) and cannot be
  reseated. Every use of its name means the bound object.
* `&r` is the address of the bound object; `sizeof(r)` is the size of `T`.
* Reference collapsing: with `typedef int& R;`, `R&` is `int&`.
* Overloading: binding an lvalue of exactly `T` is an exact match. `f(int&)` is preferred
  over `f(const int&)` for a non-const `int`; a `const int` picks `f(const int&)`.

### Rejected

| Program | Diagnostic |
|---|---|
| `int& r;` | `reference requires an initializer` |
| `int& r = 5;`, `int& r = longValue;` | `initializer is not convertible to the declared type` |
| `const int x = 1; int& r = x;` | `initializer is not convertible to the declared type` |
| `f(3)` with `void f(int&)` | `no viable overload for this call` |
| `const int& r = x; r = 4;` | `assignment target is not a modifiable lvalue` |
| `int& r[2];` | `arrays of references are not allowed` |
| `int& (*p);` | `pointers to references are not allowed` |
| `void f(void& v)` | `references to void are not allowed` |
| `struct S { int& r; };` | `reference members are not supported` |
| `T(const T& other)` | `user-defined copy constructors are not supported` |
| `int&& r = 1;` | `rvalue references are not supported` (parse error) |
| `const T& r = make();` where `T` has a destructor | `initializer is not convertible to the declared type` |

Returning a reference to a local is not diagnosed; as in C++, using it is undefined.

## Design decisions

| Decision | Why |
|---|---|
| A new semantic `TypeKind::Reference`, interned like pointers | Reference-ness has to survive in signatures, overload resolution and declarations. Its layout is that of a pointer, because a reference object stores an address. |
| Expressions never have reference type | Naming a reference, or calling a function that returns one, yields an **lvalue of `T`**. All existing operator, member and conversion rules then work unchanged on references. |
| `ExprResult` remembers the undecayed category and type (`sourceLValue`, `sourceType`) | Call arguments and return values are typed as decayed rvalues. Binding needs to know whether the expression named an object, and must still see an array type for `T (&)[N]`. |
| One central binding check, `bindsReference`, reached from `isAssignable` | Initializers, arguments, returns and default arguments all ask `isAssignable`, so reference destinations work everywhere at once. |
| Two explicit conversion nodes on the AST | Lowering stays explicit, as for every other conversion. `BindReference` lowers its operand as an address; `MaterializeTemporary` converts to `T`, stores it in fresh storage and yields that address. |
| IR represents `T&` as `ptr<T>` | The IR, verifier, optimizer, interpreter and MIPS backend already handle pointers. Nothing past lowering changed. |
| Derived-to-base binding adjusts the address | Base subobject offsets are computed exactly as for inherited member access. |
| No temporaries of types with destructors | Binding them to a local `const T&` needs lifetime extension, so the destructor would have to run at scope end instead of the end of the full expression. Rejecting is honest; extension is future work. |
| Reject reference members | The language has no constructor member-initializer lists (`: r(x)`), so a reference member could never be bound. |
| Reject user-defined copy constructors | Record copies are memberwise IR copies everywhere (by-value arguments, returns, initialization). A user `T(const T&)` would be silently skipped by most copies, which is worse than an error. |
| Collapse references formed through typedefs | `int& &` cannot be written, so a reference to a reference only arises via a typedef, where C++ collapses it. |
| Tie-break `T&` over `const T&` | Matches C++ ranking for qualification-adding reference bindings, so the common `f(T&)`/`f(const T&)` overload pair is not ambiguous. |

## How it is implemented

| Stage | File | Change |
|---|---|---|
| AST | `src/types/parser/ast/declaration.hh` | `DeclarationIdentifierNode::Reference` (`&` after the `*`s, closest to the name) |
| Parser | `src/parser/declaration.cc` | `MatchReferenceDeclarator`: `&` sets the flag, `&&` is reported as an rvalue reference |
| Types | `src/semantic/types.hh/.cc` | `TypeKind::Reference`, `reference()`, `isReference()`, `referenced()`; pointer layout; `pointer()`/`array()` refuse references; `constant()` leaves references alone |
| Declarators | `src/semantic/ast_declarations.cc` | applies `&` after pointers; collapsing; pointer/array/void/function diagnostics |
| Declarations | `src/semantic/declarations.cc` | rejects reference members and copy constructors |
| Typing | `primary.cc`, `calls.cc` | names and calls of reference type are lvalues of the element type |
| Binding | `expression_types.cc` | `bindsDirectly`, `bindsReference`, `hasDestruction`; `isAssignable` delegates for references |
| Overloads | `overloads.cc` | exact match for exact lvalue bindings; fewest-added-qualifiers tie-break |
| Conversions | `conversions.cc`, `implicitCast.hh` | `bindReference` writes `BindReference` or `MaterializeTemporary`; it consumes the recorded target first, to avoid rebinding its own operand |
| Checks | `checks.cc` | `reference requires an initializer` |
| Lowering | `src/ir/lowering.cc` | `importType`: reference → `ptr<T>` |
| Lowering | `src/ir/addresses.cc` | naming a reference loads the stored address; a reference-returning call's value *is* its address |
| Lowering | `src/ir/expressions.cc` | `referenceAddress` (with base adjustment) and temporary materialization |
| Lowering | `src/ir/lifetimes.cc` | `valueRoot` stops at reference conversions, so a bound record is never constructed into the reference's own storage |

### Example lowering

```cpp
void swap(int& a, int& b) { int t = a; a = b; b = t; }
int main() { int x = 1; int y = 2; swap(x, y); return x; }
```

```
function @swap.21: fn(ptr<i32>, ptr<i32>)->void {      ; reference parameters are addresses
  parameter a.23: ptr<i32>
  parameter b.24: ptr<i32>
  local t.25: i32
  (load, %t1:ptr<i32>, &a.23:ptr<ptr<i32>>, _)         ; naming `a` loads the bound address
  (load, %t2:i32, %t1:ptr<i32>, _)                     ; ...and reading `a` reads through it
  (store, _, &t.25:ptr<i32>, %t2:i32)
  ...
function @main...
  (param, _, &x.26:ptr<i32>, _)                        ; BindReference passes &x
  (param, _, &y.27:ptr<i32>, _)
  (call, _, @swap.21, _)
```

## Tests

* `custom_tests/semantic/valid_references.cpp` and eleven invalid fixtures (all the
  diagnostics above), listed in `cases.json`.
* `custom_tests/ir/27_references.cpp`: swap, reference results as lvalues, static-local
  counters, `const&` temporaries, overloads on qualification, default arguments,
  references to pointers and arrays, struct and base-class references, constructor
  reference parameters, a global reference, `&r` and `sizeof(r)`. Interpreter plus
  QEMU, optimized and unoptimized.
* `custom_tests/parse/41_cpp_features.cpp` pins `Reference: true` in the AST.
