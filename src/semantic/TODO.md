# Semantic and IR contract checklist

This checklist records the implemented architecture.
Language-subset exclusions are policies, not claims of complete C++ coverage.

## Completed architecture work

- [x] Share target-width numeric conversions/evaluation between declaration constants,
  conversion metadata and IR folding; round floating operations at declared widths.
- [x] Convert const-object initializers before publishing declaration-time values.
- [x] Preserve conditional lvalue categories before contextual loads/decay.
- [x] Separate entity IDs from declaration positions and freeze call default visibility.
- [x] Use immutable structural type keys and borrowed completed-layout views.
- [x] Use typed AST/default/source pointers in declaration and diagnostic services.
- [x] Make mutating declaration/aggregate adapters accept mutable AST inputs.
- [x] Require AST child traversal and explicit expression-edge enumeration.
- [x] Retain selected constructor conversions during checking instead of reselecting
  while materializing conversion nodes.
- [x] Publish checked-program views that enforce AST/analyzer identity.
- [x] Publish initialization, call-argument, lifetime and ordered jump-cleanup plans.
- [x] Diagnose unavailable/ambiguous implicit construction and lifetime-entry jumps
  before IR generation; avoid emitting unrequested unusable implicit constructors.
- [x] Emit expressions directly within structured statement lowering and reset a
  complete function context instead of clearing overlapping builder state lists.
- [x] Give calls owned argument operands and provide instruction builders.
- [x] Index function locals, imported type identities and IR object/function lookup.
- [x] Use bulk zeroing and loop-based array lifetimes for bounded generated code.
- [x] Validate malformed IR operands/type graphs without unchecked type-table access.
- [x] Check representative semantic diagnostics and optimized/unoptimized execution;
  poison automatic test storage and retain TAC output fixtures.
- [x] Replace obsolete IR-readiness documentation with current contracts.

## Deliberately separate future capabilities

- A native backend, platform calling convention, symbol ABI and selectable targets.
- Exceptions/unwinding, concurrent/reentrant static initialization and nontrivial unions.
- Additional grammar such as local aggregate definitions, references and allocation.
- Full C++ overload ranking and context-uniform user-defined conversions.
- Warning/dataflow analyses, including unused declarations and all-paths-return checks.
- A separate immutable typed syntax tree if future language growth warrants migration;
  the current parser AST is normalized in place and paired with typed execution plans.

Run the commands in [README.md](README.md) before changing any contract above.
