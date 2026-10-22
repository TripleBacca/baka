# Semantic analysis implementation checklist

This checklist is based on the source in this repository and small programs written specifically for this audit. It covers baka's documented C++-like feature set, including custom loop labels. It is not a claim that baka implements all of C or C++. Items marked **policy** require a language rule; the analyzer must implement that rule consistently. Everything unchecked below is work to include, not an assertion that a runtime probe exercised every combination.

The driver currently stops after parsing. No semantic pass is wired in. Successful `--parse` means an AST was produced without a reported parser error; it does not mean the source is a valid program.

## 1. Language rules to settle before implementing checks

- [ ] Specify primitive type widths, ranks, signedness, floating types, alignment, pointer size, and the type returned by `sizeof` for the target.
- [ ] Specify permitted implicit arithmetic, boolean, enum, pointer, `void*`, and inheritance conversions. C and C++ differ here.
- [ ] Specify value categories and the result categories of assignment, comma, conditional, casts, and prefix/postfix increment. C-based tests cannot automatically define these for a C++-like language.
- [ ] Specify whether function and constructor overloading are supported; otherwise diagnose conflicting same-name declarations/signatures.
- [ ] Specify array rules: constant bounds only versus VLAs, zero-length arrays, unspecified bounds, and parameter adjustment.
- [ ] Specify file-scope declaration/definition rules: C tentative definitions versus C++-like definitions, and internal linkage via `static`.
- [ ] Specify constant versus dynamic initialization for globals and local statics, including initialization order and guards if dynamic initialization is supported.
- [ ] Specify const initialization, aggregate initialization, brace narrowing, enum conversion, and string literal type rules.
- [ ] Specify ordinary-name/type-name/tag conflicts and shadowing. The current parser prohibits declarator names matching any visible type name, which already constrains this policy.
- [ ] Specify loop-label visibility, reuse, and duplicate active labels. A named break/continue should target an active enclosing loop.
- [ ] Specify jumps across initialization and object lifetimes; decide diagnostic severity for reaching the end of a non-void function.
- [ ] Specify whether nested function definitions, `new`/`delete`, and special member generation are supported. Reject unsupported constructs deliberately.
- [ ] Specify whether executable entry-point validation is required. Library compilation must not require a `main` definition.

## 2. Semantic pass infrastructure and output

- [ ] Add a semantic stage and CLI stop option to the driver stage enum, argument handling, diagnostics, and build targets.
- [ ] Keep the AST root available after the parsing block; run semantics only after successful lexing/parsing.
- [ ] Provide AST child/operator/value accessors or a visitor interface. Most nodes currently expose only `Print()`.
- [ ] Attach source spans to AST nodes, declarations, identifiers, and operators for diagnostics.
- [ ] Introduce semantic types for builtins, qualified types, pointers, arrays, functions, aggregates, enums, and the null-pointer literal.
- [ ] Assign stable identities to types, symbols, scopes, functions, and control-flow targets. Same spelling does not make two distinct structs/enums equal.
- [ ] Introduce symbols with kind, type, declaring scope, source location, storage duration/linkage, definition state, and member ownership.
- [ ] Add current-scope lookup separately from outward lookup. Same-scope duplicate checks must not prohibit legal shadowing.
- [ ] Make user-caused declaration conflicts produce diagnostics rather than `assert(false)` or uncaught exceptions in the symbol table/AST constructors.
- [ ] Avoid retaining pointers to hash-table entries across insertion/rehashing; the existing symbol-table header notes pointer instability.
- [ ] Annotate identifier references with their resolved symbols and expressions with types and value categories.
- [ ] Represent implicit conversions explicitly in a typed AST/IR or equivalent metadata so code generation sees the checked conversions.
- [ ] Record evaluated constants, finalized array bounds, aggregate layouts, field offsets, and resolved jump targets for later stages.
- [ ] Maintain current function/return type, class/implicit object, lexical scopes, enclosing loops, and enclosing switches independently.
- [ ] Use an error type/symbol and suppress dependent duplicate diagnostics. Never pass invalid typed nodes to code generation.
- [ ] Preserve source order where declaration visibility and initialization depend on it. Member predeclaration for method lookup and source-ordered initialization are different needs.

