# baka typed TAC

## Build, run, test

```sh
cmake -S src -B build -DBUILD_TESTING=ON
cmake --build build --parallel 2
./build/baka --ir path/to/input.cpp
make ir file=path/to/input.cpp
./custom_test_hook ir
python3 -B custom_tests/ir/run.py 03_calls
./build/ir_pass_tests 03_calls
ctest --test-dir build --output-on-failure
./custom_test_hook
```

`--ir` runs the existing lexer, parser and all semantic passes first. An error
in any frontend stage prevents lowering. The default compiler stage remains
semantic analysis. `--codegen` retains its previous placeholder behavior; this
milestone does not emit machine code or assembly.

Successful nonverbose `--ir` prints only `IR::TAC:` followed by the module.
`--verbose --ir` also prints the existing frontend debug output. IR failures use
the existing source/caret diagnostic renderer with stage `(IR)`, return status 1,
and do not print a partially lowered module. Earlier stages retain their output.

`--ir` prints **unoptimized TAC**. The C++ lowering API defaults to
`optimize=false`; callers can request `optimize=true` or invoke the optimization
passes explicitly. Structural tests exercise both forms. No new CLI
optimization flags or third-party dependencies are introduced.

## Files and interfaces

| File | Responsibility |
| --- | --- |
| `src/ir/ir.hh`, `ir.cc` | Owned IR data model, operand-use/definition helpers, type interning |
| `lowering.hh`, `lowering.cc` | Checked-AST entry point, frontend-to-IR type/symbol import, builder primitives |
| `expression_regions.cc` | First pass: collect runtime roots and emit expression regions |
| `expressions.cc` | Expression/address emission, calls, conversions, saved compound targets |
| `statements.cc` | Second pass: connect regions and lower statements/control flow |
| `initializers.cc` | Scalar/aggregate initializer planning and stores |
| `runtime_initialization.cc` | Ordered global startup function and guarded local statics |
| `lifetimes.cc` | Constructor/destructor work items, member initialization, cleanup and ownership |
| `cfg.cc` | Third pass: predecessor/successor construction; flat linearization |
| `verifier.cc` | Fourth pass: read-only structural/type/temporary verification |
| `printer.cc` | Deterministic textual typed quadruples |
| `constant_folding.hh`, `optimization.cc` | Fifth pass: constant folding, local copy propagation, CFG-aware DCE |
| `custom_tests/ir/` | Golden fixtures, negative diagnostics, structural tests and test-only executor |

The public lowering entry point is:

```cpp
explicit Lowerer(const semantic::SemanticAnalyzer& analyzer);
bool lower(const types::ProgramNode& program, bool optimize = false);
const Module& module() const;
const std::vector<Diagnostic>& diagnostics() const;
Module takeModule();
```

Use one `Lowerer` per program. Its analyzer must have reached `Checked` with no
diagnostics. On failure, inspect diagnostics and discard the partial module.
`lower` does not catch unrelated programming errors or reinterpret a failed
frontend as a valid program.

The driver invokes lowering inside the live `SemanticAnalyzer` scope. The
builder borrows const frontend references and copies everything retained in its
result. `Module` contains no AST pointers, string views, semantic IDs requiring
arena lookup, or references to source memory. A module can be moved out and
printed, verified, optimized, or consumed after those frontend objects die.

Public downstream passes:

```cpp
void BuildCFG(Function& function);
std::vector<FlatInstruction> Linearize(const Function& function);
std::vector<Diagnostic> Verify(const Module& module);
void ConstantFold(Module& module);
void CopyPropagate(Module& module);
void EliminateDeadCode(Module& module);
void Optimize(Module& module);
void Print(const Module& module, std::ostream& output);
```

`Optimize` expects valid IR and rebuilds CFG metadata. `Verify` is read-only.
`Print` expects valid IR; it is not a recovery printer for corrupt modules.
`Linearize` copies instructions in stored block order and preserves the owning
block ID beside each instruction. All transfers are explicit, so physical block
order never implies fallthrough. A backend emitting a flat stream must emit a
label at each change of owning block ID.

## Owned representation

- `Module`: its own type table, global/static objects, function declarations and
  definitions, and an optional `initializer` function ID. Type index zero is
  invalid. Record identities remain nominal.
