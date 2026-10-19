# Semantic analysis: grouped implementation and review checklist

Unchecked entries are acceptance work, not a claim that no corresponding code
exists. Check an entry only after reviewing all relevant contexts and running its
positive and negative controls. Checked regression entries below refer to the
155 ordinary fixtures, 32 diagnostic fixtures, eleven phase scenarios and six conversion scenarios plus a member-call scenario documented in README; they do
not imply coverage of every possible variant. [README.md](README.md) defines the pass contracts.

The four public phase methods and the five work queues are implemented. Source
inspection confirms AST metadata, owned resolved names, persistent scopes and a
const final checker. The checklist below deliberately retains broad behavioral
items until regression evidence covers their relevant contexts; it is not a
percentage-complete report.

## Implemented architecture (source inspection)

- [x] Public `registerDeclarations`, `resolveAndRename`, `annotateTypes`, `check` and `analyze` API with `SemanticPhase` transitions.
- [x] Separate `registration.cc`, `resolution.cc`, `annotation.cc` and `checks.cc`, following the parser's split-file organization.
- [x] `TopLevel`, `Bodies`, `Expressions`, `Flows` and `Switches` queues with saved contexts and cached results.
- [x] `ASTSemanticInfo`, identifier binding IDs and owned resolved names; source spelling remains available.
- [x] Persistent scope arenas and context selection; resolved expression names consume saved bindings.
- [x] Const `checkReadOnly()` consumes annotations/work results without registration or expression evaluation.
- [x] Document declaration-time bounds/enumerators evaluation, syntax-only omitted-array shape inference and intrinsic diagnostics during annotation.

## Pipeline acceptance and shared metadata

- [x] Execute declaration registration, mutating resolution/rename, mutating type annotation, then read-only checks as distinct phases.
- [x] Collect ordered declaration/body/initializer work items during discovery; avoid repeated program-root searches.
- [ ] Preserve declaration-point visibility when declarations are indexed ahead of use.
- [x] Resolve type names in their current lexical symbol table and attach the resulting metadata to the AST.
- [x] Attach stable binding identity and resolved type/value-category metadata; preserve source spelling and locations for diagnostics.
- [x] Ensure the final checks consume annotations without mutating the AST, symbol definitions or type arena.
- [x] Document arena lifetimes, error results, recovery behavior and phase prerequisites.

## Registration and name resolution

- [ ] Reject undeclared/out-of-scope identifiers and use before declaration; permit legitimate inner-block shadowing and recursion.
- [ ] Reject duplicate locals/parameters, conflicting object/function declarations, incompatible prototypes and repeated function definitions.
- [ ] Resolve typedefs; accept compatible repeats and reject incompatible repeats or aggregate-name conflicts.
- [ ] Register aggregate members, enum constants and callable signatures without discarding duplicate declarations.
- [ ] Register `this` in instance members and reject it in static methods or outside an instance context.
- [ ] Resolve goto labels function-wide and named-loop targets only while the target loop is active.
- [ ] Reject nested function definitions deliberately.

## Type annotation and declaration rules

- [ ] Build nested pointer/array/function types correctly; reject arrays of void and direct array/function return types.
- [ ] Reject void objects, incomplete by-value objects/members and recursive by-value aggregates; permit pointers to incomplete/self types.
- [ ] Reject object declarators with function bodies.
- [ ] Check aggregate-kind modifiers and unsigned modifiers after typedef resolution without assertions.
- [ ] Check duplicate storage specifiers for definitions and ordinary declarations consistently.
- [ ] Require integral nonnegative valid array bounds; infer supported omitted bounds from initializers.
- [ ] Record variadic signatures, parameter adjustments and declaration-specific default arguments.
- [ ] Propagate const through aliases, aggregate receivers and recursively const members.
- [ ] Evaluate explicit/implicit enumerator values with correct declaration-point visibility and range checks.

## Expression and conversion checks