## 3. Name binding, scope, and declarations

- [ ] Resolve every value identifier; reject undeclared uses, uses before the permitted declaration point, and uses after leaving scope.
- [ ] Register each declarator at the correct point, including declaration lists such as `int a=1,b=a;`; resolve self-initialization under the chosen language rules.
- [ ] Apply global, function parameter, function-body, nested block, `for` initializer, class, and label scope rules.
- [ ] Make parameters visible in the function body and reject duplicates or prohibited redeclarations in the function's outermost block.
- [ ] Allow legal inner-scope shadowing without confusing distinct objects or types.
- [ ] Reject duplicate local objects and conflicting symbol kinds, such as a function and object with the same conflicting name.
- [ ] Distinguish declarations from definitions; allow compatible prototypes and permitted redeclarations, reject multiple definitions.
- [ ] Compare complete declaration types, qualifiers, array dimensions, parameter types, return types, and linkage as required.
- [ ] Give function-local labels function-wide visibility, including forward goto targets, independently of block-local object lookup.
- [ ] Keep each function's label and jump tables isolated. A goto cannot target another function's label.
- [ ] Resolve type names, aliases, enum constants, and aggregate members through the appropriate namespaces under baka's chosen rules.
- [ ] Register library functions through declarations or a deliberate builtin table. `printf`, `scanf`, and file functions are not automatically known merely because the README lists them.
- [ ] If reserved builtins have special rules, prevent incompatible redeclarations and record the actual builtin identity rather than recognizing calls by spelling alone.

## 4. Typedefs, type specifiers, and declarator construction

- [ ] Expand aliases while preserving type identity and qualification; reject incompatible typedef redefinitions.
- [ ] Keep aliases distinct from their underlying aggregate identity; redeclaring an alias must not overwrite an aggregate's completion state.
- [ ] Check `struct`/`class`/`union`/`enum` modifiers against the named type's actual category under the language's tag rules.
- [ ] Validate `unsigned` against permitted primitive spellings/types and the language's alias rules; do not crash on invalid input.
- [ ] Validate duplicate/conflicting specifiers consistently in declarations, definitions, parameters, casts, typedefs, and `sizeof` type operands.
- [ ] Interpret qualifiers on the type, not merely the declaration text; propagate const through typedefs, dereference, and aggregate member access.
- [ ] Build pointer/array/function types in declarator binding order, preserving parentheses. `int *a[2]` and `int (*a)[2]` differ.
- [ ] Distinguish function declarations from function-pointer variables, including nested function-pointer return types.
- [ ] Reject arrays of functions, arrays of void, functions returning functions, and functions returning arrays directly, including invalid nested types.
- [ ] Allow pointers to functions, pointers to arrays, and permitted pointers to incomplete types; do not reject them merely because a related by-value type is invalid.
- [ ] Reject plain void objects, void members, and void parameters except a separately supported void-parameter-list marker.
- [ ] Require complete types where storage, by-value arguments/results, member access, or size calculations require them.
- [ ] Detect recursive by-value aggregate layouts, including indirect cycles through arrays/members; allow recursive pointers.
- [ ] Validate legal storage specifiers by declaration context; distinguish static globals, locals, members, and functions.
- [ ] Validate that a function definition's resolved declarator actually denotes a function. The parser currently accepts an object-like declarator followed by a body.

## 5. Literals and expression typing

