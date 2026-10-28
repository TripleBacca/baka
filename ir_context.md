# Typed TAC implementation

`--ir` runs lexing, parsing, semantic checking, normalization and TAC lowering.
A failed compilation prints source-located diagnostics and never successful TAC.
`--semantic` prints the normalized semantic AST. `make test-ir` and
`ctest --test-dir build --output-on-failure` exercise the implementation.

## Pipeline and ownership

The driver obtains `SemanticAnalyzer::checkedProgram()` after successful checking
and constructs `ir::Lowerer` from that view. The AST and analyzer live through
lowering. Compatibility entry points also validate the exact AST/analyzer pairing.
The resulting module owns all strings, representation types, layouts, object and
function declarations, operands, instructions and CFG metadata.

The semantic execution plans are the lowering contract. Initialization plans map
source expressions to typed operations/subobject offsets; call plans contain selected
callees and expanded default-expression references; lifetime plans describe traits
and ordered subobjects; cleanup plans list objects leaving each jump. Unavailable
or ambiguous construction and prohibited lifetime-entry jumps are semantic errors.

Lowering imports declarations, emits ordinary functions, emits requested special
members and destructor helpers, emits module startup, builds CFGs and verifies the
result. Optional optimization runs after verification and is followed by verification.
The normal CLI emits unoptimized TAC.

## Code organization

| Source | Responsibility |
| --- | --- |
| `src/base/target.hh`, `numeric.*` | Shared LP64 widths/layout constants and compiler numeric evaluation |
| `src/semantic/execution_plans.*`, `*_plans.cc` | Typed initialization, call, construction and cleanup decisions |
| `src/ir/ir.*` | Owned data model, indexed lookup, operand-use/definition queries |
| `lowering.*` | Checked entry point, type/symbol import, builder primitives and per-function context |
| `expressions.cc` | Expression dispatch and explicit/implicit conversion emission |
| `primary.cc`, `addresses.cc` | Literals, names, receivers, member and indexed locations |
| `operators.cc`, `conditional.cc`, `calls.cc` | Arithmetic, mutation, conditional CFG and calls |
| `statements.cc`, `functions.cc` | Statement dispatch, lexical scope cleanup and function bodies |
| `iteration.cc`, `selection.cc`, `jumps.cc` | Loops, branches, switch dispatch and planned jump cleanup |
| `initializers.cc` | Emit already-published initialization steps |
| `lifetimes.cc` | Special-member bodies, activation flags, temporary ownership and array loops |
| `runtime_initialization.cc` | Ordered global startup and first-use static guards |
| `cfg.cc`, `verifier.cc` | Edges, linearization, structural/type validation and temporary dataflow |
| `optimization.cc`, `constant_folding.cc`, `dead_code.cc` | Substitution, numeric folding and liveness-based removal |
| `printer.cc` | Deterministic TAC output |

There is one structured expression/statement emitter. Expressions emit into the
current block and may leave the insertion point in a new block after a branch.
There is no separate expression-region pass or AST-pointer evaluation cursor.
Call arguments use a contiguous, block-local `param` sequence followed by `call`.

## Types, storage and identities

IR type IDs, object/function IDs and block IDs are module identities. Temporaries
are local to a function. Zero denotes an absent/invalid identity. Representation
types distinguish void, integer, floating, pointer, array, function and nominal record.
Pointers use the shared target layout. Booleans have one value bit and one storage
byte. Source qualifiers are checked by semantics and erased during import.

Records retain field types, size, alignment, member offsets, union identity and an
optional base type/offset. Their nominal identities remain distinct even when
names/layouts match. Incomplete records
remain opaque and cannot be used as stored values. Function declarations may mention
opaque record parameters/results; definitions and actual calls require complete values.
Complete record values are snapshots:
`Load` captures bytes, `Copy` copies a value, and `Store` writes the complete value.
Calls and returns keep record types; native ABI classification is deferred.

Function-style `new(p, args...)` lowers to `__baka_allocate(size, alignment)`
through ordinary `param`/`call` instructions. The first pointer variable supplies
the object or fixed-array type without loading its value. A successful allocation
is followed by typed stores or constructor calls. `delete(p)` invokes destructors
before calling `__baka_deallocate(ptr<void>)`; null skips both. Allocation failure
returns null and skips initialization. Heap objects are not registered for scope
cleanup or module-exit destruction.

The printed IR describes one translation unit directly, without a `module { ... }`
wrapper. It starts with named aggregate definitions. References use the declared
name rather than repeating an internal numeric record ID:

```text
X = record(i32, i32, ptr<i32>) size 16 align 8
Node = record(i32, ptr<Node>) size 16 align 8
Derived = record(X, i32) size 24 align 8
Number = union(i32, f64) size 8 align 8
Opaque = opaque
```

Fields appear in storage order; an inherited base is the first subobject. Methods,
static members and typedef declarations occupy no per-object storage and are not
included. Typedef aliases share their underlying representation type. Recursive
pointers may refer to names declared elsewhere in the module. Unions use `union(...)`
to distinguish overlapping fields from sequential record fields. Duplicate source
names receive a `.typeID` suffix; anonymous records and names colliding with primitive
IR type names also receive unique names. Internal TypeIds still index the module's
owned type table. Verification checks field/base types, offsets, bounds and rejects
recursive by-value containment while accepting recursive pointers.

Objects own a storage type, linkage/storage flags, optional sparse scalar/relocation
initializers and literal bytes. Ordinary source globals are zero-filled and explicit
initializers execute in source order through module startup. A source-const global
that is initialized at runtime must remain writable during that initialization.

Lookup indexes use IDs and vector positions, so copying a module does not retain
pointers into another module. Appending storage is detected by the indexes. Tools
that edit IDs in place must call `invalidateIndexes()`; verification does this before
checking a module. Operand identity is independent of its printed source name.

