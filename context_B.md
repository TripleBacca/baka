# context_B.md — Teammate B handoff (expressions, calls, constants, initialization)

Branch: `saturn/slop`. Build: `cmake -S src -B build && cmake --build build --parallel 2`.
B implements the 24 B packages in [TASK_SPLIT.md](semantic_audit/TASK_SPLIT.md) on top of A's
type/symbol/declaration services. Everything lives under `src/semantic/`; the semantic stage is
reachable with `./build/baka --semantic <file>` (see [context_C.md](context_C.md) for wiring).

## 1. Summary

`ExpressionChecker` (`src/semantic/expressions.{hh,cc}`) is a full typed-expression, overload,
constant-evaluation, `sizeof`, and initialization checker. It is a `DeclarationEvaluator`, so A's
`ASTDeclarations` calls it back for enumerator/array-bound constants, and it supplies
`ASTAggregates::MemberSignature` via `memberSignature`. It never rewrites the AST: it returns typed
`ExprResult` records and reports through A's `Symbols::diagnose`. C drives it from
`SemanticAnalyzer` and owns the semantic stage/CLI.

Verified state: `./custom_test_hook parse` **57/57**, `python3 semantic_audit/teammate_a/run.py`
**17/17**, `python3 semantic_audit/run.py` no crashes (lex 113, parse 96/17), and no crashes or
timeouts classifying all 221 `semantic_audit/` fixtures with `--semantic`.

## 2. Result contract (B02)

```cpp
enum class ValueCategory { LValue, RValue };
struct ExprResult {
    TypeId type;                       // A's interned type (0 = error)
    ValueCategory category;           // lvalue/rvalue
    bool error;                       // poisoned; never produces a fabricated valid type
    bool constantExpression;          // valid integer constant-expression (B14)
    IntegerResult constant;           // {ConstantState, int64_t} when folded
    bool nullPointerConstant;         // eligible zero integral constant (B07)
    explicit operator bool() const;   // !error && type.value
};
```

Public surface: `checkValue` (rvalue/decay context), `checkCondition` (scalar conversion),
`integer(...) override` (B14 evaluator contract), `convertible(from,to,value)`, `initialize`,
`memberSignature`, `registerMinimumArity`, `setFunctionContext`.

## 3. B01–B24 coverage

