# context_A.md — Teammate A handoff (types, symbols, declarations, aggregates)

Baseline commit: `b6f9a26` (`format testcases`). All work below is **uncommitted** on branch `saturn/slop` (applied from a master stash). The compiler driver does **not** call the semantic layer yet: `--parse` is still parser-only. Do not treat parser acceptance as semantic completion.

## 1. Summary

Teammate A implemented the semantic foundation for the C++-like subset compiler: a header-only C++23 service package under `src/semantic/` (type model with LP64 layout, hashed type interning, symbols/scopes, declaration checking, aggregate metadata) plus adapters from the existing AST to those services. It also repaired A-owned parser/AST code so the semantic layer receives input the parser previously discarded (enum initializers, omitted array bounds, union names, all ctors/dtors, member order). `src/base/symbol_table/` was switched from `__gnu_pbds::gp_hash_table` to node-based `std::unordered_map` so entry pointers stay stable across rehashes. Everything is verified: two service/AST suites and 15 parser regression checks pass, and the historical audits reproduce exactly (95/18 original, 94/14 extended, 0 crashes).

## 2. Changes by file

### New: `src/semantic/` (header-only; B/C include, no build-system changes)
- `types.hh` — central type model:
  - ID wrappers `TypeId`/`SymbolId`/`ScopeId` (`uint32_t`, defaulted `==`; value 0 is always error/null).
  - `TypeKind`, `Builtin`, `Completion{Forward,Defining,Complete}`, `BoundKind{Unspecified,Constant}`, `Bound`, `Member` (name/type/symbol/isStatic/isType/isConstructor/isDestructor/isConstMethod/source), `Type`, `Layout`, `QueryState`, `LayoutResult`.
  - Class `Types`: LP64 layout (`ComputeLayout`, `layout`), sizes/ranks/signedness (`arithmetic`, `sizeType`), type constructors (`pointer`, `array`, `function`, `constant` (const-qualify), `nominal`), aggregate editing (`beginDefinition`, `setBase`, `addMember`, `bindMemberSymbol`, `finishDefinition`), queries (`validDerived`, `lookupMember`, `derivesFrom`, `recursivelyConst`, `subobjects`, `instanceMembers`).
  - Hash-based interning (see §3): `MixHash`, `HashVector`, `CanonicalHash`, `CanonicalEqual`, `InternHash`, `InternEqual`, `InternSet InternTable`.
  - `Types(const Types&) = delete` — one instance lives through checking/lowering.
- `symbols.hh` — `Diagnostic{code,message,source,previous}` (diagnostic codes have **no** prefix; e.g. `"undeclared"`, `"redeclaration"`, `"bound"`), `Symbol`, `Scope`, `Declaration`, `SymbolKind{Object,Function,Alias,Type,Enumerator,Parameter,BuiltinFunction}`, `ScopeKind{Global,Block,Function,Prototype,Aggregate,For}`, `Linkage`. Class `Symbols`: scope stack (`enter`/`leave`), `lookup`/`lookupCurrent`/`bind`, `declare` (redeclaration/linkage/overload rules), `completeObject` (inferred array bounds), `setConstant`, `objectsLeaving` (cleanup order), `installTypes`, `installLibrary` (printf/scanf only), `diagnose`/`diagnostics`.
- `declarations.hh` — class `Declarations`: `ConstantState{Value,Nonconstant,Unavailable,Error}`, `IntegerResult`, `DeclarationContext{parameter,member,prototype,storageRequired}`, `SpecialMembers`; API: `specifier`, `boundArray`, `validate`, `object`, `functionDefinition`, `alias`, `aggregate`, `beginAggregate`, `member`, `base`, `finishAggregate`, `enumerator`, `nextEnumerator`, `specialMembers`, `memberAccess`, `thisType`.
- `ast_declarations.hh` — `EvaluationContext{purpose,scope,expected}`, abstract `DeclarationEvaluator` (B implements `IntegerResult integer(const types::ExpressionNode*, EvaluationContext)`), class `ASTDeclarations`: `specifier`, `typeName`, `declarator` (pointer/array/function suffix order), `parameters`, `declarationList` (with `InitializerCheck` callback), `aliases`, `enumeration`.
- `aggregates.hh` — `MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>` (B supplies); class `ASTAggregates`: `forward`, `definition(...)` overloads for struct/class/union AST nodes.
- `.clang-format` — scopes formatting rules to this module only.