- `Function`: ID, owned name, function-type ID, linkage flag, parameter object
  IDs, local objects, temporary-type table and basic blocks.
- `Block`: function-local ID, instruction vector, predecessors and successors.
- `Instruction`: `(opcode, result, left, right)` plus copied source line/column.
- `Operand`: none, temporary, object address, function address, integer literal,
  floating literal or block label. Value operands have an IR type ID. Labels do
  not. Temporary index zero is invalid; no fake integer type is used for errors.
- `Object`: storage type, identity/name/linkage and optional static initializer.
  `external` is available in the model; current source objects are emitted as
  definitions because the frontend has no `extern` declaration facility.

Source symbol numbers are copied as stable object/function identities, but the
module never resolves them through semantic tables. Synthetic string objects use
IDs above every source symbol. Printed suffixes disambiguate shadowing and
overloads. These names are an IR identity convention, not a platform ABI or
linker mangling scheme.

Types are `void`, signed/unsigned integers, floating values, pointers, arrays,
functions and records. Text examples are `i32`, `u64`, `u1` (bool), `f32`, `f64`,
`ptr<i32>`, `[4 x i32]`, `fn(i32, ...)->i32`, `record#N(Name)`. Record definitions
print semantic size and alignment; incomplete forward records print `opaque`.
Owned completeness flags distinguish incomplete types from complete empty records.
Sizes/offsets come from the existing fixed
LP64 layout; pointers are 8 bytes. Semantic `Type::objectLayout` is computed once
when a definition successfully completes, with no lazy writes during later passes.
The IR type owns size/alignment, `memberOffsets` (indexed like semantic members,
with no offset for static/type/function/special declarations), and `baseOffset`.
Field addressing, inheritance adjustments, aggregate initialization and lifetime
lowering read these stored offsets; they do not recompute aggregate layouts.
The compatibility `Types::layout()` query also reuses completed record metadata.
Forward and failed definitions retain no layout until completion succeeds.
Const qualification is erased after semantic checking; qualification-only casts
therefore need no runtime instruction. Readonly string literals retain a storage
hint. Runtime-initialized const source objects print `readonly-after-init`:
this describes source-level constness, not permission to freeze their physical
storage. Backends must permit initialization and destructor-body writes. No semantic type interning or
symbol mutation occurs.

Variables have mutable memory storage. Loads produce temporaries; assignment
uses explicit stores. Temporaries are non-SSA: merge results are assigned on
multiple incoming paths. There are no phi nodes, hidden stack operations, or
implicit memory reads attached to operands.

Integer literals store low bits in `uint64_t`; printing interprets signed types
at their declared width. Floating literals use `long double` storage with their
source/destination f32/f64 precision retained. A symbolic relocation uses the
operand's integer slot for a signed byte addend.

## Quadruple contracts

Every instruction prints four slots; `_` denotes an unused slot.

| Opcode | `(result, left, right)` |
| --- | --- |
| `copy` | destination temporary, value, `_` |
| `load` | destination temporary, typed address, `_` |
| `store` | typed destination address, scalar value, `_` |
| `cast` | destination temporary, source value, `_`; result type defines the conversion |
| `offset` | destination pointer temporary, base pointer, signed byte offset |
| `add/sub/mul/div/rem` | destination temporary, left value, right value |
| `shl/shr/and/or/xor` | destination temporary, left value, right value |
| `eq/ne/lt/gt/le/ge` | integer/bool destination, compatible left/right values |
| `neg/complement/not` | destination temporary, operand, `_` |
| `arg` | `_`, captured argument value, zero-based argument index |
| `call` | result temporary or `_` for void, callable pointer, argument count |
| `jump` | `_`, destination label, `_` |
| `branch` | **false label**, condition value, **true label** |
| `ret` | `_`, return value or `_` for void, `_` |

A readable example from the pointer/array lowering pattern:

```text
(cast, %t1:ptr<i32>, &a.12:ptr<[3 x i32]>, _)
(offset, %t2:ptr<i32>, %t1:ptr<i32>, 4:i64)
(load, %t3:i32, %t2:ptr<i32>, _)
(ret, _, %t3:i32, _)
```

