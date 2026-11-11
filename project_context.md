# Baka project context

This is a self-contained account of the implemented compiler on `saturn/slop`
at commit `f6227df` (25 October 2026), extended on `saturn/slop2` with the MIPS64
backend and block-local optimizer (12 November 2026). It covers the source language, every
compiler layer, semantic/IR contracts, runtime behavior, validation, and limits.
The source is authoritative if later changes diverge.

## Project and build

Baka is a C/C++-like subset implemented in C++23. It lexes, parses, checks and
normalizes semantics, lowers to typed three-address code (TAC), and generates
MIPS64 assembly (n64 ABI, little-endian). `src/main.cc` enters
`src/driver/driver.cc`. `src/lexer` tokenizes; `src/parser` builds the AST;
`src/semantic` checks it; `src/ir` lowers, verifies, optimizes and prints TAC;
`src/codegen` selects instructions, allocates registers and emits assembly.
`src/types` contains token and parser-AST definitions. `src/base` contains
target arithmetic, source indexing, memory and shared utilities.

```sh
cmake -S src -B build
cmake --build build --parallel 2
./build/baka --ir path/to/input.cpp
ctest --test-dir build --output-on-failure
./custom_test_hook lex parse
```

`--lex`, `--parse`, `--semantic`, `--ir` and `--codegen` stop at the named
stage. The default stops after semantics. `--semantic` prints the normalized
AST; `--ir` prints raw TAC after successful checking and lowering (optimized
with `--optimize`); `--codegen` prints MIPS64 assembly (unoptimized with
`--no-optimize`). `--verbose` adds stage output. Errors are source-located and
prevent successful output. CMake builds frontend, IR and `mips_codegen`
libraries separately; `baka_heap_runtime` is a static library for the
interpreter's allocation helpers (generated code emits its own).

## Implemented source language

This is neither full C nor full C++. It has `void`, `bool`, the
`char`/`short`/`int`/`long` integer families, `float`, `double`, pointers,
fixed arrays, function pointers, typedefs, enums, structs/classes, unions, one
base class, members, constructors and destructors. Declarations include globals,
locals, `static`, `const`, prototypes, definitions, overloads, trailing default
arguments and variadic functions. Parameters require names. Functions can be
forward-declared and recursively called. `main` is an ordinary function for
semantic checking; natively it is `int main(int n)` (a test input) or
`int main(int argc, char** argv)` (see the MIPS64 section).

Expressions include literals, calls, indexing, dot/arrow member access, casts,
`sizeof`, address/dereference, unary/binary operators, assignments, compound
assignments, prefix/postfix mutation, ternary expressions and comma expressions.
Statements include blocks, conditionals, `switch`, `for`/`while`/`do`,
`goto`, `return`, `break` and `continue`. Baka loop suffix labels allow
`while (condition; L)` and `break L` or `continue L`. The handwritten parser
uses recursive descent and precedence climbing; its bundled C99 reference
grammar is background material, not a Baka specification.

Array bounds are positive constants; omitted bounds can be inferred from
initializers. There are no variable-length arrays. `printf` and `scanf` are
the automatically installed library names; other externals need source
declarations. There is no preprocessor or header processing. Lexer recognition
of a keyword alone does not imply source-language support.

Notable limits: no references, `long long` declarations, pointer-to-member
values, multiple inheritance, templates, lambdas, exceptions, namespaces,
access control, local aggregate definitions, unnamed parameters or
`main(void)`. Overload resolution takes a unique exact match, otherwise a
unique viable converted candidate; there is no per-argument promotion ranking.
Constructor argument conversion permits one user-defined conversion, not a
chain. Assignment and return do not inherit that call-argument policy.
Nonempty braced construction of classes with user constructors is rejected;
use `Type(args)`. Nontrivial union members are rejected.

## Semantic pipeline and layers

The analyzer owns persistent type, symbol and scope arenas. Its public phases
are:

```text
register declarations -> resolve bindings -> annotate and normalize -> check
```