- [ ] Reject assignment/mutation of constants, non-lvalues, arrays and const-qualified targets.
- [ ] Validate dereference, address-of and indexing operands and result categories.
- [ ] Validate arithmetic, shifts, bitwise, logical and comparison operands, including pointer arithmetic and pointer multiplication rejection.
- [ ] Apply promotions and implicit conversions consistently, including null pointer constants and qualifier preservation.
- [ ] Require scalar non-void conditions.
- [ ] Validate casts; reject array destinations and apply the documented aggregate-cast policy.
- [ ] Validate `sizeof`, including void/functions/incomplete objects and pointers to incomplete types.
- [ ] Resolve member lookup/inheritance, nonexistent members and `.`/`->` receiver requirements.

## Calls, returns and initialization

- [ ] Reject non-callable targets; validate function pointers and selected callable candidates.
- [ ] Check required/excess argument counts, variadics, defaults and argument conversions.
- [x] Keep defaults associated with declarations/symbols: unrelated functions sharing an interned signature must not share their required argument count.
- [ ] Validate default expressions in their declaration environment and check conversion to the parameter type.
- [ ] Preserve the distinction between comma-separated arguments and a parenthesized comma-expression argument.
- [ ] Validate bare/value returns against the containing function's result type.
- [ ] Check scalar/aggregate/string initializers, excess array elements and inferred dimensions.
- [ ] Apply the selected const-initialization and static/global initialization rules in every context, including `for` declarations.
- [ ] Diagnose duplicate member names, enumerators, constructor signatures and destructors.

## Read-only control-flow checks

- [ ] Validate ordinary break/continue against enclosing loops/switches.
- [ ] Validate undefined, expired and duplicate active loop labels according to the documented policy.
- [ ] Bind trailing do-while labels before analyzing/checking their bodies.
- [ ] Diagnose duplicate/undefined goto labels and prohibited jumps over initialization.
- [ ] Require integral switch conditions and integral constant cases.
- [ ] Detect duplicate cases after conversion to the promoted switch type and multiple defaults per switch.
- [ ] Reject case/default outside switches; preserve nested-switch independence.

## Parser/AST prerequisites and diagnostics

- [ ] Parse bare `return;` and standalone empty statements.
- [ ] Preserve omitted array bounds, explicit enum values, variadic markers and union names.
- [ ] Preserve every constructor/destructor declaration for duplicate checking.
- [ ] Preserve labeled-loop header/body structure across `for`, `while` and `do-while` forms.
- [ ] Replace invalid unsigned-type assertions and conflicting enum/struct exceptions with source-located diagnostics.
- [ ] Reject unsupported syntax predictably rather than silently dropping AST information.

## Language policy acceptance checks

- [ ] Verify the documented no-VLA, positive-constant-bound and inferred/adjusted omitted-bound policy in every declaration context.
- [ ] Verify the documented constant global/local-static initialization policy, including automatic-local addresses.
- [ ] Verify const scalar initialization requirements and the accepted aggregate/constructor behavior.
- [ ] Cast/implicit conversion rules and permitted aggregate conversions.
- [ ] Verify forward function calls, source-order global object visibility and local declaration-point lookup.
- [ ] Duplicate active loop labels, loop/goto namespaces and goto bypassing initialization.
- [ ] Verify unsupported `long long` declarations and `main(void)` are diagnosed; keep parameters named.
- [ ] Verify unsupported `new`/`delete` syntax is diagnosed despite keyword recognition.

## Regression coverage

- [x] Add focused custom tests for each of the four phases, including assertions on binding/type metadata.
- [x] Verify final checks do not mutate semantic annotations, symbol definitions or types.
- [x] Assert diagnostics and nonzero status for invalid cases; assert no crashes.
- [x] Retain valid recursion, shadowing, compatible typedef repeats and forward goto.
- [x] Retain valid self/incomplete-type pointers and `sizeof` of their pointers.
- [x] Retain valid function pointers, members, inheritance and default arguments.
- [x] Retain valid labeled break/continue and empty `for` clauses.
- [x] Test comma argument nesting, declaration-order visibility and defaults on unrelated same-signature functions.
- [ ] Reuse confirmed audit examples as regressions when the corresponding rule is accepted.