`offset` is byte-based, never element-based. Array indexing and pointer arithmetic
multiply an index by the pointee size first. Pointer difference converts both
addresses to LP64 integers, subtracts, then divides by element size. Member
access emits the selected member's byte offset, including inherited base offsets.
Pointer arithmetic on a zero-sized/unsized pointee is diagnosed rather than
inventing a stride.

A cast to bool tests nonzero, including pointer values. Integer casts sign/zero
extend according to the source type and truncate to destination width. Integer
to pointer casts preserve the source's extended low address bits. Floating
conversions round to the destination floating precision; float-to-integer casts
truncate toward zero when representable. Pointer casts preserve the address;
qualification-only casts disappear. The IR cast-to-void mechanism evaluates
its operand for side effects and produces no value; the current frontend rejects
source casts to void, so that mechanism is not currently reachable from source.

Integer add/subtract/multiply/negate wrap at their result width, including signed
operations. Signed division of minimum/-1 wraps to minimum; its remainder is
zero. Signed right shift is arithmetic, consistent with the current frontend's
constant evaluator. Division by zero, invalid shift counts and out-of-range
float-to-integer conversions are not folded; the IR does not invent a recovery
value or decide a future machine backend's trap policy for them.

## Ordered lowering passes

1. **Expression lowering.** After importing each function's storage/signature,
   collect its runtime expression roots once in source order. Bounds, case tags
   and sizeof operands are not runtime roots. Each root is lowered into an
   independent entry/exit region and a result operand. Nested short-circuit and
   ternary expressions already contain their internal control flow. These
   regions remain builder metadata, not AST annotations.
2. **Statements and control flow.** Connect expression regions at their runtime
   sites, then add statement edges/terminators. Initializer stores consume the
   planned scalar regions. A region may be connected only once syntactically;
   loops reenter its existing blocks through CFG edges. Declarations, labels and
   loop targets are not re-resolved.
3. **CFG.** Rebuild predecessor/successor vectors from terminators. Every block
   ends in jump, branch or return, including disconnected lexical regions.
4. **Verify and print.** Verify the complete unoptimized module before it can
   reach optimization. The public printer is separate. The driver prints only
   after successful verification of the final module.
5. **Optimize.** Fold constants, propagate copies and eliminate dead code, rebuild
   CFG, then verify again. These passes are available through the C++ API; the CLI
   currently stops at verified unoptimized TAC.

The expression/statement split deliberately does not hoist runtime evaluation:
regions are generated early but execute only where the statement pass connects
them. Building an IR region never executes a source expression.

### Evaluation and address rules

- Evaluation is left-to-right: lvalue address before assignment RHS, binary left
  before right, callee/receiver before arguments, arguments in source order.
- Arguments are captured into values before any `arg` quadruples are emitted.
  Nested calls therefore cannot interrupt an outer argument sequence. The
  parenthesized comma-expression distinction in the AST is preserved.
- Compound assignment pushes its target address on a builder stack. Semantic
  `CompoundTargetNode` loads that saved address. Nested compounds use nested
  stacks. The target expression is never duplicated.
- Prefix/postfix mutation saves an address once, loads the old value, computes
  and stores the new value. Postfix returns the old value. Baka assignment and
  prefix mutation are lvalues; an outer lvalue-to-rvalue conversion reloads the
  saved location after its write without reevaluating that location.
- `&&`/`||` use branches and a mutable result temporary; only the required RHS
  path executes. Ternary uses two arms and a join. Comma emits children in order.
- Direct functions are symbolic callable operands. Function pointers, including
  fields, are loaded and called indirectly. An instance call uses a copied
  function signature with an explicit first receiver pointer; ordinary parameters
  follow it. Definitions expose the corresponding `this` parameter. Implicit,
  dot, arrow and inherited receivers use the same convention. Static member
  calls evaluate their explicit base but pass no receiver.
- `break`/`continue` map semantic `Target` IDs to destination blocks. A for-loop
  continue targets the update, while/do continue targets the condition. Named
  nested-loop targets use the resolved ID directly, not a name lookup.
- Switch evaluates its condition once, dispatches by comparison chains, and
  preserves lexical case fallthrough. Case collection skips nested switches.
  It also finds labels nested under blocks, conditionals and loops.
- Goto labels are function-wide and allocated on demand, supporting forward and
  backward goto. Lexical code after a terminator starts disconnected blocks;
  later labels can make it reachable. DCE removes truly unreachable blocks.