- [ ] Assign correct types and values to integer, unsigned, long, floating, character, boolean, string, and `nullptr` literals retained by the lexer.
- [ ] Preserve numeric signedness/width and decoded string length, including escapes, embedded zero bytes, and the terminator used for array sizing.
- [ ] Treat string literals as arrays with the language's chosen element qualification; reject direct writes regardless of their addressability.
- [ ] Distinguish objects, function designators, arrays, ordinary values, and null-pointer values.
- [ ] Apply lvalue-to-value, array-to-pointer, and function-to-pointer conversions only in the appropriate contexts.
- [ ] Suppress array/function decay for contexts such as address-of and `sizeof` where required.
- [ ] Apply integer promotions and usual arithmetic conversions before choosing operator result types.
- [ ] Reject use of void results where a value is required; allow valid discarded-value contexts and supported void expressions.
- [ ] Determine the type and value category of every expression, including single-element comma wrappers produced by the parser.

## 6. Operators, assignment, and pointers

- [ ] Unary `+`/`-`: require arithmetic operands and apply promotions; reject aggregates and pointers.
- [ ] `~`: require an integral operand and apply promotions.
- [ ] `!`, `&&`, `||`: require permitted condition conversions and produce the specified boolean/result type; retain short-circuit behavior for lowering and constant evaluation.
- [ ] `*`, `/`: require arithmetic operands. `%`, shifts, and bitwise binary operators require integral operands.
- [ ] `+`/`-`: distinguish arithmetic operations, pointer-plus/minus-integer, and compatible pointer subtraction; reject pointer addition and integer-minus-pointer.
- [ ] Require complete object pointee types for pointer arithmetic, indexing, and pointer increments; reject void/function-pointer arithmetic unless explicitly supported as an extension.
- [ ] Pointer subtraction: validate compatible element types and choose the target's pointer-difference result type.
- [ ] Comparisons: check arithmetic conversions or permitted pointer/null/enum comparisons; distinguish equality from ordering constraints.
- [ ] Reject implicit conversions that discard pointee constness, including unsafe multilevel-pointer qualification conversions.
- [ ] Validate object-pointer, function-pointer, and `void*` conversions separately; equal storage size is not type compatibility.
- [ ] Unary dereference: require a suitable pointer/function-pointer operand and assign the correct resulting category/type.
- [ ] Address-of: require an addressable object or function as allowed; preserve array type in pointer-to-array results.
- [ ] Indexing: require pointer/array plus integral index, supporting `i[p]` only if part of the language; produce the element type/category.
- [ ] Plain assignment: require a modifiable object target and an allowed conversion to the destination type.
- [ ] Reject assignment to constants, whole arrays, functions, string literals, and non-modifiable expression results.
- [ ] Compound assignment: validate both the underlying operation and conversion back to the target type; preserve single evaluation of its left operand.
- [ ] Prefix/postfix increment/decrement: require a modifiable arithmetic object or appropriate pointer, subject to the language's bool rule; assign their different result categories.
- [ ] Conditional `?:`: validate the condition, unify branch types/qualifiers, handle permitted void/aggregate/pointer/null combinations, and compute the result category.
- [ ] Comma expressions: type-check each operand, use the final operand's type/category according to policy, and preserve sequencing.
- [ ] Reject aggregate operators unless aggregate assignment, comparison, or overloads are explicitly supported.

## 7. Casts and sizeof

- [ ] Resolve the cast type plus abstract declarator; validate the target as a legal type.
- [ ] Validate arithmetic, enum, pointer, integer-pointer, inheritance, qualification, and void casts under explicit language rules.
- [ ] Reject illegal casts to array/function types and unsupported aggregate casts.
- [ ] Do not make a cast result assignable/addressable merely because its operand is.
- [ ] `sizeof(type)` and `sizeof(expr)`: require a permitted complete sized type; reject functions, void, and incomplete by-value types.
- [ ] Type-check `sizeof` expressions without ordinary decay; non-VLA operands are unevaluated. If VLAs are supported, handle their evaluated bounds and runtime sizes as specified in the [extended review](extended/FINDINGS.md).
- [ ] Permit `sizeof` of a pointer to an incomplete type; its pointee need not be complete.
- [ ] Compute sizes/alignment from target types, including array bounds, aggregate padding, union size, and inheritance.
- [ ] Produce a representable size value and diagnose type/array layouts that exceed implementation limits.

