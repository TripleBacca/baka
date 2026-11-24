# 17. Captureless lambdas

Lambda expressions without captures: `[](int x) { return x * 2; }`. A captureless lambda
has no state, so baka turns its body into an ordinary internal function. The lambda
expression then simply names that function. No stage after parsing needed new IR,
optimizer or backend support.

Status: **implemented**.

## Using it

```cpp
int apply(int (*f)(int v), int x) { return f(x); }

void sortWith(int* a, int n, bool (*before)(int x, int y));

struct Operation { int (*run)(int a, int b); };

int main() {
    int doubled = [](int x) -> int { return x * 2; }(21);     // immediate call
    int (*twice)(int v) = [](int v) { return v * 2; };         // stored, result deduced
    int r = apply([](int v) { return v + 1; }, 4);             // passed as a callback

    int values[4];
    sortWith(values, 4, [](int x, int y) { return x > y; });   // comparator

    Operation op;
    op.run = [](int a, int b) { return a * b; };               // in a struct field

    [] { /* no parameters, no result */ }();
    return doubled + twice(3) + r + op.run(2, 3);
}
```

### Syntax

```
[] ( parameters ) -> R { body }
[] ( parameters ) { body }          result deduced
[] { body }                         no parameters, result deduced
```

### Rules

* The capture list must be empty. `[x]`, `[=]` and `[&]` are rejected.
* Without `-> R` the result type is **deduced**: `void` if the body has no
  `return expr;`, otherwise the type of the returned value (after lvalue, array and
  function decay, without top-level `const`). Every return must agree.
* The body can use globals, functions, types and other lambdas. It **cannot** use the
  enclosing function's parameters or local variables, since those would be captures.
* The value of a lambda expression is a **function pointer** with the lambda's
  signature, which a captureless C++ closure converts to. Because baka has no `auto`
  and no templates, every use goes through a function pointer anyway:
  * call it immediately;
  * store it in a function-pointer variable, array element, struct field or global;
  * pass it to a function-pointer parameter.
* Parameters can be anything a function parameter can be, including references:
  `[](int& r) { r = r + 1; }(counter)`.

### Diagnostics

```
lambda captures are not supported; only captureless lambdas '[]' are allowed   (parse)
undeclared name 'x' (a lambda cannot capture local variables)
inconsistent deduced return types in lambda
expected '{' to begin the lambda body                                           (parse)
```

### Differences from C++

* `sizeof` of a lambda is the size of a function pointer, and two lambdas with the
  same signature have the same type. In C++ each closure has its own unique type, but
  without `auto` or templates that is mostly unobservable.
* A lambda inside a member function cannot access that class's private members,
  because its body is not a member function.
* Lambdas can't use local typedefs of the enclosing function.

## Design decisions

| Decision | Why |
|---|---|
| Hoist the body to an internal function `__lambda_N` | A captureless lambda *is* a function. Registration, overloading, forward calls, lowering, the interpreter and both backends already handle functions, so nothing downstream changes. |
| Hoist in the parser, right **after** the enclosing top-level declaration | Everything visible at the lambda's position is visible in the hoisted body, including the enclosing class (complete after its definition) and the enclosing function itself. The enclosing code calls the lambda before its definition, which forward calls already allow. Nested lambdas are hoisted inner-first. |
| The expression is a plain identifier naming the hoisted function | Function designators already decay to function pointers in value contexts and can be called directly, so no new expression node or typing rule was needed. The AST printout shows `__lambda_N` and the hoisted function, marked `Lambda: deduced result` or `declared result`. |
| Discard lambdas parsed during a rolled-back function probe | `TryParseFunction` sometimes parses ahead and backtracks; lambdas parsed in that window are dropped, so they are not hoisted twice. |
| Deduce results in a pass before all other typing | Callers must see the final signature. The deduction pass types the return statements of deduced lambdas first, then re-signatures the function (symbol, AST node, body context, pending return checks). Because inner lambdas are hoisted first, nested deduction resolves inner-first. A global initializer that uses a lambda also sees the final type. |
| Unique internal names `__lambda_N`, `static` linkage | No collision with user names (the leading `__` is reserved) or across files. |
| Report a captured name as an undeclared name with a hint | The hoisted body really doesn't see the enclosing scope. The hint says why the name is missing. |
| No `auto` | Adding type deduction for variables is a separate feature; function pointers already give every captureless-lambda use case. |

## How it is implemented

| Stage | File | Change |
|---|---|---|
| AST | `src/types/parser/ast/function.hh` | `FunctionNode::IsLambda`, `DeducedReturn` (printed as `Lambda: …`) |
| Parser | `src/parser/expressions.cc` | `ParseLambda` in primary position on `[`: capture check, parameter list, optional `-> [const] [unsigned] T *…&`, body via `CompoundStatement()`. It builds a `static` `FunctionNode` named `__lambda_N` (return type `void` as the placeholder when deduced) and returns a primary expression naming it. |
| Parser | `src/parser/parser.hh/.cc` | `PendingLambdas`; the program loop appends them after each top-level declaration |
| Parser | `src/parser/function.cc` | rolls `PendingLambdas` back when `TryParseFunction` backtracks |
| Semantic | `semantic_analyzer.hh`, `registration.cc` | `Context::lambda` and `Context::deduced` on the body context |
| Semantic | `resolution.cc` | the capture hint on undeclared names inside lambda bodies |
| Semantic | `annotation.cc` | deduction pre-pass and `deduceLambdaResult` (agreement check, re-signature) |
| Semantic | `symbols.cc` | `Symbols::deduceFunctionResult` (same parameters, new result) |

### Example

```cpp
int main() { return [](int a, int b) { return a + b; }(40, 2); }
```

becomes, after parsing,

```cpp
int main() { return __lambda_0(40, 2); }
static int __lambda_0(int a, int b) { return a + b; }   // result deduced as int
```

and lowers to an ordinary internal function plus a direct `call`.

## Tests

* `custom_tests/semantic/valid_lambdas.cpp`, `invalid_lambda_capture_use.cpp`,
  `invalid_lambda_inconsistent_returns.cpp`.
* `custom_tests/ir/28_lambdas.cpp`:
  * immediate calls, with declared and deduced results;
  * lambdas stored in function pointers, arrays of structs, struct fields and globals;
  * a sort comparator, nested lambdas and reference parameters;
  * a `double` deduced result, a lambda inside a member function, and a no-parameter lambda.

  It runs in the interpreter and under QEMU, optimized and unoptimized.
* `custom_tests/parse/41_cpp_features.cpp` pins the hoisted AST.