- Falling off a function emits plain `ret` for void or typed zero for a scalar,
  pointer or record result. Record zero is assembled in local storage and loaded
  as a typed value. Frontend `main` rules are unchanged.

### Aggregate values and calls

Complete records (structs, classes and unions) are first-class typed TAC values.
Function signatures, `arg`, `call` results and `ret` preserve their record types.
There are no hidden result parameters. ABI lowering later decides whether a value
travels in registers or memory, and whether a hidden result pointer is required.

`load` of a record captures a value snapshot; `copy` copies that snapshot and
`store` writes it into independent destination storage. These operations cover
the entire record, including nested arrays and records, at the semantic LP64
size. They impose no eight-byte/scalar size limit. This is essential for
left-to-right calls: loading an earlier aggregate argument captures its contents
before later argument expressions can mutate the original object. Callee
parameter objects receive independent copies. Record-valued returns remain
valid after the callee's frame has ended.

Member access or a bound method call on a record prvalue materializes its value
in a synthetic `.materialized` local object. This does not change the call
signature. Same-type explicit casts retain the record value. Derived-to-base
initialization materializes a captured derived value in `.slice` storage and
loads its base subobject using the semantic layout offset. Conditional record
values use typed mutable temporaries and the existing branch/join pattern.

These value operations also apply to records with lifetime machinery. The
test-only interpreter represents record values as independent byte vectors, so
the tests verify value independence rather than accidentally sharing addresses.
The authorized frontend dereference correction permits `return *p` for a record
pointer. Dereference preserves the qualified pointee lvalue; scalar-condition
checking runs only in actual condition contexts. Non-pointers and void pointees
remain errors, and const member writes remain errors.

### Storage and initialization

Globals and local statics have zero-filled module storage. Every explicit global
initializer emits runtime TAC in declaration order, including `int x = 3 + 4`
(`add` then `store`) and `int x = calc()` (`call` then `store`). An owned internal
`.module_init` function contains this code. The printed module's
`initializer @.module_init.ID` directive requires the eventual runtime/backend to
invoke that function once after allocating/zero-filling static storage and before
entry. The test-only interpreter implements that contract. The compiler currently
prints TAC; it does not execute startup code itself.

This is the user-selected language policy, deliberately different from C++'s
constant-before-dynamic initialization. For `int get(); int a = get(); int b = 7;
int get() { return b; }`, startup assigns `a = 0` before assigning `b = 7`.

Local statics emit a branch on a private zero-filled guard at their declaration.
Only the first-use path evaluates the initializer and stores its result; the
guard is marked complete afterward. Skipped branches do not initialize their
statics, loop visits and later calls do not repeat initialization. Globals may
call functions containing local statics during startup. Guards are ordinary
single-threaded TAC storage: threads, exceptions and recursive initialization of
the same static are unsupported. Completed constructions register destructor
callbacks, as described below.

Source objects with explicit initializers have `runtimeInitialized = true` and
no pre-filled initializer data. The IR model still supports sparse constant data
for future/generated uses. String literals own readonly backing bytes; storing
their addresses or copying character arrays is emitted as runtime TAC.

Local scalar/array/record brace initializers are planned in semantic subobject
order. Nested braces, brace elision and omitted scalar elements are handled;
omitted elements receive typed zero. Union initialization uses its first
instance member. Uninitialized automatic objects retain the frontend's
uninitialized-storage behavior; lowering does not insert unsolicited checks.
Record copying is supported for automatic and static objects. Runtime scalar arithmetic/conversions use the ordinary expression
emitter, including wrapping semantics and explicit casts; constant initializer
expressions are not folded unless the caller explicitly requests optimization.

### Constructors, destructors and lifetime edges

Lifetime registration imports the semantic record members before emitting any
body. Every explicit constructor/destructor gets an owned synthetic IR function
with a first `this` pointer and a void result. Constructor parameters follow
`this`; semantic `MemberOwner`/`MemberIndex` select explicit construction calls.
Defaults come from the selected constructor's checked parameter AST, and ordinary
argument evaluation/conversion rules apply. No semantic table or AST is changed.