`SemanticAnalyzer::analyze` runs these phases. Only a successful final check
publishes `CheckedProgram`, a view tying the exact AST to the analyzer and its
IDs. AST and analyzer must live through lowering. The compatibility lowering
entry point also validates AST identity, phase and diagnostics.

| Layer | Job and published facts |
| --- | --- |
| Parser AST | Source spans, syntax, direct expression edges and `visitChildren` traversal; semantic metadata and explicit conversion nodes are added later. |
| `Types` (`types.*`) | Arena IDs, immutable interning for pointers/arrays/qualifiers/signatures, nominal aggregate identity, completeness, fixed-target layout and stored field/base offsets. |
| `Symbols` (`symbols.*`) | Persistent lexical scopes, entity IDs, overload sets, declaration compatibility and separate positions for redeclarations. |
| `Declarations`, `ASTDeclarations`, `ASTAggregates` | Storage and member rules, AST declarators/parameters, inferred array bounds, aggregate completion and special-member traits. |
| `ExpressionChecker` | Intrinsic types/categories, overload selection, initializer compatibility, destination conversions and recoverable diagnostics. |
| Conversion writer | Actual AST rewriting for loads, decay, promotions, casts and retained constructor selections. |
| `ExecutionPlans` | Published initialization, call, lifetime and jump-cleanup decisions for lowering. |
| Final checks | Read-only validation of queued expression and control-flow facts. |

`registration.cc`, `resolution.cc`, `annotation.cc`, `checks.cc` express
the phase order. Expression rules are split by construct among `primary.cc`,
`operators.cc`, `calls.cc`, `members.cc`, `initializers.cc` and neighboring
files. `array_bounds.cc` handles early shape inference. `execution_plans.*`
and the `*_plans.cc` files group lifetime, initialization, calls and cleanup.

Registration collects signatures before bodies. Resolution binds identifier IDs
in saved lexical contexts. Declaration position matters independently of symbol
identity: a later global object or default argument is not retrospectively
visible to an earlier use. Forward function signatures are available. Each local
initializer binds after its own declarator is inserted and before the next one.
Goto labels are function-wide; loop/switch targets have separate identities.

Expression annotation distinguishes intrinsic, undecayed type and value category
from contextual value use. Thus `sizeof(array)` keeps the array while passing
it as a value inserts decay. Loads and casts are explicit AST nodes after
normalization. Bound methods retain receivers; function-pointer objects retain
their pointer values. Compound assignment saves its target address, so
`a[i++] += value` evaluates the index once. Numeric destination types and
selected converting constructors survive rewriting rather than being selected
again.

An `InitializationPlan` specifies typed value/string/zero/construction steps,
byte offsets, receiver context and compact omitted-array ranges. A `CallPlan`
records the selected callable plus supplied and default argument expressions.
Lifetime plans record requested special members and ordered base/member
subobjects. Cleanup plans list automatic objects crossed by `return`,
`break`, `continue` or `goto` in reverse construction order. Semantics rejects
unavailable or ambiguous requested constructors and jumps entering active
object lifetimes. Lowering supplies storage and runtime activation flags; it
does not repeat name lookup, overload resolution, brace matching or lifetime
legality. Unused implicit special members need not be emitted.

## Target and numeric contract

`src/base/target.hh` fixes an LP64 target. `src/base/numeric.hh/.cc` shares
width-aware conversions and evaluation between semantic constants and IR
folding. Pointers and `long` are 64-bit. Boolean values have one bit of meaning
and one byte of storage. Semantics enforces qualifiers; IR representation erases
them. Integer operations wrap at their declared width, including signed
arithmetic. Comparisons use converted operands, boolean casts test truth, and
floating operations round to each declared-width result. Division by zero and
out-of-range float-to-integer conversions are not fabricated into host-language
constant results; unavailable folds stay as operations. Constant objects are
converted to their declared destination type before value publication.

## IR pipeline, types and storage

The driver constructs `ir::Lowerer` from `CheckedProgram`. Lowering imports
types/declarations, emits ordinary functions and requested special members,
emits module startup, derives CFG edges, and verifies. Optional optimization
runs after verification and is verified again. Normal `--ir` prints raw TAC.
The resulting `ir::Module` owns all strings, type/layout descriptions,
globals, functions, operands, instructions and CFG metadata; no AST pointer or
semantic ID escapes it.