## Review findings from the implementation before this revision

These describe the reviewed baseline, not a permanent statement about the current
code. Keep the regression after fixing each defect.

- [x] Replace the single interleaved traversal with explicit phase orchestration and recorded work/context (source inspection; structural regression remains in the coverage section).
- [x] Replace transient expression results with persistent AST binding/type annotations (source inspection).
- [x] Remove default-argument required-count leakage through canonical function-type IDs.
- [x] Run initializer and const/static-storage checks for `for` declaration initializers, using the same rules as ordinary declarations.
- [x] Treat `main` like any other function; remove special signature restrictions while retaining ordinary function/return checks.


## Explicitly outside this implementation

- Warning-only analyses such as unused declarations and unreachable code.
- Cleanup/destruction lowering and code generation.
- Full C/C++ standard coverage beyond the documented language subset.


## Remaining concrete limitations

- [ ] Add parser support for local aggregate definitions before enabling the excluded legacy `valid_aggregate_shadow` control.
- [ ] Broaden arithmetic constant evaluation deliberately; current passing fixtures do not establish full standard coverage of all formats and arithmetic edge cases.
- [ ] Expand final-pass immutability tests beyond sampled metadata and arena counts if mutable service interfaces are introduced later.


## Array-shape regression batch

- [x] Infer omitted outer array bounds from initializer shape during registration/resolution before later `sizeof` and bound dependencies.
- [x] Retain controls for braced character strings, scalar braces and flat brace-elided multidimensional inferred arrays.
- [x] Reject an empty initializer for an unsized array rather than leaving an apparently complete object.
- [x] Keep initializer element typing/conversion in annotation; early shape inference is not general expression annotation.


## Implicit conversion AST and IR boundary

- [x] Insert explicit load, decay, promotion, numeric, boolean and pointer conversion nodes during annotation.
- [x] Rewrite owning AST slots for initializers, arguments/defaults, returns, conditions, switches and expression operands.
- [x] Preserve assignment/address targets, unevaluated array operands and argument nesting.
- [x] Represent compound assignment computation with a saved target address and explicit read/store conversions.
- [x] Persist selected call signatures and aggregate member/constructor identities independently of checker caches.
- [x] Add structural tests for conversions, single target evaluation, persistent call/member facts and final-pass immutability.
- [x] Review the IR input contract and record the remaining integration/normalization work in [IR_READINESS.md](IR_READINESS.md).
- [ ] Keep semantic arenas alive in the driver across future IR generation.
- [ ] Normalize aggregate initialization and per-call defaults before lowering those features without semantic reconstruction.
- [ ] Expose cleanup/construction plans before lowering nontrivial object lifetimes and jumps.

- [x] Print resolved control target IDs and mirror named jump bindings onto their label identifiers; test nested `for` targets across switches and later passes.


## Diagnostic locations and independent-error recovery

- [x] Require explicit token anchors for every parser AST allocation; capture statement/declaration starts before descending into children.
- [x] Anchor identifiers/literals at their tokens and binary/postfix expressions at their operators.
- [x] Point switch/case/condition/default/return conversion diagnostics at offending expressions and duplicate loop labels at their label identifiers.
- [x] Retain accurate previous-declaration notes; add multiline tests checking line, column and displayed source text.
- [x] Continue after recoverable local resolution/type errors so later invalid calls and errors in other functions are reported.
- [x] Suppress unresolved-expression cascades and keep conversion rewriting/IR eligibility gated on successful analysis.


## Bound member-function calls

- [x] Keep instance-method callees bound to their receiver; do not decay them to ordinary function pointers.
- [x] Preserve ordinary decay for free/static functions and pointer loads for indirect calls, including function-pointer fields.
- [x] Preserve selected member/signature metadata through parentheses, implicit calls and dot/arrow receivers.
- [x] Reject bound-method value/address conversions without pointer-to-member support; require an object for implicit instance calls in static contexts.
- [x] Add independent structural member-call tests and source acceptance/rejection fixtures.