| Package | Where | Notes |
| --- | --- | --- |
| B01 conversion/value policy | `isAssignable`, `promote`, `usualArithmetic`, `conversionRank`, `isNullPointerConstant` | arithmetic/enum/pointer/null conversions, lvalue vs rvalue categories. |
| B02 typed output | `ExprResult`, `makeValue`/`makeConstant`/`makeError` | error propagation; no AST rewrite. |
| B03 literals/strings | `checkConstant`, `StringLiteral` helper | int/char/unsigned/long long/bool/float/double/`nullptr`; `SLString` → `const char[N]`. |
| B04 conversions/unevaluated | `check(expr, shouldDecay)` | explicit decay control; `sizeof`/short-circuit type-check unevaluated operands without folding. |
| B05 arithmetic/logical/bitwise | `checkBinary`, `checkUnary` | integral-only bitwise/shift, short-circuit `&&`/`||` type-check both operands but only evaluate what is reached. |
| B06 pointer arithmetic | `checkBinary` (add/sub/diff), `checkIndex` | complete-object pointee checks, `ptrdiff`, result types. |
| B07 comparisons/null | `checkBinary` (relational/equality) | equality vs ordering separated; ordering rejects function/void pointers; `p==0` accepted, foldable zeros don't become null unless integral constant. |
| B08 address/index | `checkUnary` (`&`,`*`), `checkIndex` | array/function decay suppression, lvalue requirements, element type/category. |
| B09 assignment/modifiability | `checkAssignment`, `isModifiableLValue`, `flattenMembers` | recursive const fields, arrays not assignable, aggregate copies with array members allowed. |
| B10 compound/inc-dec | `checkAssignment` (compound), `checkUnary`/postfix | single left-side evaluation, correct result category. |
| B11 conditional/comma | `checkTernary`, `checkBinary` (`OP_COMMA`), comma-list branch | usual arithmetic/pointer composite; single-element comma wrappers preserved for constness; genuine comma is not a constant expression (C11 ICE rule). |
| B12 casts | `checkCast` | through A's `specifier`/`typeName`; rejects array/function and unsupported aggregate casts. |
| B13 sizeof | `checkSizeof` | type or expression; uses `Types::layout`; suppresses decay; result `ULong`. |
| B14 constant evaluator | `integer`, `check` constants | distinguishes ICE validity from foldability: comma/increment/assignment/function-call in evaluated position invalidate; unevaluated operands still type-check. |
| B15 bounds/VLA | `ASTDeclarations` + `integer` | numeric bounds only; VLAs are not supported (diagnosed through A's bound checks). |
| B16 scalar init | `initializeScalar` | brace shape, narrowing diagnostics, `const` requires initializer (C's rule via analyzer). |
| B17 aggregate/string init | `initializeAggregate`, `initializeArray`, `initializeList` | nested shape/count, member order, brace elision, string terminator/length inference, omitted elements. |
| B18 global/static init | `isConstantInitializer`, `initialize` | analyzer rejects non-constant static storage initializers using B's constant folding. |
| B19 calls/overloads | `checkCall`, `resolveCallee`, `functionCandidates`, `checkArguments`, `conversionRank` | arity/boundary/conversion, overload ranking, constructor candidates. |
| B20 defaults/variadics | `MinimumArity`, `registerMinimumArity`, `memberSignature` | trailing-default min-arity map; variadic accepted (scalar-only args, minimal promotions). |
| B21 object/construction | `checkPrimary` (`ThisNode`), `checkMember`, `checkCall` | receiver typing, `this`, const-member calls, object copy, ctor arguments. |
| B22 allocation | `checkBinary`/`checkUnary` unsupported-node path | `new`/`delete` are diagnosed unsupported (policy). |
| B23 builtin calls | A's `installLibrary` symbols + `checkCall` | ordinary signature checking; printf/scanf literal format checks not implemented (see gaps). |
| B24 regression checks | `semantic_audit/` + teammate A `run.py` | fixtures encode accept/reject by name; see §6. |

## 4. Files changed

- **New** `src/semantic/expressions.hh`, `src/semantic/expressions.cc` — the checker above.
- **B-owned parser fixes** (`src/parser/`):
  - `expressions.cc` — conditional middle operand parses assignment expression plus an optional
    comma list (unparenthesized comma in `a ? 2,3 : 4`); single-expression operands keep their
    original AST shape so existing golden ASTs stay stable.
  - `sizeof.cc` — expression operand parses an assignment expression plus an optional comma; fixes
    the single-expression comma wrapper that made `sizeof(f)` decay to a function pointer and slip
    past the function-type diagnostic.
  - `function.cc` — `...` now sets `Variadic` on `FunctionParameterList`; duplicate `static`/`const`
    specifiers diagnosed.
- **A-owned header request** (landed): `declaration.hh` `FunctionParameterList::setVariadic`,
  `ConstructorNode::getArgs/getBody`, `DestructorNode::getBody`.

## 5. Known gaps (policy-conditional, documented, not silently accepted)

- Variadic argument promotions are minimal (scalar only); no default-argument promotions beyond
  the checker's arithmetic conversions.
- No printf/scanf compile-time format checking (B23 optional part).
- VLA support is out of scope: bounds must be integer constant expressions.
- `new`/`delete` expression syntax/type checks are unsupported by policy (diagnosed).
- Warnings (missing return, unreachable, uninitialized, unused) are C's C21 items, not implemented.

## 6. Reproduce

```bash
cmake --build build --parallel 2
./custom_test_hook parse                     # 57 PASS
python3 semantic_audit/teammate_a/run.py     # {"checks":17,"passed":17}
python3 semantic_audit/run.py                # lex/parse snapshot, no crashes
for f in semantic_audit/programs/*.cpp semantic_audit/extended/programs/*.cpp; do
  ./build/baka --semantic "$f" >/dev/null 2>&1; done   # 0 crashes/timeouts
```

Fixtures whose name starts with `valid_` (or is an explicit positive control) must exit 0;
`wrong_`/`invalid_`/`undefined_`/`duplicate_`/`bad_`/`nonconstant_`/`nested_` fixtures must exit
non-zero with a `SEMANTIC` diagnostic.