Module type, object, function and block IDs are identities; zero means absent
or invalid. Temporary IDs are function-local and mutable rather than SSA.
Functions own parameter object IDs (including explicit `this`), local objects,
typed temporaries and basic blocks. Blocks own instructions and derived CFG
edges. A reachable temporary use needs a definition along every incoming path.
Every block has an explicit terminator and no implicit fallthrough.

IR types are void, integer, floating, pointer, array, function and nominal
record. Printed forms include `i32`, `u64`, `f32`, `ptr<i32>`,
`[17 x i32]` and `fn(i32, ptr<i8>)->void`. Nested pointers are literal:
`ptr<ptr<i32>>` points to a pointer. Pointer-ness is in the type, not a
special opcode. The type table owns complete record field types, offsets,
size, alignment, union identity and optional base type/offset. Opaque records
can appear in unused declarations/signatures; stored values, definitions and
actual calls that need a complete record are rejected. Typedefs share
underlying representation; nominal records stay distinct.

The printer emits named aggregate definitions before functions:

```text
Node = record(i32, ptr<Node>) size 16 align 8
Number = union(i32, f64) size 8 align 8
Opaque = opaque
```

Fields are in storage order; an inherited base is the first subobject.
Methods, static members and typedefs use no per-object storage. Recursive
pointers are legal; recursive by-value containment is not. Duplicate,
anonymous or primitive-colliding names gain stable ID suffixes. Printed
names are display identities; internal `TypeId` addresses the owned table.
Complete records are first-class byte snapshots: `Load` captures, `Copy`
copies, and `Store` writes the whole value. Calls and returns keep record
types. Native register/memory ABI classification is deferred.

Globals have linkage/storage flags, literal bytes and sparse scalar/relocation
initializers. Source globals are zero-filled, then explicit global
initializers run in source order before entry, including constants.
Source-`const` globals with runtime initialization must be writable during
startup. Local statics use first-use guards and publish completion only after
initialization. Static destructor callbacks register after successful
construction and execute in reverse registration order after entry returns.

## Strict typed TAC

Each instruction is exactly `(opcode, result, left, right)`; a result exists
only for a produced value. Operand kinds are absent, temporary, object address,
function address, integer literal, floating literal and block target. Values
carry representation types. The arithmetic opcode supplies the operation;
there is no embedded `add:i32` suffix. Verifier rules and checked conversions
enforce each operator's applicable operand/result types.

| Opcode(s) | Meaning |
| --- | --- |
| `Copy`, `Load`, `Store` | Capture a typed value; read through an address; write `right` through `left` address. |
| `Cast`, `AddressOffset` | Convert a value; add a typed byte offset to a pointer. |
| `Add`, `Subtract`, `Multiply`, `Divide`, `Remainder` | Typed arithmetic. |
| `ShiftLeft`, `ShiftRight`, `BitAnd`, `BitOr`, `BitXor` | Integer shift and bit operations. |
| `Equal`, `NotEqual`, `Less`, `Greater`, `LessEqual`, `GreaterEqual` | Typed comparisons. |
| `Negate`, `Complement`, `LogicalNot` | Unary arithmetic, bitwise or boolean operation. |
| `Param` | Stage one captured argument in `left`; no result/right operand. |
| `Call` | `left` callable pointer, `right` integer argument count, optional result. |
| `RegisterDestructor`, `ZeroMemory` | Register a callable/receiver for exit cleanup; zero `right` bytes at `left`. |
| `Jump`, `JumpIf`, `Return` | Unconditional branch, conditional true branch, optional returned value. |

The callee and all arguments are evaluated and captured in source order.
Only then are contiguous `Param` instructions emitted immediately before
`Call` in the same block; `Call.right` counts them. Nested calls finish
before enclosing params are staged. There is no call argument side vector.
For `f(a, g(b))`, the essential shape is:

```text
t1 = load a
t2 = load b
param t2
t3 = call g, 1
param t1
param t3
t4 = call f, 2
```