Implicit aggregate argument conversions arrive as
`ImplicitCastNode(ConstructorConversion)` with the selected constructor metadata.
They share explicit construction lowering, including trailing default arguments,
aggregate snapshots and temporary cleanup/parameter ownership transfer. Overload
selection is exact-first, then sole viable conversion with no ranking; IR never
reselects an overload. Fixture `45_overload_conversions` checks execution through
direct, indirect, instance, static and function-pointer-field calls.

Synthesized default constructors initialize bases first, then instance members
in declaration order. Explicit constructors get the same prologue before their
user body. Default member initializers execute with the destination object's
receiver, including references to already initialized fields. Arrays construct
elements in increasing index order. Missing scalar default initializers leave
automatic storage uninitialized. Explicit value initialization `A()` through an
implicit default constructor first zeroes scalar subobjects, then applies default
member initializers. Aggregate braces use explicit supplied fields,
then default member initializers or zero for omitted fields. Each use of a
shared default-member AST gets a separate expression region and receiver context.

Destructors execute the user body, then destroy instance members in reverse order
and finally the base. Explicit `return;` follows the same epilogue. Records with
destructible subobjects get synthesized destructors even without an explicit
destructor. Array destructor wrappers visit elements in reverse index order.

Automatic objects/parameters with nontrivial construction or destruction have a lexical cleanup entry and
a mutable boolean temporary initialized to false at function entry. Completion
of initialization marks it true. Cleanup branches on the flag, clears it, and
calls the destructor. This separates scope membership from whether construction
actually ran on the current path. Normal block exits destroy objects in reverse
order. Returns capture their value before destroying locals/parameters. Loop and
switch jumps destroy exactly the scopes they leave. A for-loop's shared exit
destroys its initializer object; continue preserves that object. Resolved semantic
target IDs handle named jumps. Goto target planning records live lexical objects
at each label, including backward targets; jumping into an uninitialized
nontrivial lifetime is diagnosed even if the type has no destructor. Switch
dispatch gets the equivalent check.

Construction prvalues have storage and full-expression cleanup flags. Initializer
destination hints construct directly into variables, members and selected ternary
arms, avoiding a second destructor for a copied temporary. Otherwise a temporary
is destroyed after the enclosing store/expression/condition finishes. Conditional
arms have separate flags, so only constructed objects are destroyed. Comma
expressions preserve all temporary lifetimes until full-expression end. Method
calls on prvalues reuse their object storage, so destructor bodies see mutations
made by the method. Same-record casts and mixed lvalue/prvalue conditionals create
the required temporary copies. Passing a prvalue by value or returning it transfers
cleanup ownership; ordinary lvalue copies still create independent object values.
Early TAC retains aggregate-valued `call`/`ret`; machine ABI lowering remains later.

Static-duration objects register cleanup after initialization completes:

```text
(register_dtor, _, @A.dtor.ID:ptr<fn(ptr<record#N(A)>)->void>, &object.ID:ptr<record#N(A)>)
```

The runtime must push each `(callable, receiver)` and invoke callbacks in reverse
registration order after entry returns. This handles a local static first
constructed during another global's initialization correctly; fixed declaration
order finalizer lists would not. Registration follows object construction but
precedes full-expression temporary destruction: a temporary's destructor may
itself cause another static object to be constructed. A local-static guard
prevents repeat registration.
The test executor drains this stack and exposes post-shutdown globals for checking
destructor effects. The verifier checks callback signature and receiver types.
Registration is effectful and survives all optimization passes.

### Verifier and optimization safety

Verification checks owned types/widths, object/function/temporary identities,
operand/instruction types, scalar memory operations, call argument indices and
counts/signatures, returns, destructor-registration signatures, valid branch destinations, terminators and CFG
symmetry. A forward must-definition analysis intersects predecessor definition
sets, so a mutable temporary used at a join must be defined on every reachable
incoming path. Object-memory initialization is not a verifier requirement.

Constant folding uses unsigned host arithmetic plus explicit target-width masks
to implement signed wrapping without host signed-overflow UB. It folds integer
arithmetic/comparisons, safe scalar constant casts and constant branches. Finite constant floating arithmetic is evaluated at its declared f32/f64 width.
Symbolic pointer relocations and unsafe divisions/shifts remain instructions.
No aggressive floating-point algebra is performed.

