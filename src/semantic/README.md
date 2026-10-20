# Semantic analysis

Semantic analysis connects the parser AST to the type and symbol services before
lowering. The implemented pass order is:

```
declaration registration -> resolve/rename -> type annotation -> read-only checks
```

This directory contains both the existing rule implementations and the orchestration
that invokes them. The remaining acceptance and regression tasks are tracked in [TODO.md](TODO.md). A rule
existing in a helper does not establish that every AST context invokes it correctly;
end-to-end regression programs remain part of the acceptance criteria.

## Services and ownership

| Files | Responsibility |
| --- | --- |
| `types.hh`, `types.cc` | Stable `TypeId` values, canonical derived types, nominal aggregate identity, completeness, member lookup, LP64 layout |
| `symbols.hh`, `symbols.cc` | Stable `SymbolId`/`ScopeId` values, lexical scopes, declaration compatibility, diagnostics |
| `declarations.hh`, `declarations.cc` | Rules for type formation, object storage, aliases, aggregate members and enumerators |
| `ast_declarations.hh`, `ast_declarations.cc` | Interpret parser specifiers/declarators using the current scoped symbol table |
| `aggregates.hh`, `aggregates.cc` | Register aggregate definitions, signatures, inheritance and layout |
| `expressions.hh`, `expressions.cc` | Expression types, constants, conversion validation, calls and initializer rules |
| `overloads.cc` | Count filtering, exact-first overload selection and converting-constructor viability |
| `conversions.cc` | Materialize checked implicit conversions and compound computations on AST edges |
| `semantic_analyzer.hh`, `semantic_analyzer.cc` | Public phase API, saved contexts, service construction and orchestration |
| `registration.cc` | Top-level registration, callable/member signatures, declaration/body collection |
| `resolution.cc` | Source-order local registration, binding/renaming, statement and expression work collection |
| `annotation.cc` | Expression annotations and cached results, initializer typing and finalized array metadata |
| `checks.cc` | Const validation of queued results, storage policy, returns, switches and jumps |

Zero IDs represent an invalid result. IDs belong to one analyzer's arenas and must
not be reused with another analyzer. Source pointers identify existing parser nodes;
they are not owning pointers. The AST and arenas must outlive consumers of their
semantic metadata.

## Public API and phase state

`SemanticAnalyzer::analyze(ProgramNode&)` invokes these methods in order:

| Method | Resulting `SemanticPhase` | Main output |
| --- | --- | --- |
| `registerDeclarations(ProgramNode&)` | `Registered` | Top-level declarations, signatures and body work |
| `resolveAndRename()` | `Resolved` | Scoped bindings, unique names and contextual expression/control work |
| `annotateTypes()` | `Annotated` | AST type/value metadata and cached `ExprResult` values |
| `check()` | `Checked` | Read-only validation diagnostics |

An analyzer begins in `Parsed`. Each method requires its preceding phase; its
boolean result reports whether diagnostics are empty. `analyze()` stops after
invalid top-level registration, but continues resolution, annotation and read-only
checks after recoverable local errors to diagnose independent work. Error-valued
expressions suppress cascades, and AST conversion materialization is skipped when
errors exist. A `Checked` phase alone is therefore not proof of success: require
empty `diagnostics()` before lowering. `phase()` supports intermediate inspection.
`types()` and `symbols()` expose const service views. The three work-count getters
support structural tests without exposing mutable queues. Use one analyzer for one
AST; this API does not promise a reset/reuse operation.

## AST metadata

`ASTNode::semanticInfo()` holds an `ASTSemanticInfo` record. `Type`, `Symbol`,
`Scope` and `Target` are analyzer-owned arena IDs or control-target identities;
zero means unavailable. `Resolved`, `Annotated` and `Error` describe processing
state. `LValue`, `ConstantExpression`, `HasConstant`, `NullPointerConstant` and
`Constant` carry integral-expression facts. `HasFloatingConstant` and
`FloatingConstant` retain supported floating constant results. `CallSignature`,
`MemberOwner`, one-based `MemberIndex` and `Construction` retain selected call/member
facts for lowering. The parser initializes this record empty.

