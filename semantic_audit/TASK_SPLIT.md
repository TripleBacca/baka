# Three parallel workstreams

Each person owns **24 work packages**. These group all **254 source checkboxes** from the main and extended checklists, including repeated/refined findings; each source checkbox has one primary owner in [TASK_OWNERSHIP.json](TASK_OWNERSHIP.json). Equal package counts are a planning balance, not a claim of identical hours. Optional features stay conditional on the chosen language policy.

| Person | Workstream | Packages | Primary output |
| --- | --- | ---: | --- |
| A | Types, names, declarations, and struct/class/union metadata | 24 | Resolved symbols/types, signatures, member tables, layout/lifetime queries |
| B | Expressions, calls, constants, and initialization | 24 | Typed expressions, explicit conversions, evaluated constants, initialization records |
| C | Parser/AST integration, statements, and control flow | 24 | Correct statement AST, semantic stage, resolved jump targets, integrated diagnostics |

Start by agreeing interface declarations and the policy questions already listed in the audit. Publish small usable interfaces early; nobody should wait for another person's full implementation. Each owner implements the regression checks for their own rules.

**Interfaces to agree first:**

- A provides `TypeId`, `SymbolId`, `ScopeId`, type constructors/queries, lookup/declaration services, member/special-member metadata, and layout queries.
- B provides expression checking with an explicit expected-type/evaluation-context parameter, conversion checking, typed constant evaluation, and initializer checking. Its result carries type, value category, resolved symbol, conversions, and optional constant value.
- C provides source spans, diagnostic reporting, statement/program traversal, current-function/class context, and loop/switch/label target IDs. These use A's IDs rather than an independent symbol/type system.
- A calls B's evaluator for dimensions/enumerators when needed; B calls A's lookup/layout services. Use forward-declared service interfaces and explicit deferred queries for incomplete layout, so this is not a mandatory whole-pass ordering or mutually owning data structures.
- C opens/closes A's scopes and supplies context to B. Function predeclaration must not accidentally make later local declarations visible early. Enum/bound evaluation must run with the correct declaration-time environment.
- Agree supported feature/version flags, error-result conventions, source-order rules, and whether typed data lives in AST fields or side tables. Unsupported service stubs should produce an explicit unavailable/error result, never a fabricated valid type or constant.

**File ownership to avoid merge conflicts:**

- A owns new type/symbol/declaration/layout modules, `src/base/symbol_table/`, and declaration/type/typedef/struct/class/union/enum parser files.
- B owns new expression/conversion/constant/initializer/call modules and expression/initializer/sizeof/function-parameter parsing in `expressions.cc`, `initializer.cc`, `sizeof.cc`, and `function.cc`.
- C owns new statement/control-flow/driver modules, loop/selection/label/jump/compound-statement parsing, driver/build wiring, and the shared audit runner.
- C coordinates the initial AST accessor/source-span patch. After that, A owns declaration/type/aggregate/enum AST headers (including `declaration.hh`), B owns expression/constant/cast/initializer/function AST headers, and C owns base/program/statement/control-flow AST headers. B requests parameter-list additions in A's `declaration.hh`; C requests source-location hooks in files owned by A/B. One owner lands each shared-header change.
- Cross-stream edits are small coordinated patches through the owning person. C integrates the pipeline; A/B remain responsible for their service behavior and fixes.


## Person A — types, names, and declarations

- [ ] **A01: Type and storage policy.** Record target widths, ranks, alignment, type identity, and the declaration/storage policy; publish assumptions used by the other streams.

- [ ] **A02: Semantic type model.** Implement builtin, pointer, array, function, qualified, enum, and struct/class/union types, plus an error type.

- [ ] **A03: Symbols and lexical scopes.** Implement stable symbol/scope IDs, current-scope versus outward lookup, namespaces, and legal shadowing without unstable hash-entry pointers.

- [ ] **A04: Name binding and declaration points.** Bind value references in source order, including declaration lists, self-initialization, and array-bound declaration-point cases.

