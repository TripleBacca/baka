# context_C.md — Teammate C handoff (pipeline, statements, control flow)

Branch: `saturn/slop`. Build: `cmake -S src -B build && cmake --build build --parallel 2`.
C implements the 24 C packages in [TASK_SPLIT.md](semantic_audit/TASK_SPLIT.md): the semantic
stage/driver, program/statement traversal, control-flow targets, and the C-owned parser repairs.

## 1. Summary

`SemanticAnalyzer` (`src/semantic/semantic_analyzer.{hh,cc}`) owns A's services
(`Types`, `Symbols`, `Declarations`, `ASTDeclarations`, `ASTAggregates`) and B's
`ExpressionChecker`, walks the program, opens/closes A's scopes, records initialization/jump
state, and reports through A's `Symbols::diagnose`. The driver runs this as a new `SEMANTIC`
stage selected by `--semantic`, rendering diagnostics with source spans.

Verified state: `./custom_test_hook parse` **57/57**, `semantic_audit/teammate_a/run.py` **17/17**,
`semantic_audit/run.py` no crashes (lex 113, parse 96/17), and all 221 audit fixtures classified
with no crashes or timeouts.

## 2. Driver / stage integration (C03)

- `src/types/driver/defs.hh` — `Stage { LEX=0, PARSE=1, SEMANTIC=2, CODEGEN=3 }` and `to_string_view`.
- `src/driver/args.cc` — `--semantic` sets `UptoStage = Stage::SEMANTIC`.
- `src/driver/driver.cc` — after parsing, if `TillStage() >= Stage::SEMANTIC`, constructs
  `SemanticAnalyzer`, calls `analyze(*Program)`, and for every diagnostic casts
  `Entry.source`/`Entry.previous` (opaque `const void*`) back to `types::ASTNode*`, reads
  `getSourceSpan()`, and emits `Gctx::GenerateLineError(...)` / `GenerateLineWarning("previous
  declaration is here")` at `Stage::SEMANTIC`. If analysis fails with no diagnostic, a global
  error is emitted so silent failures cannot pass. The default stage stays `PARSE`, so parser and
  audit behavior is unchanged unless `--semantic` is requested.
- `src/semantic/CMakeLists.txt` — `add_library(semantic ...)` compiling all `src/semantic/*.cc`,
  linking `base`/`parser`; `src/CMakeLists.txt` adds the subdirectory; `src/driver/CMakeLists.txt`
  links `semantic`.

## 3. Traversal and context (C01, C04, C05, C11)

`SemanticAnalyzer` constructor order: `Types`, `Symbols`, `Declarations`, `ASTDeclarations(nullptr)`,
`ExpressionChecker`, `ASTAggregates`, then `ASTResolver.setEvaluator(&Checker)` and
`installTypes`/`installLibrary`.

Traverses: program/declaration lists (with per-declarator initializer callback), typedefs, enums,
struct/class forwards and definitions, unions, and functions (prototype + definition). Aggregate
bodies use class context for methods/ctors/dtors. Functions enter a `Function` scope, register
parameters, evaluate default arguments, set the return/class/static/const context, clear per-function
jump/label/initialization state, then analyze the body; state is restored and the scope trimmed on
exit. Error boundaries: failed nodes set `Failed` and stop the subtree; every failure path emits a
diagnostic (explicit malformed-declarator/function-body diagnostics plus the driver's global-error
fallback).

## 4. Statements and control flow (C06–C20)

- Statements: compound (nested block scopes), expression, empty, if/else-if/else, switch, for,
  while, do-while, return, goto, break, continue, labels, case/default.
- `for` opens a `For` scope, handles expression / declaration-list / typedef initializers, absent
  condition/update, and rejects function declarations in the initializer.
- Conditions (`while`/`do`/`for`/`if`) go through B's `checkCondition` (scalar conversion).
- `return` checks void vs value and B convertibility; `break`/`continue` track loop/switch depth.
- Named loop jumps (`break L`/`continue L`) resolve against an active loop-label stack; trailing
  `do ... while(cond; L)` labels are prebound before the body. Duplicate active labels, inactive/
  undefined targets, and `break`/`continue` with no loop are diagnosed.
- `goto` collects function-wide labels (`PendingGotos`), allows forward jumps, rejects
  undefined/duplicate targets, and keeps functions isolated. A jump-bypasses-initialization pass
  records the initialized-symbol set at each label and each goto and rejects a goto that would skip
  a live initialization.
- `switch` checks the controlling type with B, tracks nested switch contexts (isolated), and
  `case`/`default` use B's `integer()` evaluator with duplicate-case/multiple-default checks.
