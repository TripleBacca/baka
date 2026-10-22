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
There is no separate expression-region pass, adjacency-based argument stack, or
AST-pointer evaluation cursor.

## Types, storage and identities

IR type IDs, object/function IDs and block IDs are module identities. Temporaries
are local to a function. Zero denotes an absent/invalid identity. Representation
types distinguish void, integer, floating, pointer, array, function and nominal record.
Pointers use the shared target layout. Booleans have one value bit and one storage
byte. Source qualifiers are checked by semantics and erased during import.

Records retain size, alignment, member offsets and optional base offset. Their
nominal identities remain distinct even when names/layouts match. Incomplete records
remain opaque and cannot be used as stored values. Function declarations may mention
opaque record parameters/results; definitions and actual calls require complete values.
Complete record values are snapshots:
`Load` captures bytes, `Copy` copies a value, and `Store` writes the complete value.
Calls and returns keep record types; native ABI classification is deferred.

Objects own a storage type, linkage/storage flags, optional sparse scalar/relocation
initializers and literal bytes. Ordinary source globals are zero-filled and explicit
initializers execute in source order through module startup. A source-const global
that is initialized at runtime must remain writable during that initialization.

Lookup indexes use IDs and vector positions, so copying a module does not retain
pointers into another module. Appending storage is detected by the indexes. Tools
that edit IDs in place must call `invalidateIndexes()`; verification does this before
checking a module. Operand identity is independent of its printed source name.

## Instructions

Scalar TAC retains opcode/result/left/right operands, with opcode-specific builders
for stores, branches and calls. Important operand roles are:

| Instruction | Contract |
| --- | --- |
| `Load` | Result is a typed temporary; left is its storage address |
| `Store` | Result is the destination address; left is the captured value |
| `AddressOffset` | Result is a pointer; left is a pointer; right is an integer byte offset |
| `Call` | Left is a callable pointer; the instruction owns its argument vector; result is absent for void |
| `ZeroMemory` | Result is a destination pointer; left is a byte count; right is absent |
| `Branch` | Left is scalar condition; right is true block; result is false block |
| `Jump` | Left is destination block |
| `Return` | Left is the optional captured result |
| `RegisterDestructor` | Left is a void(T*) callable; right is its receiver |

Arguments are evaluated in source order and captured before the call instruction.
Nested calls therefore have independent argument operands. `Uses()` includes call
arguments and memory destination addresses; calls and memory operations remain
observable during optimization.

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

Optimization consists of block-local constant folding and copy propagation,
reachability removal, and temporary-liveness dead-code elimination. It preserves
stores, bulk zeroing, calls, destructor registration, control effects and potentially
faulting loads/divisions. No alias-based dead-store removal or SSA optimizer is implied.

Source fixtures group related language features, following the lexer/parser suites.
Each IR fixture checks deterministic TAC output and expected results using an
independent interpreter, before and after optimization. The lifetime programs also
exercise construction order, scope cleanup, arrays and static destruction.
See `src/semantic/README.md` for phase contracts and intentional language policies.