### Modified: `src/base/symbol_table/symbol_table.hh`
- `gp_hash_table` → `std::unordered_map` (node storage ⇒ stable entry addresses; `AddEntry` no longer asserts on duplicates, returns `false`).
- Added `LookupCurrent` (current-scope-only); `ExitScope` guards empty stack; `AddEntry`/`RegisterOrReplaceType` auto-open a scope when the table is empty.

### Modified: A-owned parser files
- `src/parser/class.cc`, `src/parser/struct.cc` — name checks now use `TypeLookup.LookupCurrent(...)` (current-scope) instead of outward `LookupType`; `SetIsStructDefined` only when actually a struct.
- `src/parser/enums.cc` — removed a stray file-scope `enum {BALLS,EAT}`; named enums recheck via `LookupCurrent` and only `AddType` when new; `K_INT`/`OP_ASSIGN` now parses an optional enumerator initializer expression into the AST.
- `src/parser/union.cc` — union names registered via current-scope check, `union U;` forward declarations return `UnionNode(nullptr, name)`, named node created with name.
- `src/parser/structBody.cc` — null `ParentName` guarded (anonymous aggregates no longer crash on ctor detection); anonymous destructor diagnosed and redirected with placeholder parent name.
- `src/parser/declaration.cc` — `[]` with no size appends a null `ArraySize` (bound `Unspecified`), in both declarator paths; removed the "identifier cannot be a type name" error so a declaration may shadow an outer type name.
- `src/parser/type.cc` — central invalid-`unsigned` guard in `ParseTypeIdentifier`: diagnoses "unsigned requires char, short, int, or long" and returns an error-marked `int` identifier instead of tripping legacy AST asserts (covers B's function/parameter parsers too).
- `src/parser/typedef.cc` — `RegisterOrReplaceType` only when the name is not already declared in the current scope (preserves typedef type metadata; compatible alias repeats stay valid).

### Modified: AST headers (A owned per TASK_SPLIT.md)
- `declaration.hh` — `DeclarationList`/`FunctionParameter`/`DeclarationIdentifierNode`/`SingleDeclarationNode` gained const accessors (`getIsStatic`, `getIsConst`, `getModifier`, `getTypeName`, `getDeclarations`, `getParameters`, `getDeclarator`, `getIdentifier`, `getArraySizes`, `getFunctionParameters`, `getInnerDeclaration`, `getVariableName`(null-safe), `getInitializer`); legacy ctor asserts for invalid unsigned removed (parser now diagnoses); `FunctionParameterList` gained `getParameters`, `isVariadic`, constructor/a setter `setVariadic` (**B must set this when it consumes `...`** — currently nothing sets it).
- `enums.hh` — `EnumNode` stores parallel `std::vector<ExpressionNode*> Initializers`; `addEnumValue(value, initializer = nullptr)`; `getInitializers()`.
- `structBody.hh` — `Constructor`/`Destructor` are now **vectors** (`Constructors`, `Destructors`) and everything appends to a source-order `std::vector<ASTNode*> Members`; `getMembers()`; wide accessor set (`getDeclarations`, `getFunctions`, `getTypedefs`, `getConstructors`, `getDestructors`).
- `struct.hh`, `class.hh` — `getName`, `getBaseName`, `getBody` (+ `getName` on declaration nodes).
- `typedef.hh` — ctor init order fixed (isConst initialized first), accessors `getIsConst`, `getModifier`, `getTypeName`, `getVariables`.
- `typenode.hh` — ctor assert removed; accessors `getIsConst`, `getModifier`, `getTypeName`.
- `union.hh` — `UnionNode(body, name = nullptr)` optional name; `getName`, `getBody` (null-safe Print).

### New: `semantic_audit/teammate_a/`
- `POLICY.md` — the binding language/type contract (LP64, canonical vs nominal types, declaration order rules, array bound policy, aggregate/layout rules, enums, printf/scanf).
- `run.py` — compiles `services_test.cc` and `ast_test.cc` (`c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -Isrc -Isrc/base`), runs them, then 15 parser regressions against `build/baka --parse --verbose`; excludes `valid_aggregate_shadow.cpp` as a known C gap.
- `services_test.cc`, `ast_test.cc` — service checks incl. interning stress (24-param vector hash, rotated/mutated params, slot reuse after duplicate pop, 512-element params, name-equivalence of structurally identical anonymous structs), parser symbol-table stability over 10 000 inserts.
- `programs/` — 6 targeted parser fixtures (`valid_aggregate_shadow.cpp` is the known-failing C-integration case).
- `verification.json` — compact audit results (parser counts, integration gaps).

### Coverage map: the 24 A packages (A01–A24) and who integrates each

| Package | Implemented deliverable | Integration responsibility |
| --- | --- | --- |
| A01 | LP64 widths/ranks/signedness/alignment, sizeof type, explicit supported subset | B/C consume POLICY.md |
| A02 | Canonical builtin/pointer/array/function/const-qualified/error types, nominal enum/aggregate IDs | shared `Types` instance |
| A03 | Stable SymbolId/ScopeId arenas, current/outward lookup, lexical hiding; parser storage node-based | C owns scope traversal |
| A04 | Per-declarator bound → binding → initializer order; no later-local predeclaration | B evaluator/initializer callbacks, C traversal |
| A05 | Prototype/parameter registration, duplicates, shared parameter/outer-body scope contract | C must reuse the Function scope for the outer body |
| A06 | Kinds, definitions, compatible repeats, overloads, linkage/storage conflicts | C classifies declaration versus body |
| A07 | Alias identity/qualification/completion and incompatible replacement errors; parser preserves existing type metadata | B/C use the alias service |
| A08 | Semantic specifier validation; invalid unsigned inputs diagnose without aborting | B still owns duplicate static/const checks in `function.cc` |
| A09 | Nested declarator binding, illegal derived types, object-with-body rejection | C calls `functionDefinition` for bodies |
| A10 | Prototype vs definition storage-completeness; arrays below pointers checked | B completes inferred array bounds |
| A11 | Parameter adjustment, const preservation on parameter objects, normalized overload signatures | B preserves variadic syntax and selects calls |
| A12 | Constant/unspecified array bounds only; nonconstant bounds rejected before type construction | B must classify bound constants correctly |
| A13 | Forward/Defining/Complete states, anonymous identity, recursive containment rejection, union forwards | C dispatches aggregate AST declarations |
| A14 | Ordered members/aliases/functions/special members, duplicate signatures/names, member symbols | B supplies function signatures via `MemberSignature` |
| A15 | Single complete nonunion base, cycle/duplicate checks, inherited lookup/hiding | B conversion and call rules use base queries |
| A16 | Checked layout, alignment/padding, union/base/field offsets, overflow/incomplete results | B `sizeof` and C lowering consume queries |
| A17 | Retained enum initializer AST, declaration-time constant callback, constants/conflicts/overflow | B implements `DeclarationEvaluator` |
| A18 | Every ctor/dtor retained; signature validation, generated special-member metadata, subobject lifetime order | B signature callback; C/B body and cleanup integration |
| A19 | Union names/forwards, static storage exclusion, nontrivial union restrictions | B/C manage the chosen active union alternative |
| A20 | Invalid unsigned and aggregate-state crash repairs; `[]` retention; safe malformed-declarator access | existing excluded spellings remain excluded |
| A21 | Flexible arrays/bit-fields/designators explicitly disabled; incomplete member storage rejected | B rejects unsupported initializer forms |
| A22 | Builtin type names and protected printf/scanf signatures/identities | additional file APIs need explicit agreed signatures |
| A23 | Dot/arrow/member/this/static/const receiver services, inherited overload candidates | B checks receiver value category and call viability |
| A24 | Service and AST suites, owned parser crash/valid controls, full audit comparisons | compiler-level semantic fixtures await C integration |

### Pre-existing, untouched (do **not** attribute to A)
- `source_code` scratch input edit, `.codex/config.toml`, the rest of `semantic_audit/` (CHECKLIST, TASK_SPLIT, TASK_OWNERSHIP, run.py, RESULTS, extended/ corpus).

## 3. Implementation details

### Type interning (structural equivalence) in `types.hh`
- Canonical types (pointer/array/function/const) are interned structurally:
  - `MixHash(size_t& Hash, size_t Value)` — 64-bit golden-ratio combiner (`Hash ^= v + HashSeed + (Hash<<6) + (Hash>>2)`).
  - `HashVector(Seed, vector<TypeId>)` — mixes length then every element *in order* ⇒ order- and length-sensitive.
  - `CanonicalHash` covers exactly `kind, builtin, element, bound{kind,count}, parameters, variadic`; `CanonicalEqual` checks the same field set — parity is a hard invariant.
  - `InternTable` is `std::unordered_set<uint32_t, InternHash, InternEqual>` over **arena indices**; `InternHash`/`InternEqual` capture `this`, hence `Types` is non-copyable/non-movable.
  - `InternType`: `AddType` pushes the candidate onto the `deque` arena, then the set insert either finds a duplicate (⇒ `TypeArena.pop_back()`; IDs stay contiguous) or keeps it.
- Nominal types (`nominal()` — struct/class/union/enum) are never interned (always `AddType`): structurally identical aggregates keep distinct IDs (name equivalence). Aliases point at the underlying ID.
- Const over arrays qualifies the element (`constant()` recursion); completion state is deliberately **not** hashed (an aggregate completing doesn't change its ID).
- Stress-verified: 4 000 randomized constructions replay exact IDs.

### Data flow (declaration-time)
```
AST (struct/class/union/enum/typedef/declaration nodes)
        │
ASTAggregates / ASTDeclarations          ← adapters, C calls per source order
        │ uses                     │ calls B via
Declarations checking        DeclarationEvaluator::integer() (array bounds, enumerator values)
        │                            │ results recorded in
Types arena + Symbols scopes/symbols/diagnostics
        │
B consumes: layout queries, member metadata, symbols; C consumes: symbols, scopes, objectsLeaving, diagnostics
```
- Source order is enforced: each declarator's bounds → insert symbol → initializer callback → next declarator. `int x = x;` sees the new x; `int x[sizeof(x)]` does not.
- Unspecified array bounds are inserted first; the initializer (B) infers them via `Symbols::completeObject`, after which A re-validates storage.
- Parameters adjust to pointers (`parameterObject`/`adjustParameter`); prototypes checked before definitions; function definitions require complete storage.

### Config/env
- None. New code is headers only; tests compile with the system `c++` (`-std=c++23 -Wall -Wextra -Wpedantic -Werror`).

## 4. Design decisions

Deliberate (can be changed, but coordinate):
- **LP64 fixed target**, layout = size/alignment/padding/offsets via `ComputeLayout`; constexpr-free runtime model. Rejected: consulting host ABI (POLICY.md A01).
- **Constant-only array bounds** (`BoundKind{Unspecified,Constant}`); runtime arrays/VLAs are not representable; nonconstant bounds are rejected by `boundArray` *before* a type is created. Rejected: runtime layout state.
- **Interned canonical vs nominal types**: structural identity for derived types, name identity for aggregates. Alternative hashing-everything-as-identity rejected (collisions conflate types; a hash cannot store layout/members). The prior O(n) scan intern was replaced by the hash table.
- **Node-based parse symbol table** replacing `pb_ds` hash; `AddEntry` returns bool instead of assert. Rejected: keeping rvalue/unstable pointers.
- **Current-scope name checks** in struct/class/enum/union registration: inner scopes may shadow outer types; `struct S variable;` at file scope still blocked by C-owned `ParseProgram` dispatch (see §6).
- **`unsigned` only on direct char/short/int/long spellings**; invalid spellings diagnose in `type.cc` (single guard covers B-owned parsers). No `signed`, no `long long`, no `(void)` param spelling, no references, no const-pointer spelling.
- **Diagnostics as data** in `Symbols::diagnostics()` with bare owner-free codes; services never fabricate valid results (error = ID 0 / `{}`).

Temporary shortcuts / known simplifications:
- `Types` non-copyable instead of a pointer-stabilizing design (fine: one instance per pipeline).
- `installLibrary` covers exactly printf/scanf; FILE/file APIs deliberately not guessed.
- Flexible arrays/bit-fields/designated init explicitly rejected by `Declarations::member` even though the extended parser now *accepts* the flexible-array syntax.
- Unverified: ast_test.cc coverage beyond what run.py executes (run.py passes both suites).

## 5. Interfaces and contracts (exact, stable)

### `src/semantic/types.hh` — class `Types` (one instance per pipeline, non-copyable)
```cpp
const Type& get(TypeId) const;      TypeId builtin(Builtin) const;
std::optional<ArithmeticInfo> arithmetic(TypeId) const;  // {bits,rank,isSigned,floating}
TypeId sizeType() const;            // ULong
TypeId unConst(TypeId) const;       // strips top-level const; identity otherwise
bool   isConst(TypeId) const;       bool isAggregate(TypeId) const;
bool   isIntegral(TypeId) const;    bool isVoid(TypeId) const;
TypeId constant(TypeId, bool = true);  // const-qualify (arrays qualify elements)
TypeId pointer(TypeId);                TypeId array(TypeId, Bound = {});
TypeId parameterObject(TypeId);        TypeId adjustParameter(TypeId);
TypeId function(TypeId result, std::vector<TypeId> params = {}, bool variadic = false);
TypeId nominal(TypeKind, std::string name = {});
bool   beginDefinition(TypeId);        bool setBase(TypeId, TypeId);
bool   addMember(TypeId, Member);      bool bindMemberSymbol(TypeId, size_t, SymbolId);
LayoutResult finishDefinition(TypeId); LayoutResult layout(TypeId) const;
bool   validDerived(TypeId) const;
MemberLookup lookupMember(TypeId receiver, const std::string&) const;
bool   derivesFrom(TypeId, TypeId) const;  bool recursivelyConst(TypeId) const;
std::vector<Subobject> subobjects(TypeId, bool destruction = false) const;
std::vector<size_t>    instanceMembers(TypeId, bool destruction = false) const;
```
Invariant: `TypeId{0}` / `SymbolId{0}` / `ScopeId{0}` are invalid; constructors return `{}` on error; equality of the same canonical structure is ID equality (hash interning).

### `src/semantic/symbols.hh` — class `Symbols`
```cpp
ScopeId enter(ScopeKind);              bool leave();
std::vector<SymbolId> lookup(const std::string&) const;       // outward
std::vector<SymbolId> lookupCurrent(const std::string&) const; // scope only
SymbolId bind(const std::string&, const void* source);        // value use; rejects overload sets/aliases
SymbolId declare(Declaration);         // Declaration{name,kind,type,definition,isStatic,linkage,source}
bool     completeObject(SymbolId, TypeId completed);  // [] → N inference; B calls after insertion
bool     setConstant(SymbolId, int64_t);
std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const; // cleanup order
TypeId lookupType(const std::string&, source);
void   installTypes();  void installLibrary();   // once, global scope
const std::vector<Diagnostic>& diagnostics() const;  void diagnose(code, message, source, previous);
```

### `src/semantic/declarations.hh` — class `Declarations`
```cpp
IntegerResult{ConstantState state; int64_t value;}
enum class ConstantState { Value, Nonconstant, Unavailable, Error };
TypeId specifier(std::string name, bool constant=false, bool isUnsigned=false,
                 std::optional<TypeKind> expected={}, const void* source);
TypeId boundArray(TypeId element, IntegerResult, const void* expr, bool omitted=false);
bool   validate(TypeId, DeclarationContext, const void* src) = validate(TypeId, {parameter,member,prototype,storageRequired}, src);
SymbolId object(Declaration, DeclarationContext);
SymbolId functionDefinition(Declaration);
SymbolId alias(std::string name, TypeId, const void* src);
TypeId  aggregate(std::string name, TypeKind, const void* src);
bool    beginAggregate(TypeId, src);  bool base(TypeId, TypeId, src);  bool finishAggregate(TypeId, src);
bool    member(TypeId owner, Member);
SymbolId enumerator(std::string, TypeId, IntegerResult, src);
IntegerResult nextEnumerator(std::optional<int64_t>) const;
SpecialMembers specialMembers(TypeId) const;   // {constructors,destructor,implicitDefault/implicitCopy/implicitDestructor,nontrivial}
Access memberAccess(TypeId receiver, std::string name, bool arrow, bool staticContext, src);
TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod, src);
```

### `src/semantic/ast_declarations.hh` and `aggregates.hh`
```cpp
struct EvaluationContext{ enum Purpose{ArrayBound,Enumerator}; Purpose purpose; ScopeId scope; TypeId expected; };
class DeclarationEvaluator { virtual IntegerResult integer(const types::ExpressionNode*, EvaluationContext) = 0; };  // B implements
class ASTDeclarations {
    TypeId specifier(const types::IdentifierNode*, types::TypeSpecifierModifier, bool constant, const void* src);
    TypeId typeName(const types::TypeNode&);
    TypeId declarator(TypeId base, const types::DeclarationIdentifierNode*);       // errors → {}
    std::vector<SymbolId> parameters(const types::FunctionParameterList&);         // into C's current Function scope
    std::vector<SymbolId> declarationList(const types::DeclarationList&, InitializerCheck = {}); // InitializerCheck: std::function<void(SymbolId, const types::ExpressionNode*)>
    std::vector<SymbolId> aliases(const types::TypedefNode&);
    TypeId enumeration(const types::EnumNode&);
};
class ASTAggregates {
    TypeId forward(const types::IdentifierNode*, TypeKind, const void* src);
    TypeId definition(const types::StructDefinitionNode&, MemberSignature = {});   // + Class/Union overloads
    using MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>;
};
```

Naming convention for diagnostics: bare dot-free codes like `"redeclaration"`, `"unsigned"`, `"union-lifetime"`; `source` is the offending AST node pointer, `previous` the earlier conflicting symbol. `Diagnostic::source`/`previous` are opaque handles for C's source-span map.
`.clang-format` file must be preserved by B/C edits under `src/semantic/`.

## 6. Handoff for B and C

### Teammate B — expressions, calls, constants, initialization (24 boxes)
Build on:
- `Types::arithmetic`/`sizeType`/`layout` for numeric semantics and `sizeof`; `Types::constant`/`unConst`/`isConst` for the qualification model (pointee qualification supported; const-pointer spelling excluded by the parser).
- The `ConstantState` contract: implement `DeclarationEvaluator::integer(expr, ctx)` — return `{ConstantState::Value, n}` only for true integral constants in the live environment; `Nonconstant`/`Unavailable`/`Error` otherwise; never a fabricated value. The evaluator style used by (wired into) `boundArray` and enumerator paths.
- Overload metadata: `memberAccess(...).candidates`, `lookupMember(...).indices`; `Type::variadic` (via `TypeKind::Function`) gives you the call-tail variadic marker.
- `Symbols::completeObject(id, TypeId)` to complete `T a[] = ...` inside the `declarationList` initializer callback before final storage validation, and `Symbols::setConstant`/`get(id).constant` for enumerator values.
- `MemberSignature` (required by §5) — you own expression/constant/cast/initializer/sizeof AST headers and `function.cc`, so you produce member signatures with `isStatic`/`isConstMethod`/`isConstructor`/`isDestructor` flags from your AST; no fabricated defaults exist.
- Enumerators' explicit initializer AST (`EnumNode::getInitializers`) is retained for you to evaluate.

Do **not** change:
- Existing `Type` field semantics (`element` for pointee/qualified/return), LP64 tables, enum rank/signedness, source-order semantics, diagnostic code set ("`bound`", "`redeclaration`", ...), and the `.clang-format` under `src/semantic/`.
- Parser unsigned guard in `src/parser/type.cc` (it already covers your `function.cc` inputs).

Policy choices written down, not negotiated live — change deliberately, with A's contract in mind (`POLICY.md`): constant-only array bounds; optional aggregate syntax (flexible arrays, bit-fields, designated initializers) disabled; zero-argument `()` means exactly zero parameters; single nonvirtual inheritance; the existing source exclusions (`long long`, unnamed parameters, `(void)` grammar, references, namespaces) stay unexpanded.

`installLibrary()` gives printf/scanf builtin `SymbolId`s with protected signatures — use those identities for optional format checking. Adding any other library declaration (FILE, file APIs) requires a jointly agreed signature/opaque-type policy first.

Known B tasks (unverified by A whether done):
1. Set `FunctionParameterList::setVariadic` where `function.cc` consumes `...` (flag exists; nothing sets it).
2. `TryParseFunction` still fails to diagnose repeated `static`/`const` specifiers.
3. Supply `MemberSignature` (function/constructor/destructor signatures with static/const/special-member flags — A preserves every special member in source order and substitutes nothing) and expression receivers (`this` via `thisType`), call ranking, promotions.
4. Type inference/initialization: defaulted special-member construction/copy/destruction *usability and deletion*, member initializers, global/static initialization, and const-initialization requirements are all B behavior — C must not bypass A's final storage validation.

### Teammate C — pipeline, statements, driver (24 boxes)
Build on:

**Integration sequence (exact) — implement in this order:**
1. Create one `Types`, one `Symbols`, one `Declarations`; call `installTypes()` once (plus `installLibrary()` only if using the agreed implicit-library policy). Keep them alive through semantic checking and lowering. Stop after lexer/parser errors. `Diagnostic::source`/`previous` are opaque AST/source handles for C's source-span map.
2. B implements `DeclarationEvaluator::integer` (purpose, current `ScopeId`, expected int type; return Error/Unavailable for invalid/unimplemented; never Value for floating or nonconstant expressions). Build `ASTDeclarations` with that evaluator injected.
3. Visit program/block contents in source order: `ASTDeclarations::declarationList` (with B's initializer callback), `aliases`, `enumeration`, and `ASTAggregates::definition` for A-owned nodes. Initializers see their own new symbol; subsequent declarators do not exist yet. B may complete `T a[] = ...` inside the callback via `Symbols::completeObject`.
4. For a function body, resolve its declaration type first, pass it to `Declarations::functionDefinition`, open a Function scope, register parameters through `ASTDeclarations::parameters`, and visit the OUTER compound body in that SAME scope. Further nested blocks and for-statements enter/leave separate scopes.
5. For aggregate definitions, B supplies `MemberSignature` for function/constructor/destructor nodes (static/const/special-member flags). `ASTAggregates::definition` stores ordered metadata and completes layout before C/B check member bodies; a missing callback result becomes an explicit diagnostic; the temporary member-declaration scope is closed after collection. Method-body name binding needs C/B to expose the class context and use member lookup.
6. Resolve expression names with `Symbols::lookup`/`bind`; retain overload sets for B. `bind` deliberately rejects unresolved overload sets as single value symbols; B handles call selection and expression receiver categories.
7. Attach diagnostics to source locations, stop lowering on any error, and combine `Symbols::objectsLeaving`, `Types::subobjects`, and B's initialization records for jump cleanup and lifetime restrictions. This API does not by itself validate goto legality or execute cleanup.

General properties:
- Stop-on-error: parser recovery marks error nodes (`hasError()`) — skip those; stop the pipeline before lowering on any semantic diagnostic.
- `Diagnostic::source`/`previous` are opaque AST/source handles — feed into your source-span map.

Do **not** change:
- `src/semantic/*` public signatures (listed in §5); `symbol_table.hh` node-storage/stability contract (`GetEntry` stable until scope exit; duplicate inserts return false); A-owned parser registration logic (`struct.cc/class.cc/enums.cc/union.cc/typedef.cc` current-scope rules).
- `semantic_audit/teammate_a/run.py` expectations (15 parser cases + the excluded `valid_aggregate_shadow.cpp`); audit snapshot files.

Known C tasks (the largest gap):
1. **Wire the semantic stage into the driver** — `--parse` is parser-only today; add the CLI/phase and stop on semantic diagnostics (C03/C04).
2. **Local aggregate dispatch** — compound-statement parsing routes `struct Item{...};` through declaration parsing and fails; `ParseProgram` treats every leading struct/class/union token as a type declaration, blocking `struct S variable;` at file scope. Fixture: `semantic_audit/teammate_a/programs/valid_aggregate_shadow.cpp` (currently failing; excluded from the 15).
3. Coordinate in C-owned parser files (`compoundStatement.cc`, `parser.cc`); A's aggregate parsers now support shadowing by current-scope rules.

Likely conflicts: A owns these files — if you must touch `structBody.cc`/`declaration.cc`/`enums.cc`/`union.cc`/`type.cc`/`typedef.cc`/AST headers above, send a patch through A's owner instead (per TASK_SPLIT.md §File ownership), except `struct.hh/class.hh/union.hh/enums.hh/declaration.hh` accessors which are shared-header changes B/C may request.

## 7. Status and open items

Done and verified:
- Service package compiles standalone (`-Wall -Wextra -Wpedantic -Werror`); both suites pass.
- 15 parser regressions pass; driver builds; historical audits reproduce (95/18 original, 94/14 extended, 0 crashes/0 timeouts).
- Hash-based interning stress-tested in-session: 4 000 randomized function-type constructions replay exact IDs.

Partial / open:
- Driver does not invoke the semantic layer (C task).
- `FunctionParameterList::setVariadic` is never set (B task).
- Local aggregate parsing (`valid_aggregate_shadow.cpp`) fails pre-service (C task).
- Repeated `static/const` specifiers in `TryParseFunction` underspotted (B task).
- File APIs/FILE opaque handling undefined (needs joint policy decision before anyone claims file semantics).
- `Types` is non-copyable/non-movable (InternTable captures `this`) — deliberate; relax only with a pointer-stabilizing redesign.
- `unverified`: none of the semantic checks run end-to-end through the compiler (no driver integration exists).

## 8. How to run and verify

```bash
# build driver (expected: built target baka, no warnings)
cmake --build build --parallel 2

# service suites + AST suite + 15 parser regressions (expected: two "all checks passed" lines, 17/17 checks)
python3 semantic_audit/teammate_a/run.py

# full audit runners (expect: lex 113 accepts, parse 95 accepts / 18 rejects; extended: 108, 94/14; 0 crashes/timeouts)
python3 semantic_audit/run.py --output /tmp/out-main
python3 semantic_audit/run.py --programs semantic_audit/extended/programs --output /tmp/out-ext

# compile semantic headers against the warnings policy as B/C should
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -Isrc -Isrc/base -fsyntax-only src/semantic/aggregates.hh
```

Full token/AST logs land in `/tmp/out-*/`. The known-failure `valid_aggregate_shadow.cpp` is reported by `run.py` as the excluded C gap, not a check.