## 8. Constant expressions and array bounds

- [ ] Provide a typed constant evaluator separate from ordinary expression type checking.
- [ ] Define allowed operators, casts, literals, enumerators, constant objects, and `sizeof` in integral/static-address constant expressions.
- [ ] Reject nonconstant expressions where constants are required, rather than accepting an arbitrary variable expression.
- [ ] Evaluate with target widths/signedness and the same conversions as ordinary expressions.
- [ ] Diagnose division/remainder by zero, invalid shifts, and invalid overflow in required constant expressions according to the language rules.
- [ ] Respect short-circuit/conditional evaluation; do not report invalid evaluation of an unselected operand as if it ran.
- [ ] Check integral array bounds, positivity/zero-bound policy, constant/VLA context rules, and total allocation-size overflow.
- [ ] Infer omitted outer bounds from initializers where supported; require known inner dimensions for multidimensional arrays as appropriate.
- [ ] Preserve inner dimensions when adjusting an array parameter to a pointer.
- [ ] Support permitted constant address initializers, null pointers, and symbol-plus-offset values without inventing runtime values.

## 9. Initialization and storage

- [ ] Distinguish no initializer, scalar expression, empty braces, and nested brace lists. Preserve syntax needed by narrowing and aggregate rules.
- [ ] Check scalar assignment conversions and permitted scalar bracing; reject excess scalar elements.
- [ ] Validate array/struct/union initializer shape, recursive element types, element count, member order, and excess elements.
- [ ] Apply permitted brace elision and omitted-element zero/default initialization rules.
- [ ] Check string-to-character-array initialization, terminator space, nested string rows, and disallow incompatible element types.
- [ ] Reject ordinary scalar/pointer initializers for whole arrays unless a specific language rule permits them.
- [ ] Validate aggregate copying and class construction separately; select applicable constructors and diagnose missing/ambiguous matches if supported.
- [ ] Require initialization for const objects where required; account for constructors/member defaults rather than checking a const flag alone.
- [ ] Validate global/static initializers under the chosen constant/dynamic initialization policy. Dynamic initialization is not universally an error in a C++-like language.
- [ ] Record static zero initialization, automatic initialization, member initialization, and any required runtime initialization work.
- [ ] Check static member declarations/definitions under baka's chosen rules and prevent accidental instance storage for static fields.
- [ ] Plan required constructor/destructor actions for automatic objects, arrays, members, bases, globals, and statics if object lifetimes are supported.

## 10. Function declarations, calls, and returns

- [ ] Construct function signatures with return type, adjusted parameter types, variadic flag, member ownership, and applicable qualifiers.
- [ ] Validate compatible prototypes/definitions; reject duplicate definitions, conflicting return types, and forbidden linkage changes.
- [ ] Reject nested definitions if unsupported; handle block-scope prototypes according to the language rules.
- [ ] Distinguish a function call from a constructor call and a function-pointer call; reject calls on arithmetic/array/aggregate values unless callable objects are explicitly supported.
- [ ] Resolve direct and indirect callees and check required/excess argument counts against defaults and variadic status.
- [ ] Check conversion of every fixed argument to its parameter type, including pointee qualification and by-value aggregate completeness.
- [ ] Type-check default expressions, their declaration-time scope, permitted references, conversion to parameter types, and repeated-default restrictions.
- [ ] Merge defaults across compatible declarations only if supported; otherwise diagnose conflicting repeated defaults.
- [ ] Apply variadic default promotions, including float/integer promotions, and check permitted variadic argument categories.
- [ ] Preserve argument boundaries: `f(a,b)` has two arguments; `f((a,b))` has one. Count only immediate children of the call's argument wrapper.
- [ ] Handle function decay/address-of, compatible function-pointer assignment, and signature validation on indirect calls.
- [ ] If overloads are supported, identify signatures, rank viable conversions, and reject no-match/ambiguous calls. Return type alone must not distinguish overloads under C++-like rules.
- [ ] Check every return against its enclosing function: expression present/absent, expression conversion, void-value policy, and constructor/destructor restrictions.
- [ ] Diagnose return outside a function if representable after parser recovery/future grammar changes.
- [ ] Apply the chosen fall-through rule for non-void functions and the special executable `main` rules if supported.
- [ ] Supply the resolved call/result type and inserted conversions to lowering; argument passing/register assignment remains a later code-generation task.