Copy propagation is basic-block-local and tracks temporary redefinitions,
invalidating aliases when their source changes. It does not propagate mutable
memory through stores or calls. DCE computes CFG reachability and backward
interblock temporary liveness. It removes unused pure temporary definitions;
it preserves stores, calls, terminators and potentially faulting loads/divisions.
It makes no aliasing assumptions and never reorders instructions.

## Current limitations

- Nonempty brace initialization of a class with explicit constructors lacks
  selected-constructor metadata in the checked AST. IR diagnoses it; use
  `A value = A(arguments)` instead. Empty braces can select a default constructor.
- Nontrivial union members are rejected by the frontend; IR also guards against
  missing active-member metadata. Cleanup does not guess the active union field.
- The parser cannot start a standalone expression statement with `A(arguments)`
  when `A` is recognized as a type. `0, A(arguments);` exercises a discarded
  construction through the existing expression grammar. No parser behavior was changed.
- Omitted function and method arguments are expanded from the selected semantic
  symbol's checked default expressions. They retain declaration-site name binding
  and are evaluated at each call, after explicit arguments, from left to right.
  This also supports defaults supplied by an earlier function declaration.
- No exception model, ABI/register allocation, assembly emission,
  native execution, linker or machine calling convention is implemented.
  Receiver-first calls are an IR contract for a later backend to implement.
- Address-of arrays preserves the entire array type: `&a` is pointer-to-array,
  while ordinary value uses of `a` still decay to pointer-to-element. Dereferencing
  a pointer-to-array preserves its array lvalue for sizeof and address-of; indexing
  performs value-context decay as needed. Pointer arithmetic scales by the full
  array size. This frontend bug was fixed in a follow-up to the initial milestone.
- Local aggregate type definitions, new/delete, references, namespaces and other
  syntax rejected by the current frontend cannot be enabled by `--ir`.
- The existing GCC build is the tested compiler configuration. Existing AST
  header portability issues with Clang were not repaired in this IR work.

## Regression coverage

Golden `.expected` files cover scalar conversions, all control-flow families,
arrays/member offsets, inheritance, direct/indirect/bound calls, globals/statics,
relocations, operator families, wrapping and expression sequencing. Negative
fixtures require status 1, an `(IR)` diagnostic and no partial TAC output; a crash
never counts as expected rejection. The runner never blesses snapshots itself.

Structural C++ tests use the actual lexer/parser/analyzer and assert that AST
printing, semantic type count and symbol count do not change. Modules are used
after the frontend lifetime ends. The test-only executor runs both unoptimized
and optimized TAC on concrete inputs, checking actual control flow, byte memory,
conversions, nested calls, single evaluation and side effects. Additional tests
exercise malformed IR rejection, CFG edges, temporary redefinitions and folding
at the signed-overflow boundary. The executor is only test infrastructure and is
not linked into the compiler.

Run all existing custom suites and CTest as shown above; legacy audit checks can
also be run with:

```sh
python3 -B semantic_audit/teammate_a/run.py --compiler ./build/baka
```

The new library follows the adjacent semantic library's four-space formatting
configuration, copied to `src/ir/.clang-format`. Check it without changing files:

```sh
clang-format --dry-run --Werror src/ir/*.hh src/ir/*.cc
```

## Verified results for this implementation

- `cmake -S src -B build -DBUILD_TESTING=ON` and
  `cmake --build build --parallel 2`: passed.
- `./custom_test_hook`: **427 passed, 0 failed**, including 98 IR checks
  (45 positive goldens, 4 negative diagnostics, 49 structural scenarios).
- `ctest --test-dir build --output-on-failure`: **71/71 passed**.
- `python3 -B semantic_audit/teammate_a/run.py --compiler ./build/baka`:
  **17/17 passed**; the runner's pre-existing aggregate-shadow exclusion remains.
- `clang-format --dry-run --Werror src/ir/*.hh src/ir/*.cc`: passed.
- The initial IR implementation left frontend sources unchanged. The authorized
  array-address follow-up fixes semantic address-of and array dereference typing,
  with six source regressions, a semantic metadata test and an IR golden/execution
  test covering const arrays, multidimensional arrays, sizeof and full-array strides.
  Lexer/parser files and `context_A/B/C.md` remain unchanged by this follow-up.