## Instructions

Every instruction is a strict typed quadruple `(opcode, result, left, right)`;
there is no extra argument payload. The result field is used only for a produced
temporary. Important operand roles are:

| Instruction | Contract |
| --- | --- |
| `Load` | Result is a typed temporary; left is its storage address |
| `Store` | No result; left is the destination address; right is the captured value |
| `AddressOffset` | Result is a pointer; left is a pointer; right is an integer byte offset |
| `Param` (`param`) | No result; left is one captured argument value; right is absent |
| `Call` | Left is a callable pointer; right is absent; result is absent for void |
| `ZeroMemory` | No result; left is a destination pointer; right is a byte count |
| `JumpIf` (`if_goto`) | No result; left is a scalar condition; right is the true block |
| `Jump` (`goto`) | No result; left is the destination block |
| `Return` | Left is the optional captured result |
| `RegisterDestructor` | Left is a void(T*) callable; right is its receiver |

The callee is captured first, then arguments are evaluated and captured in source
order. Only after all evaluations finish are the `param` instructions emitted,
immediately followed by `call`. For `f(a, g(b))`, the essential sequence is:

```text
t1 = load a
t2 = load b
param t2
t3 = call g
param t1
param t3
t4 = call f
```

Each call consumes its entire preceding param sequence, in order, with no explicit
argument-count operand. The printed callee is just its function name or indirect-call
temporary; its signature remains in the typed IR and function declaration.
Zero-argument calls have no params. No instruction or block boundary may interrupt
a param sequence.
This includes calls through function pointers and constructor/destructor calls;
instance methods pass `this` as their first param. Records remain captured values.
The verifier checks grouping, arity against the callee signature, fixed argument
types and result types; variadic extras are ordinary params after semantic promotions.

A conditional block ends with `if_goto condition, true_block` followed by
`goto false_block`. Both instructions form its control-flow tail; the verifier
rejects conditional jumps elsewhere. CFG construction includes both successors,
and constant folding reduces the pair to one goto. There is no implicit fallthrough
between blocks.

The printed function body is a flat instruction listing. Only jump destinations
receive labels such as `L2:`; straight-line code has no labels. Predecessor/successor
lists and basic-block headers are omitted. The in-memory IR retains blocks and CFG
edges for verification, optimization and code generation.

`Uses()` includes param values and memory destination addresses; params, calls
and memory operations remain observable during optimization. Unary negation,
bitwise complement and logical NOT retain their dedicated opcodes.

The opcode audit retains typed byte offsets, bulk zeroing and destructor registration:
each already fits the operand limit, and expanding them would add casts, loops or
an otherwise unnecessary runtime helper interface.

Temporaries are mutable rather than SSA values. Conditional results and lifetime
flags can have multiple definitions. Verification requires a definition along every
incoming reachable path before a use. Every block ends with an explicit terminator.
CFG edge lists are derived from terminators and checked for consistency.

## Numeric contract

The fixed LP64 target is defined in one header. `base/numeric.hh` is shared by
semantic constant evaluation, explicit conversion facts and IR folding. Integer
arithmetic wraps at its declared width; comparisons consume converted operands;
boolean casts test truth. Floating operands/results round at their declared width.
Unavailable folds and out-of-range floating casts remain explicit operations.

The test interpreter is intentionally independent. It poisons unwritten automatic
storage, zeroes static storage, and compares optimized/unoptimized execution. This
prevents a missing automatic initialization store from passing solely because the
old interpreter happened to return zero for unwritten bytes.

## Construction, ownership and cleanup

Semantic lifetime plans select default constructors and validate their availability
against ordered base/member initialization. Unused unavailable implicit constructors
are not emitted. Explicit converting constructor identity is retained during checking
and consumed during normalization, not selected again by the conversion writer.

The lowerer assigns storage and explicit receiver parameters. Construction destinations
allow a prvalue to initialize its final object directly. Temporary ownership tracks
full-expression destruction and ownership transfer to parameters/results. Runtime
activation flags are initialized in the entry block, enabled after construction and
cleared during cleanup, supporting skipped branches and repeated loops.

Semantic cleanup lists determine the objects leaving return/break/continue/goto
edges. Normal lexical exit still releases active objects in reverse order. Array
construction/destruction unrolls small arrays and uses IR loops for larger arrays;
omitted initialization ranges use one bulk-zero operation. Plan size and emitted
code do not scale with a huge zero-filled suffix.

Startup runs once before entry. Local static guards publish completion after
initialization and temporary cleanup. Destructor callbacks are registered after
successful initialization and invoked in reverse order after entry returns. Exceptions,
thread-safe/reentrant guards and active-member tracking for nontrivial unions are
outside the current subset.

## Validation and optimization

The verifier validates operand kinds/IDs before opcode-specific dereferences,
rejects malformed structural type cycles, checks calls and stored values, compares
CFG edges with terminators, and performs definite-assignment dataflow for temporaries.
A small verifier test checks missing terminators and uses of undefined temporaries.

Optimization repeats local constant folding and copy propagation, CFG-wide
copy/constant propagation, conservative integer identities and local value numbering,
branch simplification, jump threading, reachability removal and temporary-liveness
dead-code elimination until stable. It preserves stores, bulk zeroing, calls,
destructor registration, control effects and potentially faulting loads/divisions.
There is no alias-based dead-store removal or SSA optimizer.

Source fixtures group related language features, following the lexer/parser suites.
Each IR fixture checks deterministic TAC output and expected results using an
independent interpreter, before and after optimization. The lifetime programs also
exercise construction order, scope cleanup, arrays and static destruction.
See `src/semantic/README.md` for phase contracts and intentional language policies.