- `main` is validated: `int`-like return, and either no parameters or `(int, char**)`-compatible.

## 5. C-owned parser repairs (C06–C10)

- `statement.cc`/`compoundStatement.cc` — bare `return`, standalone empty statements, empty bodies.
- `iterationStatement.cc` — for-header clause delimiters consumed once; all omitted-clause
  combinations and all five previously dropped label positions preserved; loop suffix requires a
  separator plus an identifier; empty/numeric/multiple suffixes rejected; one type-name collision
  policy across loop kinds; labels kept out of runtime expression clauses.
- `labelStatement.cc`/`selectionStatement.cc`/`jumpStatement.cc` — labeled statements own their
  statement (including unbraced loop bodies, labeled `do` bodies, and dangling-else); general
  switch body statement; goto/case/default label ownership.
- `expressions.cc` — unparenthesized comma in the middle conditional operand.
- `declaration.cc` — `struct X`/`class X` used where `X` names a non-struct type is diagnosed.
- `function.cc` — variadic marker and duplicate-specifier diagnostics (shared with B).

## 6. C01–C24 coverage

| Package | Where |
| --- | --- |
| C01 AST access/source spans | `ASTSourceSpan`, node `getSourceSpan()`, C-owned AST headers |
| C02 source locations/diagnostics | `driver.cc` span rendering, previous-declaration notes |
| C03 driver/build wiring | `defs.hh`, `args.cc`, `driver.cc`, `semantic/CMakeLists.txt` |
| C04 traversal/context | `SemanticAnalyzer` + context stacks |
| C05 recovery/error boundaries | `Failed` gating, explicit diagnostics, global-error fallback |
| C06 basic statement grammar | `statement.cc`, `compoundStatement.cc` |
| C07 for-header parsing | `iterationStatement.cc` |
| C08 loop suffix grammar | `iterationStatement.cc` |
| C09 labeled statement ownership | `labelStatement.cc`, `selectionStatement.cc` |
| C10 general switch grammar | `selectionStatement.cc` |
| C11 function boundaries/returns | `analyzeFunction`, `analyzeConstructor`, `analyzeDestructor` |
| C12 condition checking | `checkCondition` calls |
| C13 for scope/empty clauses | `analyzeStatement` (for branch) |
| C14 ordinary break/continue | depth counters in `analyzeStatement` |
| C15 named loop jumps | `LoopLabels`, trailing-do prebind, `noteLoopLabel` |
| C16 label namespaces/collisions | `FunctionLabels`, `LoopLabels`, `noteLoopLabel` |
| C17 goto resolution | `PendingGotos`, `JumpTargets`, `JumpSources` |
| C18 switch contexts | `Switches` stack |
| C19 case values/defaults | B's `integer`, duplicate/multiple-default checks |
| C20 jump lifetimes/cleanup | `Initialized`, `trimInitialized`, bypass pass |
| C21 warnings/dataflow | not implemented (see gaps) |
| C22 language policy integration | documented policy choices below |
| C23 audit harness/ownership | `semantic_audit/` runner + fixtures (do not commit audit fixtures) |
| C24 pipeline integration checks | parse hook + teammate A `run.py` + audit classification |

## 7. Known gaps (policy-conditional, documented, not silently accepted)

- Warnings (C21): missing return, unreachable code, uninitialized/unused, lifetime warnings are
  not emitted; the language policy treats them as warnings, not errors.
- VLA/`new`/`delete` are unsupported by policy and diagnosed rather than accepted.
- Loop-label namespace decisions: active duplicate loop labels are errors; sibling reuse after exit
  is allowed; ordinary goto labels and loop suffixes are separate namespaces.
- Labels before declarations and at block ends are accepted. Empty brace initialization is accepted.
- Long-form type spellings such as `long long` remain a parser decision (not enabled); C11-style
  for-clause storage restrictions are not applied.
- Execution-only jump tests (outer-for continue, outer-do continue, named break through a switch)
  are deferred until code generation exists.

## 8. Reproduce

```bash
cmake --build build --parallel 2
./custom_test_hook parse                     # 57 PASS
python3 semantic_audit/teammate_a/run.py     # {"checks":17,"passed":17}
python3 semantic_audit/run.py                # lex/parse snapshot, no crashes
for f in semantic_audit/programs/*.cpp semantic_audit/extended/programs/*.cpp; do
  ./build/baka --semantic "$f" >/dev/null 2>&1; done   # 0 crashes/timeouts
```