`IdentifierNode::GetName()` preserves source spelling. `resolve()` stores candidate
symbol IDs and an owned unique name of the form `sourceName.SymbolId`, available
through `getBindings()` and `getResolvedName()`. Overload candidates can require
multiple bindings before call selection. The symbol ID, rather than the generated
spelling, is the binding identity.

## Pass contracts

### 1. Declaration registration

Walk the program's top-level declarations once and collect ordered work items.
Collect aggregate members, function signatures, bodies and initializers as their
owners become known. Preserve original source order and lexical ownership even
when later processing groups items by kind.

Registration is an index of work, not permission to use every collected declaration
from every source location. A later declaration must not become visible earlier
merely because it is already in a queue. Preserve declaration-point information or
activate each binding in source order. Function bodies can recurse through their
own registered signature. All top-level function headers are collected before
body resolution, so forward function calls are supported. Global object references
retain a source-order visibility horizon. Forward goto labels are deliberately function-wide.

Do not repeatedly search the program root to find functions or aggregate bodies.
Queue their pointers when discovering the declaration. Declaration-time type
dependencies are handled in the live environment as described below; registration
is not a general fixed-point solver for arbitrary forward type dependencies.

### 2. Resolve and rename (mutating)

Create and enter lexical scopes, activate declarations in order, and bind identifier
uses through the **current scoped symbol table**. Resolve type names through that
same environment, including typedefs, shadowing and aggregate-kind modifiers.

Attach the resulting binding identity to each relevant AST node. A stable
`SymbolId` is the semantic identity even when source spelling is unchanged. The
resolver stores a separate owned unique name while retaining original spelling
for diagnostics. Never use textual rewriting as a substitute for
symbol identity.

Record resolved type-name identities on type AST nodes, and enqueue expressions,
initializers and statements with their owning scope/function/class context. Register
an implicit `this` binding only in instance member contexts. Function-local goto
labels and active named-loop targets have different lifetime and lookup rules.

### 3. Type annotation (mutating)

Declaration registration/resolution has already built signature and object types
from scoped specifiers and nested declarators. This pass uses those types to annotate
expressions with their `TypeId`, value category, constant-evaluation result and
selected symbol/call target where applicable. Annotate declarations with the final
object/function type, recording a previously inferred array shape where applicable.

The annotated form must preserve `f(a,b)` versus `f((a,b))`. Type construction must
also preserve `int *a[3]` versus `int (*a)[3]`. Qualifiers must survive typedef
resolution and member access. Array/function decay is contextual; retain the
undecayed information needed by address-of and `sizeof`.

`ExpressionChecker` caches expression results and writes AST annotations during
annotation. Original AST `Type` records the intrinsic undecayed type and `LValue` records
the corresponding value category. A separate cached `ExprResult` records the
contextual value conversion/decay. This distinction preserves array identity for
`sizeof` and address-of even when a value use converts the same expression to a
pointer. After typing succeeds, `conversions.cc` rewrites value-use edges with
`ImplicitCastNode` (see below), so the final AST explicitly contains those
contextual conversions. Resolved identifier uses consume their saved bindings rather than
performing a later name lookup that could select a different declaration.
Intrinsic expression/call/type errors and initializer shape/conversion errors are
diagnosed here: they are prerequisites to producing valid annotations. This phase
finalizes and records previously inferred array shape and updates declaration
metadata. Initializer services also support deferred inference when called directly;
the orchestrated pipeline establishes shape earlier for dependent declarations.

Array bounds and enum values are declaration-time dependencies and may already
have been evaluated before this phase, using their original live environment.

### 4. Read-only checks

Consume resolved bindings, AST annotations and queued `ExprResult` values to
validate conditions, returns, defaults, storage requirements, switches and jumps. Diagnostics and
checker-local bookkeeping may change; AST nodes, symbol definitions and type
identities must not change during this phase.