## 11. Conditions, loops, switches, and jumps

- [ ] Check condition conversions in `if`, `else if`, `while`, `do while`, and present `for` conditions.
- [ ] Treat omitted `for` conditions as true and omitted initializer/update as absent; type-check discarded updates without requiring boolean type.
- [ ] Enforce `for` declaration visibility through condition/update/body and end it after the loop.
- [ ] Unlabeled break: find an enclosing loop or switch; reject otherwise.
- [ ] Unlabeled continue: find an enclosing loop even when nested in a switch; reject a switch-only context.
- [ ] Named break/continue: resolve to an active enclosing loop, rejecting absent, already-exited, sibling, ordinary goto-label, or cross-function targets.
- [ ] Define and enforce duplicate active loop-label behavior and permitted reuse after a loop ends.
- [ ] Record continue targets correctly: `for` update, `while` test, and `do while` test; named continue uses the selected outer loop's target.
- [ ] Switch: require a permitted integral/enum type and apply the controlling expression's promotion/conversion rules.
- [ ] Require case/default labels to belong to an enclosing switch; nested switches have independent case/default sets.
- [ ] Type-check and constant-evaluate each case, convert to the switch comparison type, then reject duplicate converted values.
- [ ] Allow at most one default per switch; distinguish nested switch defaults from duplicates.
- [ ] Resolve goto targets function-wide, allow valid forward targets, and reject undefined or duplicate labels even across nested blocks.
- [ ] Track legal labeled statements and their bodies so control-flow lowering receives the correct destination.
- [ ] If C++-like lifetime rules apply, reject goto/switch entry that bypasses required initialization and compute cleanup when control exits scopes via return/break/continue/goto.
- [ ] Reset loop/switch/return context for every function, including any parsed nested function, to prevent inheriting the enclosing function's targets.

## 12. Structs, classes, inheritance, unions, and enums

- [ ] Track forward-declared, being-defined, and complete aggregate states separately. Being inside a definition does not make by-value self-use valid.
- [ ] Enforce aggregate definition uniqueness and permitted class/struct tag compatibility without relying on a single parser `IsStruct` flag.
- [ ] Build member tables for fields, static fields, methods, typedefs, constructors, and destructors; detect conflicting/duplicate members.
- [ ] Preserve member declaration order for aggregate initialization and layout; predeclare methods/members for lookup only as language rules allow.
- [ ] Resolve `obj.member` on appropriate aggregate expressions and `ptr->member` on suitable pointers; reject missing members and wrong receiver types.
- [ ] Propagate receiver constness to non-static fields and validate permitted member calls on const objects.
- [ ] Resolve unqualified instance members through the implicit object; reject use without an object in static methods/free functions.
- [ ] Permit `this` only where an implicit object exists; assign pointer-to-enclosing-class type and applicable qualification.
- [ ] Distinguish static and instance member access and prevent instance offsets being applied to static fields.
- [ ] Validate base class kind, completeness, self-inheritance, inheritance cycles, and unsupported multiple inheritance.
- [ ] Resolve inherited fields/methods and define hiding/overload behavior; apply permitted derived/base pointer and object conversions.
- [ ] Compute base-subobject and field offsets/alignment without recursive layout cycles.
- [ ] Validate constructor signatures, calls, defaults, duplicates/overloads, and any generated default constructor.
- [ ] Validate destructor uniqueness and any generated destructor; preserve all declarations so duplicate checks are possible.
- [ ] Define required base/member construction/destruction order and the availability of required special members.
- [ ] Union: retain its name/type identity, validate members, layout by maximum member size/alignment, and restrict simultaneous member initialization.
- [ ] If unions can contain nontrivial class members, implement explicit lifetime/special-member rules or reject that unsupported combination.
- [ ] Enum: register type identity and enumerator value symbols, detect duplicate/conflicting names, and assign increasing implicit values.
- [ ] If explicit enum initializers are added, require valid integral constants, allow references to earlier enumerators, and check representable values/increment overflow.
- [ ] Apply enum conversion rules consistently in assignment, arithmetic, comparison, switch, initialization, and calls.
- [ ] Anonymous aggregates/enums need valid internal identity even when no source name exists.