- [ ] **A05: Parameter scopes.** Register parameters in the proper scope; reject duplicates and forbidden redeclarations in the outer function body.

- [ ] **A06: Redeclarations and linkage.** Track declarations versus definitions, reject conflicting kinds and repeated definitions, and allow compatible declarations without requiring external definitions locally.

- [ ] **A07: Typedef resolution.** Expand aliases with qualification and stable identity; reject incompatible replacements and preserve completion state.

- [ ] **A08: Type modifiers and specifiers.** Validate unsigned/type-kind/storage specifiers consistently; replace invalid-input assertions with diagnostics.

- [ ] **A09: Nested declarators.** Construct correct pointer/array/function binding, classify function versus object definitions, and reject illegal derived types.

- [ ] **A10: Completeness rules.** Distinguish prototypes from definitions and uses requiring storage; check array element completeness even below pointers.

- [ ] **A11: Function signature compatibility.** Normalize array/function parameters and top-level qualifiers, retain inner bounds/pointee const, and define overload identity and empty-list policy.

- [ ] **A12: Variably modified type restrictions.** Define VLA support and enforce structural scope, storage, linkage, member, and repeated-typedef restrictions; distinguish static pointers from static VLA objects.

- [ ] **A13: Aggregate identity and completion.** Track forward, being-defined, and complete states; reject duplicate definitions and recursive by-value containment, including anonymous type identities.

- [ ] **A14: Member tables and source order.** Register fields, methods, aliases, and special members with duplicate checks; preserve declaration order and provide ordered metadata.

- [ ] **A15: Inheritance.** Validate bases, completeness and cycles; provide inherited lookup, hiding rules, and base-subobject relationships.

- [ ] **A16: Layout service.** Compute size, alignment, padding, union size, base offsets, and field offsets; diagnose overflow and expose queries without evaluating expressions.

- [ ] **A17: Enums.** Retain explicit enumerator initializer syntax, register constants, detect name conflicts, and use B's constant evaluator for values and overflow checks.

- [ ] **A18: Constructors and destructors.** Preserve all declarations; validate signature sets/duplicates and determine generated special members and construction/destruction order.

- [ ] **A19: Static members and unions.** Retain union names; distinguish static/instance storage, union layout metadata, and supported nontrivial union member rules.

- [ ] **A20: Type/declaration parser repairs.** Fix type-state crashes, omitted bounds, and supported type spellings/parameter syntax in owned declaration/type parsers.

- [ ] **A21: Optional aggregate syntax.** Decide support for flexible arrays, bit-fields, and designated initialization; implement structural syntax/type restrictions when enabled, with B handling initializer interpretation.

- [ ] **A22: Library declarations.** Provide supported builtin/library symbols and prevent incompatible redeclarations; expose builtin identities to call checking.

- [ ] **A23: Member and implicit-object lookup.** Provide member lookup for dot/arrow, inherited members, this, receiver qualification, and static-context access; B supplies receiver types.

- [ ] **A24: Types/declarations regression checks.** Turn owned audit cases into diagnostic and valid-control checks, including typedefs, scopes, signatures, incomplete types, enums, inheritance, and crash regressions.



## Person B — expressions, calls, and initialization

- [ ] **B01: Conversion and value policy.** Record arithmetic/boolean/enum/pointer conversion rules and expression value categories, agreeing initialization and cast behavior with A.

- [ ] **B02: Typed expression output.** Implement expression result records, implicit conversion nodes/metadata, error propagation, and constant-result references for downstream checking.

- [ ] **B03: Literals and strings.** Type all literals, preserve numeric widths and decoded string lengths, and implement the chosen string constness rules.

- [ ] **B04: Value conversions and unevaluated contexts.** Apply lvalue reads and array/function decay in the correct contexts; type-check unevaluated operands without ordinary execution.

- [ ] **B05: Arithmetic, logical, and bitwise operators.** Check/promote arithmetic operands, integral-only operators, logical conditions, and short-circuit result types.