Checks must not silently resolve a missing name, infer an array bound, install a
member, or annotate an expression. Those are earlier-pass responsibilities.
`checkReadOnly()` is a const method returning diagnostics; `check()` appends them
and advances phase state. The final pass does not call `checkValue()`, resolve
types or re-evaluate expressions. Calls to const conversion/layout queries are
permitted. Error metadata prevents a failed annotation becoming an ordinary `int`.

## Work-item context

The queues are `TopLevel`, `Bodies`, `Expressions`, `Flows` and `Switches`.
Registration fills declaration/body work; source-order resolution fills expression,
label/jump and switch work while each body's lexical structure is available.

Each expression/body context retains `scope`, class owner, function result type,
static/const-member flags and a global visibility limit. `select()` restores the
context through `Symbols::selectScope()` and the visibility horizon. Expression
work also records its use (condition, initializer, default, return, switch or case),
expected type, associated symbol and cached result. Flow work stores concrete
control targets and initialized declarations. Switch work indexes the expression
queue so checks use the already annotated condition and cases.

Local names are bound as declarations are inserted in source order. Their saved
binding IDs freeze that decision before later declarations enter the same scope.
Named-loop lookup uses the active enclosing loop stack, not a global label map.

Scope arenas persist after leaving a scope, so attached `ScopeId` and `SymbolId`
values remain meaningful. A saved scope alone would not enforce declaration-point
visibility; the frozen bindings and global visibility horizon provide that rule.

## Validation