## 13. Builtins, optional allocation, and diagnostics

- [ ] Provide declarations/semantic types for supported I/O, file APIs, and command-line parameters, since there is no preprocessor/header inclusion.
- [ ] Validate builtin calls using the same signature checks as normal calls; optionally check literal printf/scanf formats when builtin identity is known.
- [ ] If `new`/`delete` are intended features, first add AST/parser support, then check allocated type completeness, constructor arguments, array bounds, pointer result, delete operand type, and required destruction.
- [ ] If allocation is unsupported, diagnose it explicitly; lexer keywords alone do not implement the feature.
- [ ] Report errors at the use/operator and include the previous declaration location for conflicts where useful.
- [ ] Keep unreachable code, unused declarations, suspicious narrowing, use of uninitialized locals, returned addresses of locals, obvious bounds mistakes, and missing returns as a separately chosen warning/dataflow policy.
- [ ] Do not make general runtime bounds, null dereference, memory lifetime, or division-by-zero detection mandatory compile-time rejection. Required constant-expression failures are a separate matter.
- [ ] Do not report merely declared external functions as undefined without an explicit whole-program/linking policy.
- [ ] Do not import unsupported language features just because the external C test corpus contains them: `extern`, signed declarations, namespaces, scope resolution, access control, multiple inheritance, enum classes, references, templates, virtual methods, and operator overloads need their own explicit scope decision.

## 14. Checks already present in parsing: retain or deliberately relocate

These are not all missing checks. Some overlap semantic responsibilities but currently run in the parser.

| Existing check | Location | Limitation / remaining work |
| --- | --- | --- |
| Type name lookup | `src/parser/type.cc`, `parser.cc` | Only parser type flags; no resolved types or value binding |
| Declarator name cannot be visible type name | `declaration.cc` | Broad policy, not ordinary duplicate-object detection |
| Duplicate static/const | `ParseDeclarationList` | Definition path does not check duplicates consistently; confirmed duplicate `static` on a function is accepted |
| Defaults must form a trailing suffix | `function.cc` | Does not check types, scope, cross-declaration defaults, or call arity |
| Some struct/class redeclarations and definitions | `struct.cc`, `class.cc` | Shared flags; completion and actual semantic type identity still needed |
| Base exists and has defined flag | `struct.cc`, `class.cc` | Does not replace inheritance/layout validation |
| Enum/union type-name conflict | `enums.cc`, `union.cc` | No enumerator/member value checks; some conflict paths can assert |
| Constructor/destructor matching name | `constructor.cc`, `destructor.cc` | Duplicate nodes are overwritten; signature/body semantics remain |
| Loop label is not a type name | `iterationStatement.cc` | Does not resolve break/continue targets or define duplicate-label rules |
| Unsigned spelling restriction | AST constructors | Assertion instead of recoverable diagnostic; typedef path differs |

Do not interpret every `// throw error` comment as an absent semantic check. Many occur after consuming an expected keyword, or at missing delimiter paths. Those are parser entry invariants or unfinished syntax diagnostics.

