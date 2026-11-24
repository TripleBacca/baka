# IR optimization options

This is a plan for the implemented typed TAC in `src/ir`. “Low proof” means a
pass can use local facts and the existing
type/numeric rules. It does **not** mean it needs no correctness tests.

## Starting point

`Optimize` first collects natural loops using dominators and fully unrolls eligible
straight-line loops. The unroller matches a local `int` counter that starts at zero
and increments by one, with 2–4 iterations and at most 16 body instructions.
`Optimize` then repeats block-local constant folding and copy propagation, integer
identities and strength reduction, local value numbering (commutative, casts,
comparisons, address offsets), redundant-load elimination, store-to-load forwarding,
dead-store elimination and next-use dead-code elimination. Between them it folds
constant branches (looking only at one block's `if_goto`/`goto` tail) and removes
blocks that became unreachable, a cleanup that never moves or merges instructions.
After this simplification, eligible direct tail recursion becomes an IR loop.
Small direct callees are then inlined, followed by another simplification when
inlining changed the IR.
CFG-wide propagation and jump threading were removed. Dead-code elimination starts each block's
backward scan from its global live-out set (`ir::LiveOut`). The normal `--ir` CLI prints
unoptimized TAC; `--ir --optimize` prints the result. `custom_tests/ir` executes
before and after optimization; `custom_tests/ir_opt` pins the optimized shapes.

The IR has explicit blocks and jumps, typed scalar and whole-record values,
byte-offset addresses, explicit loads/stores and bulk zeroing. It has **mutable**
temporaries: conditional results and lifetime flags can be assigned on multiple
paths. There are no phi nodes, memory SSA, alias metadata, purity annotations,
range facts, or profile data. `Param` instructions must remain contiguous just
before their `Call`; `JumpIf` must remain paired with the final `Jump`. Every block
must end in a terminator and CFG lists must match it.

| Optimization | How it fits this IR | Feasible now? | Proof / work |
| --- | --- | --- | --- |
| Repeated constant folding | Reuse `FoldConstant` and `numeric::evaluate`; rerun after substitutions and CFG simplification until stable. | Implemented | **Low.** Only fold when numeric evaluation returns a value. Preserve widths, signedness and float rounding. |
| Repeated copy propagation | Reuse local temporary copy maps, then rerun folding and dead-code elimination. | Implemented | **Low.** Kill a mapping on every definition of its destination or temporary source; never substitute mutable object memory as a value. |
| Algebraic and bitwise identities | Implemented: `x+0`, `x-0`, `x*1`, `x/1`, `x&all_ones`, `x|0`, `x^0`, shifts by zero, plus `x*0`, `x&0`, `x%1`, `x-x`, `x^x` to 0 and `x|all_ones` to all ones, integers only. | Implemented | **Low.** Constants compared at the result width. Dropping a temporary operand cannot hide a fault. Never floating (signed zero, NaN). |
| Constant branch folding | Fold a literal condition, or identical true/false targets, in a block's own `if_goto`/`goto` tail. | Implemented | **Low.** Rewrites both tail instructions as one `Jump`; unreachable blocks are then dropped. |
| Unreachable block elimination | Separate cleanup after branch folding, using CFG reachability from the entry. | Implemented | **Low.** Only deletes whole blocks; no instruction moves across blocks. |
| Jump threading and empty-block removal | Redirect a jump through a block containing only `Jump`. | Removed | Requires a separate CFG rewrite. |
| Dead temporary elimination | Backward next-use scan per block, seeded with the block's live-out temporaries from global liveness. | Implemented | **Low.** Liveness is an analysis; deletion stays inside the block. Every definition of a mutable temporary kills it. Loads, division/remainder, stores, calls, params, zeroing, destructor registration and control flow stay. |
| Local common subexpression elimination / value numbering | Reuse a prior pure result in the same block: integer/floating arithmetic, comparisons, casts, address offsets. Commutative integer operands match in either order. | Implemented | **Medium.** Entries die when an operand or result temporary is redefined. Floating only for the exact same operands. An earlier identical division is reusable because it already executed. |
| Local constant/copy propagation | Per-block constant and copy maps, rerun with folding. | Implemented (CFG-wide version removed) | **Low.** Kill a mapping on every definition of its destination or temporary source. |
| Sparse conditional constant propagation (SCCP) | Track executable CFG edges plus a constant/unknown lattice for temporaries, folding branches and unreachable paths together. | Yes, with adaptation | **Medium to high.** Standard SSA SCCP cannot be applied directly; use reaching definitions or convert temporaries to SSA first. A dense CFG dataflow pass is simpler initially. |
| Global value numbering / global CSE | Carry equivalence classes over CFG joins, or convert mutable temporaries to SSA first. | Later | **High.** Must establish dominance and distinguish definitions on different paths. Memory reads need alias/effect tracking. |
| Dead store elimination | Remove `store A, v1` when the identical named-object address is stored again later in the block with no possible read between. | Implemented, restricted | **Medium.** Only `Object` addresses. A load of another object does not block it; a load through a temporary, call, param, zeroing or destructor registration does. |
| Store-to-load forwarding | Within a block, `load A` after `store A, v` of the same type becomes `copy v`. | Implemented, restricted | **Medium.** A store to an object only forgets facts for that object and temporary-addressed facts; a store through a temporary, call, zeroing or destructor registration forgets all. Redefining the address or value temporary forgets the fact. |
| Redundant load elimination | A second `load` of the identical address and type becomes `copy` of the first result. | Implemented, restricted | **Medium.** Same invalidation as forwarding. Record loads are snapshots, so `copy` is exact. |
| Alias/escape analysis | Identify local objects whose addresses never escape via calls, returns, stores, globals or unknown pointer operations; derive exact/overlapping byte ranges for fixed offsets. | Yes, as an analysis | **High.** This is the enabling proof for memory optimizations. Union fields, pointer casts, `AddressOffset` and array indices require overlap conservatism. |
| Scalar replacement of aggregates | Replace eligible local record/array storage with scalar temporaries and field-wise operations. | Later | **High.** Must model overlapping union fields, byte writes/zeroing, whole-record snapshots, address escape, construction and destruction. |
| Strength reduction | `x*2^k` to `x<<k`; unsigned `x/2^k` to `x>>k`; unsigned `x%2^k` to `x&(2^k-1)`. | Implemented | **Low to medium.** Integers only; multiplication is exact under wrapping for both signednesses. Signed division is untouched (rounding). |
| Reassociation and expression balancing | Reorder arithmetic trees to expose constants or reduce depth. | Restricted | **Medium to high.** Integer wrapping permits some same-width rules; division, conversions and floating-point operations generally prohibit them. Benefit depends on backend scheduling. |
| Loop-invariant code motion | Find natural loops/dominators; hoist pure expressions whose inputs are unchanged and whose execution is safe on paths where the expression was previously skipped. | Restricted | **High.** Must avoid hoisting faults, calls, loads, cleanup/constructor effects or conditional work. A preheader and valid CFG rewrites are needed. |
| Induction-variable simplification | Recognize loop counters in mutable temporaries, replace derived arithmetic, and simplify exit checks. | Later | **High.** Needs loop analysis, width/overflow proof, and careful handling of `goto`, named break/continue and cleanup edges. |
| Loop unrolling / peeling | Fully unroll eligible natural loops and fold constant counter reads. | Implemented for small fixed counts | Partial unrolling and peeling need remainder and exit handling. |
| Loop vectorization | Group independent scalar iterations into vector operations. | Not directly | **Very high.** IR has no vector types/opcodes, dependence analysis, alignment facts or vector backend. |
| Inlining | Clone a small defined callee's blocks, objects, and temporaries into a caller and bind `Param` values to parameters. | [Implemented for eligible direct calls](wiki/04-ir-optimization.md#function-inlining) | Uses a register-pressure limit and bounded expansion rounds. Indirect calls keep their original form. |
| Interprocedural constant propagation / specialization | Clone functions for known arguments or summarize return values/call effects. | Later | **High.** Needs call graph, address-taken/indirect-call handling and code-growth policy. Module startup and internal destructor helpers are ordinary functions but still have effects. |
| Dead function/global elimination | Remove unreferenced internal definitions after tracing entry, initializer, address-taken functions and registered destructors. | Restricted | **Medium to high.** External linkage and callbacks are roots; startup and registration must remain intact. No linker/codegen benefit yet. |
| Tail-call optimization | Turn direct self-recursion into an IR loop, then handle calls between functions in MIPS selection. | [Direct self-recursion implemented](TAIL_CALL_OPTIMIZATION_PLAN.md) | Sibling calls will need frame teardown, argument staging, and target ABI checks. |
| Register allocation / instruction scheduling | Use liveness and dependencies when lowering TAC to machine code. | Later | **High / backend dependent.** These are backend passes; IR has no physical registers or machine latency/cost model. |

## Recommended sequence

The current pipeline combines full unrolling for simple counted loops with
block-local simplification. CFG-wide propagation and other loop passes remain future work.

1. Make the existing local pipeline iterate to a stable result, with an iteration
   guard. Rebuild CFG after branch changes, then rerun reachability and liveness.
2. Add conservative integer identities, branch simplification, jump threading and
   local value numbering. Keep each rewrite typed and verifier-valid.
3. Add CFG-wide constant/copy dataflow with an explicit `unknown` meet state for
   joins and loops. This produces many of SCCP's benefits without SSA conversion.
4. Add escape and byte-range alias facts before forwarding loads or eliminating
   stores. Begin with non-escaping local objects and exact-address accesses.
5. Reassess SSA conversion, loop passes and inlining once there is a code generator
   and a cost model. This avoids optimizing for instruction counts that may not
   predict generated-code quality.

After every CFG rewrite, rebuild edges and run `Verify`. For each new pass, compare
the independent interpreter's optimized and unoptimized results on arithmetic
widths, NaNs/signed zero, branches/loops, pointer aliases, records/unions, calls,
static initialization and destructor ordering. Add a focused IR-shape assertion
for the optimization itself: equal execution alone cannot show that a pass fired.

The relevant implementation contracts are in `src/ir/ir.hh`, `src/ir/verifier.cc`,
`src/ir/optimization.cc`, `src/ir/dead_code.cc`, `src/base/numeric.cc` and
`ir_context.md`; the interpreter and fixtures live in `custom_tests/ir`.