- [ ] **B06: Pointer arithmetic.** Validate pointer/integer arithmetic, complete object pointees, pointer differences, and result types.

- [ ] **B07: Comparisons, pointer conversions, and null.** Separate equality from ordering; check qualification, function/object/void pointers, eligible null constants, and enum conversion uses.

- [ ] **B08: Addressing and indexing.** Check address-of, dereference, and subscripting with correct element type and value category.

- [ ] **B09: Assignment and modifiability.** Check writable targets and destination conversions, recursive const fields, string/array restrictions, and valid aggregate copies containing arrays.

- [ ] **B10: Compound assignment and increment.** Check compound operators and prefix/postfix mutation, preserve single left-side evaluation, and compute correct result categories.

- [ ] **B11: Conditional and comma expressions.** Type conditional/comma results and preserve call nesting; fix parsing of an unparenthesized comma in the middle conditional operand.

- [ ] **B12: Casts.** Resolve cast target types through A, validate conversions and categories, and reject illegal array/function/unsupported aggregate casts.

- [ ] **B13: Sizeof expression semantics.** Use A's layout service; suppress decay for ordinary operands and represent evaluated VLA bounds/runtime sizes when supported.

- [ ] **B14: Constant expression evaluator.** Evaluate target-typed integral/address constants, distinguish valid constant expressions from foldability, and diagnose required-context failures with short-circuit handling.

- [ ] **B15: Array bounds and VLA evaluation.** Type/evaluate dimensions, enforce chosen numeric-bound rules, preserve runtime declaration-time extents, and check size overflow with A's structural type rules.

- [ ] **B16: Scalar initialization.** Preserve brace shape, validate scalar/const initialization and narrowing, and reject function initializers.

- [ ] **B17: Aggregate and string initialization.** Check nested shape, counts, member order, brace elision, string terminators, omitted elements/bounds, and any enabled designated initialization.

- [ ] **B18: Global/static initialization.** Validate constant/dynamic initialization policy and produce initialization records, including static zero initialization and runtime guards when supported.

- [ ] **B19: Direct/indirect calls and overload selection.** Check callable targets, argument boundaries, arity and conversions; rank supported overloads using A's signatures and distinguish constructor candidates.

- [ ] **B20: Defaults and variadics.** Retain variadic syntax; check default scope/types/redeclarations, apply trailing-default rules and variadic promotions, and expose minimum arity.

- [ ] **B21: Object expressions and construction calls.** Use A's lookup/special-member metadata to type member receivers, this, const member calls, object copying, and constructor arguments.

- [ ] **B22: Optional allocation.** Implement new/delete expression syntax and type checks only if chosen, or diagnose/document unsupported allocation; use A's layout and destruction metadata.

- [ ] **B23: Builtin call checks.** Apply ordinary signatures to library calls and add chosen literal printf/scanf format checks using builtin identity.

- [ ] **B24: Expressions/initialization regression checks.** Turn owned cases into checks for operators, values, calls/defaults, constants, sizeof, arrays/VLAs, initializers, and valid conversions.



## Person C — parsing, statements, and control flow

- [ ] **C01: AST access contract.** Publish visitor/getter and annotation access; coordinate header changes, preserving source-order and nested expression/label structure.

- [ ] **C02: Source locations and diagnostics.** Add source spans and semantic diagnostics, including previous-declaration notes and stable reporting interfaces.

- [ ] **C03: Driver and build wiring.** Keep the root alive, add the semantic stage/CLI/build integration, and stop on prior-stage errors.

- [ ] **C04: Traversal and semantic context.** Own statement/program dispatch and context stacks; connect A/B services and collect constants, layout references, initialization records, and resolved targets.

- [ ] **C05: Recovery and error boundaries.** Skip invalid nodes, contain malformed grammar recovery, and prevent cascades or invalid typed output from reaching lowering.