Instance methods pass `this` as the first param. Constructor/destructor
calls use the same mechanism; variadic extras follow semantic promotions.
The verifier checks param contiguity, count, fixed parameter types and result
type. A conditional block ends in `JumpIf` for the true target immediately
followed by `Jump` for the false target. CFG construction includes both
successors; constant folding may replace the pair with one `Jump`.

## Function-style `new` and `delete`

These are compiler-provided calls, not C++ allocation/deletion expressions
and not IR opcodes:

```cpp
T* p = new(p, constructor_or_value_arguments...);
T (*a)[N] = new(a, constructor_or_value_arguments...);
delete(p);
delete(a);
```

`new` requires a direct pointer-variable identifier as its first argument.
This is a **type witness**: its pointer type selects a complete object or
fixed-array type, but its current value is not read. The result has the same
pointer type. `T* p = new(p, n)` allocates one `T` initialized with `n`;
it is not a dynamic array. A pointer to a fixed array allocates that whole
array, including nested constant dimensions. Construction walks forward and
re-evaluates the remaining arguments for each element. Semantics chooses
constructors/defaults/conversions and retains arguments in a `CallPlan`.
For primitive elements, one argument supplies a converted stored value;
with no argument the heap path emits no primitive initialization store.
Records take their selected/default construction path when required.

`delete(p)` requires exactly one complete-object pointer expression. It
evaluates that expression once. Null does nothing. For non-null values,
destructors precede deallocation; fixed arrays destroy elements in reverse
order. Heap objects are not registered for lexical or module-exit cleanup,
so callers must explicitly delete them.

`src/ir/heap.cc` emits ordinary `Param`/`Call` to
`__baka_allocate(u64 bytes, u64 alignment) -> ptr<void>`, casts its result
to the witness pointer type, and branches around construction when allocation
returns null. Constructor arguments are therefore not evaluated on failure.
Deletion emits destructors and then
`__baka_deallocate(ptr<void>) -> void`. Both helper names are reserved from
source declarations. `src/ir/heap_runtime.cc` implements them with C linkage,
`malloc`/`aligned_alloc` and `free`; invalid alignment, overflow and
allocation failure return null. Zero-byte requests use nonzero native storage.
The independent test interpreter models these helpers, including forced
failure. Dynamic-length arrays, placement allocation, custom allocators,
exceptions, and standard `new T`/`delete p` syntax are unsupported.

## Lifetimes, verification and optimization

One structured expression/statement emitter writes to the current block;
conditional or short-circuit expressions can move its insertion point.
There is no detached expression-region pass. Lowering creates storage and
explicit receiver parameters, supports construction into final destinations,
and tracks full-expression temporaries and ownership transfer to
parameters/results. Entry-initialized activation flags tell cleanup whether
construction completed on a path. Normal scope exit and planned jump edges
destroy active objects in reverse order. Small array lifetime operations may
unroll; larger arrays use IR loops. `ZeroMemory` represents omitted ranges
without one store per element.

The verifier checks type graphs/layouts, operand kinds and IDs before
opcode-specific accesses, instruction shapes, call groups, terminators,
CFG edges and definite assignment of mutable temporaries. Optimization
transforms one basic block at a time; only analyses (CFG, reachability,
liveness via `ir::LiveOut`) are global. Passes: constant folding, copy
propagation, integer identities and strength reduction, value numbering,
redundant-load elimination, store-to-load forwarding, dead-store elimination
on identical named addresses, constant branch folding with unreachable-block
removal, and next-use dead-code elimination seeded with global live-out sets.
It preserves calls, bulk zeroing, destructor registration, control effects and
potentially faulting loads/division. There is no CFG-wide propagation, jump
threading or SSA optimizer (`IR_OPTIMIZATION_PLAN.md`).