The focused suite is in `custom_tests/semantic`. Build and run it with:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./custom_test_hook semantic
./custom_test_hook parse
```

The build directory must first have been configured with `cmake -S src -B build`.
`pass_tests.cc` links the actual lexer, parser and semantic library. CTest registers
the same ten structural scenarios exercised by the Python runner:

| Scenarios | Contract checked |
| --- | --- |
| `registration`, `registration_error` | Top-level/body work collected once; function bindings attached; bodies deferred; duplicate definitions fail registration |
| `resolution`, `resolution_error` | Shadowed locals receive distinct owned names; expression types remain deferred; use-before-declaration fails resolution |
| `annotation`, `annotation_error` | Expression types appear on the AST; work queue exists; invalid floating bitwise operands fail annotation |
| `checks`, `checks_error` | Final checks preserve sampled AST metadata and symbol/type counts; named break has a resolved target; illegal break fails checks |
| `resolution_loop_targets` | Nested labeled/unnamed loop jumps keep distinct target IDs, including across switches; IDs are printed and retained after annotation/checks |
| `resolution_this` | Instance `this` is registered/bound and then annotated as the implicit-object pointer |
| `annotation_scoped_type` | Shadowed typedef identities attach to their original type operands; `sizeof` uses the correct scoped type |

`cases.json` lists 155 end-to-end programs in `cases/`. `run.py` checks each exact
exit status and, for rejected inputs, an error diagnostic from the expected stage.
Crashes and timeouts fail the suite; a rejected input is not accepted merely because
the process exited unsuccessfully. `./custom_test_hook semantic` combines these
programs with eleven phase scenarios, six conversion/metadata scenarios and 32
multiline diagnostic/recovery fixtures in `diagnostics.json`, plus the member-call structural scenario, for **205 checks**. The conversion scenarios inspect the rewritten AST, not just exit status.

The fixtures cover declaration conflicts, types, mutation, calls/defaults,
initializers, returns, members, labels and switches. Positive controls include
recursion, shadowing, compatible typedef repeats, forward goto/function calls,
function pointers, incomplete/self pointers, inheritance, member defaults and named
loop jumps. Dedicated regressions cover declaration-specific defaults, `for`
initializers and early array-shape dependencies, `main(int argc,char **argv)`,
converted duplicate cases, static local
addresses, comma argument nesting and const/type-name dependencies.

The structural final-pass tests compare selected metadata and arena counts; the
const `checkReadOnly()` implementation supplies the broader architectural guard.
These tests are useful evidence, not a proof that every possible syntax form or
standard rule is implemented. [TODO.md](TODO.md) retains the remaining audit scope.

## Declaration-time type formation prerequisites

Explicit array bounds and enum initializers can be needed while registering a declaration's
signature. They form a deliberate prerequisite exception to the broad phase order:
a declaration-time evaluator may resolve and annotate the required expression in
the live declaration environment. This is not the final read-only checker and must
not make unrelated expressions or later declarations visible. Save its annotations
for subsequent phases instead of re-evaluating it in a different scope.

`ASTDeclarations::inferArrayBound()` is another type-formation prerequisite. During
registration/resolution it determines an omitted outer array bound from initializer
**shape**, before later `sizeof` expressions or array-bound dependencies need the
object's completed shape. It handles braced character strings and flat brace-elided
multidimensional arrays; an empty initializer cannot complete an unsized array.
This syntax-driven inference does not annotate general expressions or validate
element conversions. Element typing and scalar/aggregate initializer checks remain
in annotation, including support for scalar braces.

Defaults are different: a default argument belongs to a declaration, not to a
canonical function type. Interning makes unrelated same-signature functions share
a `TypeId`; therefore required argument counts must be keyed by callable/declaration
identity, not solely by that `TypeId`. Calls through a plain function pointer do not
inherit declaration defaults merely because their signature is equal.


## Current language policies and limits

- Target layout is fixed LP64; target selection is not implemented.
- Successful `Types::finishDefinition` stores size, alignment, member offsets and
  base offset in `Type::objectLayout`. Its offset vector follows `Type::members`;
  static/type/function/special members have no instance offset. Completed records
  cannot change storage shape. `layout()` reuses this metadata, including through
  const aliases, and never mutates the type during read-only checks. Failed or
  forward definitions have no stored layout, so dependency completion can be
  retried without retaining stale incomplete results. IR copies these offsets
  into its owned types rather than asking semantics to walk members repeatedly.
- Explicit array bounds must be positive integral constants; VLAs are unsupported.
  Omitted bounds are supported where parameter adjustment or initialization supplies
  their meaning, and final object storage must be complete.
- Globals and local statics permit any initializer accepted by the ordinary
  type/conversion checks. IR zero-fills static storage, executes all explicit
  global initializers in declaration order before entry (including constants),
  and guards local-static initialization for first use. This deliberately differs
  from C++ constant-before-dynamic initialization ordering. A static pointer may
  capture an automatic local's address; its later lifetime is the program's
  responsibility, just as for other escaped pointers.
- Const scalar objects require initialization; aggregate/constructor behavior must
  follow the accepted initialization rules rather than assuming every subobject
  independently requires an explicit source initializer.
- Duplicate active loop labels are rejected. Reusing a label in sibling loops is
  allowed. Goto labels are function-wide; named break/continue target enclosing
  loops, including a trailing do-while label visible in its body.
- Nested function definitions are unsupported. Top-level function forward calls
  are supported; global objects retain declaration-order visibility.
- Parameters require names. `main(void)`, `long long` declarations and `new`/`delete`
  are outside the implemented syntax even though some related tokens/literals exist.
- `main` is an ordinary function during semantic analysis: it has no special
  return-type or parameter-shape restrictions. Normal declaration, call and return
  compatibility checks still apply. Future executable entry-point/ABI requirements
  belong to an explicitly chosen target policy.

Local aggregate definition syntax remains a parser gap: the legacy
`valid_aggregate_shadow` case is excluded rather than being counted as a semantic
success. The arithmetic constant evaluator handles the implemented subset and is
not a complete standard evaluator for every literal format, conversion and target
arithmetic edge case.

The implementation is a C-like/C++-like subset, not a claim of complete C or C++
standard conformance. The comprehensive checklist remains a regression/audit scope;
helpers implementing a rule do not prove every syntax path is covered. Warning
analysis, cleanup lowering and code generation are not implemented by these passes.


## Explicit implicit conversions for lowering

`types/parser/ast/implicitCast.hh` defines `ImplicitCastNode`. The parser never
creates it. During `annotateTypes`, after all work items have been typed,
`ExpressionChecker::materializeConversions()` rewrites actual child/root slots
using `ASTNode::rewriteExpressions()`. This API enumerates **direct expression
slots only**; it does not repeat declaration or statement discovery. Declaration
bounds/enumerator roots remain compile-time expressions; their operands can still
carry conversions. Original work results remain available for final diagnostics.

Every conversion exposes its operand, source/destination `TypeId`, conversion kind,
source span and annotated rvalue result. Kinds distinguish lvalue loads,
array/function decay, integral promotion, numeric conversion, boolean testing,
null-to-pointer, pointer conversion, pointer qualification and constructor conversion. Explicit source
casts remain `CastFactorNode`. No redundant same-type numeric cast is emitted.

For example `double d = s + 1;` with `short s` receives a load and promotion on
`s`, an `int` addition, then an `int -> double` conversion on the initializer.
Assignments keep their left operand as an addressable expression. `sizeof(a)`
keeps the array operand intact, while a value use of `a` receives array decay.
Conditions receive boolean conversions; shifts promote operands independently;
comparisons use common arithmetic/pointer types. Fixed call arguments, default
parameter expressions, return expressions and scalar aggregate/array initializer
elements receive checked destination conversions. Ellipsis arguments receive
integer promotions and `float -> double`. The outer argument-list comma container
is preserved, including the distinction between `f(a,b)` and `f((a,b))`.

Compound assignments become `CompoundAssignmentNode(Target, Value)`. Lowering
must evaluate **Target's address once**, bind it for the Value subtree, evaluate
Value and store it. `CompoundTargetNode` refers to that saved address as an lvalue;
an explicit load/promotion chain uses it in the arithmetic expression, followed by
the conversion back to the stored type. Thus `a[i++] += 2.5` does not duplicate
`i++`. The compound assignment itself denotes the target lvalue under the
project's existing expression rules. Prefix/postfix increments remain primitive
mutation nodes and also evaluate their operand address once.

Calls now retain `ASTSemanticInfo::CallSignature`. Selected fields/methods and
constructors retain `MemberOwner` and one-based `MemberIndex`; a constructor call
also sets `Construction`. `Symbol` identifies ordinary selected callees. These
facts are independent of the expression checker's temporary caches.

`--semantic` prints `Semantic::AST` after successful analysis, making
conversion nodes visible. Parser output still shows the original syntax tree.
The new nodes use the parser's shared `ASTNodeArena` allocation convention; they
are not owned by the checker, and their type IDs require its type arena to remain
alive. See [IR_READINESS.md](IR_READINESS.md) for the lowering boundary and gaps.


## Overload resolution

`overloads.cc` shares one selection algorithm between ordinary calls and explicit
constructor calls. Filter by required argument count (including defaults) and
variadic capacity, then collapse repeated declarations by callable identity.
Canonical type IDs provide the existing equivalence rules; aliases and by-value
top-level const do not distinguish arguments, while pointee qualification does.

First collect candidates matching every supplied argument exactly. Select the
unique match, diagnose multiple matches, or continue if none match. Defaults are
not supplied arguments and never improve a match. Ellipsis operands have no
parameter type, so a call supplying them enters conversion matching. With no
ellipsis operands, `f(int)` and `f(int,...)` can both match exactly and are
ambiguous under this policy.

Conversion matching accepts the existing standard implicit conversions (including
arithmetic, qualification addition, null pointers and compatible/derived pointers)
and a viable converting constructor for an aggregate parameter. Select only a
sole viable candidate. There is no conversion-category preference, promotion
preference, per-argument ranking or rank sum. Multiple viable candidates, or an
ambiguous required constructor conversion, diagnose ambiguity.

A converting constructor has a typed first parameter and accepts one supplied
argument; trailing defaults are permitted. Its own argument can use standard
conversions, but cannot invoke another converting constructor. An exact function
match is selected before probing constructor conversions in other candidates.
This call-argument policy does not add constructor conversions to assignment or
return checking.

`ConstructorConversion` is an explicit implicit-cast AST node. It retains the
selected constructor signature, owner and member index with `Construction=true`.
Its operand already carries the checked conversion to the constructor's first
parameter. The call's supplied argument count and nesting remain unchanged;
constructor/function defaults remain on their declarations for IR expansion.
`conversion_tests.cc` checks these annotations and repeated-declaration identity;
IR fixture `45_overload_conversions` executes direct, indirect and member calls
with constructor defaults and destruction.

## Printed control-flow identities

`semanticInfo().Target` is the control identity: loop, switch and goto-label
nodes define it, while `break`, `continue` and `goto` nodes refer to their resolved
destination. A named jump's label identifier also stores the same Target and is
marked Resolved. This is separate from ordinary variable/function SymbolIds.

AST printing includes `Target: N` on those definitions and uses after resolution.
For nested `for(;;;a)` / `for(;;;b)`, `continue a` prints the outer loop's ID;
an unnamed continue prints the inner loop's ID. Zero IDs are omitted so parser-only
AST output remains unchanged. IR maps a continue's loop ID to its update/condition
block and a break's ID to its exit block; the numeric ID does not itself denote
a machine or IR instruction address.


## Diagnostic source locations and recovery

Parser allocations use `AllocNodeAt(tokenIndex, ...)`: capture the introducing
token before parsing children or bodies. There is no last-consumed-token fallback.
Statement/declaration nodes anchor their beginning, identifiers/literals their own
token, and binary/postfix expression nodes their operator. This prevents composite
nodes from accidentally pointing to a closing brace, semicolon, or child statement.
Semantic conversion nodes inherit the source location of the operand/computation.

Condition, switch, case-value, default-conversion and return-conversion errors
prefer the offending expression. Duplicate active loop labels point at the label
identifier (including a do-while suffix). Declaration conflicts retain both the
new declaration and previous declaration source. Diagnostic regression programs
assert message fragments, exact line/column, the displayed source line and previous
notes, not merely an unsuccessful exit status.

Recoverable local name/type errors do not suppress later independent diagnostics.
For example a local redeclaring parameter `a` and a later `foo(d)` passing an
aggregate to a `char` parameter both produce errors. Unresolved expression uses
produce an error result without repeating the resolver's diagnostic; final checks
skip dependent switch/case facts when their types could not be established. Invalid
top-level registration still stops the pipeline because its declaration/body work
may be incomplete. No erroneous compilation prints a successful semantic AST or
becomes eligible for IR generation.


## Instance-member calls versus ordinary function decay

A selected nonstatic method is a bound designator: its call keeps the receiver
AST and `MemberOwner`/`MemberIndex`/`CallSignature`, without `FunctionToPointer`.
Dot receivers remain addressable; arrow receivers load their object pointer.
Implicit calls retain the member identity and use the current instance context.
Parentheses around a selected method preserve this behavior. Static methods and
free functions still permit ordinary function decay. Function-pointer fields are
indirect callees: their expression remains pointer-typed and receives a pointer
load, rather than being rewritten into a method or function designator.

`ExprResult::boundInstanceFunction` prevents an instance method from silently
becoming an ordinary function pointer in initializers, arguments, bare value uses
or address-of expressions. Pointer-to-member types are not implemented, so these
uses receive a diagnostic. Calling an implicit instance method from a static
context also requires an object. `member_call_tests.cc` inspects AST edges for
explicit/implicit, dot/arrow/parenthesized instance calls and function-pointer
fields, while retaining free/static function decay controls.