## 15. Parser/AST fixes that semantics depends on

- [ ] Accept `return;` as an absent expression; let semantics check whether the function permits it.
- [ ] Accept standalone empty statements and empty loop bodies. Current expression-statement parsing expects an expression even at `;`.
- [ ] Support omitted array bounds in permitted declarations/abstract declarators and preserve an explicit unspecified-bound marker.
- [ ] Add explicit enum initializer expressions if they belong to the feature set; current enum grammar accepts names only.
- [ ] Retain union names in `UnionNode`; parser-local registration is unavailable as a substitute for a semantic AST declaration.
- [ ] Retain `...` in `FunctionParameterList`/the signature; it is currently consumed and discarded.
- [ ] Preserve all constructor/destructor declarations or diagnose duplicates before overwriting. A single pointer cannot support overload/duplicate analysis.
- [ ] Preserve member order across fields/functions/typedefs when source order matters; `StructBodyNode` currently separates them into collections.
- [ ] Preserve whether an initializer was braced when language rules need it; expression and single-element brace forms must not become indistinguishable.
- [ ] Add access to literal values, expression children/operators, full nested declarators, qualifiers, bodies, and parameter lists.
- [ ] Replace user-input assertions/uncaught errors for invalid modifiers and type-state transitions with diagnostics.
- [ ] Fix parser lookahead/recovery when a type-kind conflict triggers aggregate-state mutation; `enum E{A}; struct E x;` currently crashes.
- [ ] Preserve the existing nested comma wrapper in calls: probing confirms argument boundaries already survive parsing. A dedicated argument list is optional, not a prerequisite.
- [ ] Preserve a meaningful statement/label relationship. Label parsers currently create null statement pointers and rely on later block siblings; unbraced labeled bodies need particular care.
- [ ] Keep malformed delimiter, missing keyword, and grammar recovery work in the parser. Semantics cannot recover discarded or never-parsed syntax.
- [ ] Clarify supported type spellings/parameter syntax: unnamed parameters are explicitly excluded by README; `main(void)` and `long long` declarations need parser decisions if desired.
- [ ] Keep README-excluded syntax excluded unless intentionally expanding scope; semantic analysis does not implement preprocessing or missing grammar.

## 16. Implementation order and validation

1. Resolve language policy decisions; add diagnostics, AST access/source spans, and preserve currently discarded semantic information.
2. Implement types and scoped symbols, typedef resolution, declaration classification, compatibility, and aggregate completion.
3. Type-check literals, operators, conversions, assignments, calls, returns, and initializers.
4. Implement constant evaluation, bounds, switch cases, and type/layout queries.
5. Resolve goto/loop labels, validate statement context, and add lifetime-aware jumps if required.
6. Complete member lookup, inheritance, constructors/destructors, and static/dynamic initialization.
7. Produce typed output usable by lowering; add chosen warnings and builtin checks separately.

For every rejection rule, include an invalid example and a nearby valid example. Cover typedef-expanded forms, nested pointers/arrays, shadowing, and nested loop/switch contexts. Verify diagnostics and absence of crashes, not merely a nonzero exit status. Keep language-policy-dependent cases separate from definite errors.

The `programs/` directory contains hand-written audit inputs, not a conformance suite with fixed semantic expectations. `run.py` records both lexer and parser outcomes and saves token/AST logs. See `RESULTS.md` for observed behavior. No compiler implementation was modified for this audit.

## 17. Additional standard review and loop-label audit

See [the additional nested checklist](extended/FINDINGS.md) for newly identified omissions, corrections, and concrete parser failures from 108 additional programs. It distinguishes new tasks from refinements of existing tasks and cites public WG14 C drafts. The original 113-program results above remain a historical snapshot.

For implementation ownership, use the [three-person task split](TASK_SPLIT.md): 24 work packages per person, with every main/extended checkbox assigned in the ownership map.