`src/ir/ir.hh` defines the model/opcodes. `lowering.*` imports and builds.
`expressions.cc`, `primary.cc`, `addresses.cc`, `operators.cc`, `calls.cc`,
`heap.cc`, `conditional.cc` emit expressions. `statements.cc`,
`functions.cc`, `iteration.cc`, `selection.cc`, `jumps.cc` emit control
flow. `initializers.cc`, `runtime_initialization.cc`, `lifetimes.cc`
emit storage/lifetime behavior. `cfg.cc`, `verifier.cc`,
`optimization.cc`, `constant_folding.cc`, `dead_code.cc`, `printer.cc`
complete the pipeline. The layout follows main's construct-oriented style:
typed interfaces in headers and focused implementations by construct.

## MIPS64 code generation

`src/codegen` emits one GNU assembler file for MIPS64r2, n64, little-endian,
non-PIC, linked statically against glibc and run under `qemu-mips64el` from the
`baka-mips64el` Docker image (`tools/mips/Dockerfile`, `tools/mips/run`). n64
is LP64, so every IR layout and width is native and `target.hh` is unchanged.
`src/codegen/README.md` records each decision and its justification.

* Entry: `__baka_start(n, argv)` runs `.module_init`, then the user's `main`;
  a weak C `main` passes `atoi(argv[1])`, or the real `argc`/`argv` to a
  two-parameter `main`. `register_dtor` becomes `__cxa_atexit`. Every call goes
  through `$t9`, which glibc's PIC prologues need.
* Calls follow n64 for scalars (`$a0-$a7`, `$f12-$f19`, stack beyond eight,
  varargs in GPRs, 32-bit values sign-extended). Records travel as pointers to
  caller snapshots and record results use a hidden destination in `$a0`.
* Registers hold integers canonically extended by their own signedness. The
  allocator is the course's block-local next-use/`getReg` algorithm with
  register/address descriptors; non-escaping scalar locals act as variables;
  global liveness decides which values are written back at block ends; values
  live across calls prefer callee-saved registers.
* `mips.hh` is the machine IR (one `MInst` per line, `.set noreorder`);
  `select.cc` lowers and allocates; `peephole.cc` performs block-local redundant
  load/store removal, dead code, copy propagation/retargeting, address and
  immediate folding, strength reduction, jump cleanup and delay-slot filling;
  `printer.cc`, `data.cc` and `runtime.cc` print code, data and entry helpers.

## Tests and known limits

`custom_tests/lex` and `custom_tests/parse` are golden frontend suites.
`custom_tests/semantic` groups representative acceptance and rejection
programs. `custom_tests/ir` has 25 numbered fixtures checking printed TAC
and independent execution of raw and optimized modules; `custom_tests/ir_opt`
pins the optimized TAC of each local pass, including negative cases. The interpreter
poisons unwritten automatic storage and zeroes static storage, so missing
stores do not accidentally pass. `custom_tests/codegen/run.py` compiles every
IR fixture and 28 codegen fixtures (libc I/O, widths, floating conversions,
records, many arguments, big frames, register pressure), optimized and not,
and runs them under QEMU against natively computed results;
`peephole_tests.cc` unit-tests each peephole rule. CTest registers `semantic`,
`ir`, `ir_opt`, `peephole` and (when Docker exists) `codegen`. `ir_execution_tests` also exercises verifier rejection and native
heap-helper alignment/overflow behavior.

The heap fixture `22_heap_calls.cpp` covers scalar/fixed-array construction
and reverse destruction, null deletion, a 17-element loop, and skipped
argument side effects after forced allocation failure.
`invalid_heap_calls.cpp` groups semantic rejection cases. At `f6227df`,
both CTest stage suites and the lexer/parser custom hook passed, and that
commit was pushed to `origin/saturn/slop`. On `saturn/slop2` all five CTest
suites and the lexer/parser hook pass. Run the commands above after further
code changes. The broad `tests/chapter_*` corpus uses `main(void)` and is not
part of the native suite.

Remaining deliberate gaps include n64-compatible record passing to external C
functions, global (cross-block) register allocation, exception unwinding, concurrent/reentrant static
initialization, active-member tracking for nontrivial unions,
pointer-to-member values and dynamic heap arrays. There is no all-paths-return
or uninitialized-source-read diagnostic. A fallthrough function currently
lowers to a zero result; that behavior is not an all-paths-return proof.