- [ ] **C06: Basic statement grammar.** Parse bare return, standalone empty statements, and empty bodies while preserving the distinction between syntax and semantic legality.

- [ ] **C07: For-header parsing.** Fix double delimiter consumption and shifted clauses; preserve all omitted-clause combinations and all five silently lost label cases.

- [ ] **C08: Loop suffix grammar.** Require separator plus identifier, reject empty/multiple/numeric suffixes, unify the type-name policy, and keep labels out of runtime expressions.

- [ ] **C09: Labeled statement ownership.** Attach goto/case/default labels to their statements; fix unbraced loop bodies, labeled do bodies, and dangling-else cases.

- [ ] **C10: General switch grammar.** Accept a general switch body statement, preserving nested label ownership and recovery.

- [ ] **C11: Function boundaries and returns.** Reset context on every function, enforce nested-definition policy, validate return placement/conversion through B, and apply chosen fall-through/main rules.

- [ ] **C12: Condition checking.** Visit if/else-if/while/do/for conditions and ask B to perform the chosen boolean/scalar conversion.

- [ ] **C13: For scope and empty clauses.** Enter/leave A scopes around for clauses/body, handle absent conditions/updates, and enforce the selected declaration-clause policy.

- [ ] **C14: Ordinary break and continue.** Require the right enclosing loop/switch and resolve each unlabeled target correctly.

- [ ] **C15: Named loop jumps.** Prebind trailing do labels, use an active loop stack, reject inactive/cross-function targets, and record correct outer-loop update/test/exit destinations.

- [ ] **C16: Label namespaces and collisions.** Define variable/type/goto/loop name interactions, duplicate active labels, and sibling reuse without confusing ordinary goto labels and loop suffixes.

- [ ] **C17: Goto resolution.** Collect function-wide labels, allow forward jumps, reject undefined/duplicate targets, and isolate function tables.

- [ ] **C18: Switch contexts.** Check controlling types using B, associate case/default nodes with the right switch, and isolate nested switches.

- [ ] **C19: Case values and defaults.** Use B's typed constant evaluator, compare case values after conversion, and reject duplicates/nonconstants/multiple defaults.

- [ ] **C20: Jump lifetimes and cleanup.** Check VM/initialization entry restrictions and record cleanup on return/break/continue/goto using A's lifetime metadata and B's initialization records.

- [ ] **C21: Chosen warnings and dataflow.** Add agreed missing-return/unreachable/uninitialized/unused/lifetime warnings without treating all runtime UB as mandatory compilation errors.

- [ ] **C22: Language policy integration.** Record supported versions/features and exclusions, choose label-before-declaration/block-end behavior, and coordinate policy decisions crossing module boundaries.

- [ ] **C23: Audit harness and ownership tracking.** Maintain shared runner/fixtures and the source-task ownership map; keep diagnostic failures distinct from crashes and policy-dependent cases.

- [ ] **C24: Pipeline integration checks.** Combine A/B/C services, verify no valid controls regress and no semantic errors reach lowering, and defer execution-only jump tests until code generation exists.



## Suggested integration sequence

1. Agree the shared interfaces and language policies, then publish IDs, AST access/source spans, and service declarations early.
2. Work in parallel: A starts scopes/declarators/signatures, B starts literals/operators/conversions against the type API, and C repairs the parser and implements loop/goto/switch contexts against the expression API.
3. Integrate narrow paths incrementally: one declaration plus assignment, one checked call, then one labeled loop. A/B resolve their layout/constant-query interactions while C expands traversal.
4. Complete aggregate initialization, class lifetimes, and remaining policy-dependent rules; run each owner's checks and the shared suite.
5. Add execution assertions when code generation exists. Parser acceptance alone never closes a semantic or runtime-control-flow task.

Detailed requirements remain in [the main checklist](CHECKLIST.md) and [the extended findings](extended/FINDINGS.md). The ownership JSON records every original source checkbox, line, and assigned package; duplicates are grouped into the same implementation work rather than treated as additional features.
