# IR optimization options

This is a plan for the implemented typed TAC in `src/ir`. “Low proof” means a
pass can use local facts and the existing
type/numeric rules. It does **not** mean it needs no correctness tests.

## Starting point

`Optimize` now repeats block-local constant folding/copy propagation, CFG-wide
copy/constant propagation, integer identities, local value numbering, branch
simplification, jump threading, and dead-code elimination until stable. The last
pass removes unreachable
blocks and unused non-effectful temporary definitions using CFG liveness. It keeps
loads, division/remainder, stores, zeroing, calls, destructor registration and
control effects. The optimizer then rebuilds CFG edges. The normal `--ir` CLI
prints unoptimized TAC; the IR test runner executes before and after optimization.

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
| Algebraic and bitwise identities | Implemented: `x+0`, `x-0`, `x*1`, `x&all_ones`, `x|0`, `x^0` and shifts by zero for integers. Further rules include `!!x` only for canonical boolean values. | Partial | **Low to medium.** Do not delete an earlier computation that may fault merely because `x*0` or `x-x` has a known result. Avoid floating `x+0`, `x*0` and reassociation because of signed zero, NaNs and rounding. |
| Constant branch folding | Fold literal conditions and identical true/false targets after propagating known temporary conditions. | Implemented | **Low.** Rewrite both tail instructions as one `Jump`, then rebuild CFG and remove newly unreachable blocks. |
| Unreachable block elimination | Already implemented using CFG reachability. Repeat after branch changes and remove unreachable effectful instructions with their blocks. | Yes | **Low.** Entry is `blocks.front()`; preserve valid jump destinations and CFG metadata. |
| Jump threading and empty-block removal | Redirect a jump through a block containing only `Jump`, or collapse a branch whose arms have the same destination. | Implemented | **Low to medium.** Update every target, retain a valid entry block and rebuild CFG. Do not move instructions across cleanup blocks. |
| Dead temporary elimination | Already implemented with interblock liveness. Run to a fixed point after other passes so chains and cross-block opportunities disappear. | Yes | **Low.** Preserve all instructions with observable effects or possible faults. |
| Local common subexpression elimination / value numbering | Within one block, reuse a prior integer arithmetic/bitwise result until an input or that result is redefined. | Implemented, restricted | **Medium.** Excludes loads, calls, division/remainder and floating operations. Does not move evaluation. |
| Local constant/copy propagation through the CFG | Compute per-block IN/OUT facts to a fixed point; meet facts by intersection at joins; rewrite uses. | Implemented | **Medium.** Uses instruction order and kills on every temporary definition. Unknown/conflicting incoming values become unknown. Constants in object memory require separate alias analysis. |
| Sparse conditional constant propagation (SCCP) | Track executable CFG edges plus a constant/unknown lattice for temporaries, folding branches and unreachable paths together. | Yes, with adaptation | **Medium to high.** Standard SSA SCCP cannot be applied directly; use reaching definitions or convert temporaries to SSA first. A dense CFG dataflow pass is simpler initially. |
| Global value numbering / global CSE | Carry equivalence classes over CFG joins, or convert mutable temporaries to SSA first. | Later | **High.** Must establish dominance and distinguish definitions on different paths. Memory reads need alias/effect tracking. |
| Dead store elimination | For a known private, non-escaping object, remove an overwritten store when no intervening read, call, alias or destructor can observe it. Start with exact object plus exact byte range. | Restricted | **Medium to high.** `AddressOffset`, overlapping records/unions, `ZeroMemory`, pointer casts, calls and record snapshots complicate byte-level observability. Do not delete user/global/runtime-initialization writes by default. |
| Store-to-load forwarding | Forward a stored value into a later `Load` from the same proven address and type, with no intervening possibly aliasing write/call. | Restricted | **Medium.** Start within one block on direct `Object` addresses with exact type and no escape; widen only with byte-range alias facts. Whole-record values are snapshots. |
| Redundant load elimination | Reuse a previous `Load` if the address and contents cannot change before the next load. | Restricted | **Medium to high.** Calls, indirect stores, zeroing, destructor actions and globals require conservative invalidation. Do not assume a source `const` object is immutable during runtime initialization. |
| Alias/escape analysis | Identify local objects whose addresses never escape via calls, returns, stores, globals or unknown pointer operations; derive exact/overlapping byte ranges for fixed offsets. | Yes, as an analysis | **High.** This is the enabling proof for memory optimizations. Union fields, pointer casts, `AddressOffset` and array indices require overlap conservatism. |
| Scalar replacement of aggregates | Replace eligible local record/array storage with scalar temporaries and field-wise operations. | Later | **High.** Must model overlapping union fields, byte writes/zeroing, whole-record snapshots, address escape, construction and destruction. |
| Strength reduction | Replace expensive integer operations with cheaper equivalent ones, e.g. multiply by power of two to shift, after matching width and signed behavior. | Yes, restricted | **Medium.** Check the exact `numeric` contract, especially signed division/remainder, overflow and shifts. Real benefit requires a backend cost model; there is no native codegen yet. |
| Reassociation and expression balancing | Reorder arithmetic trees to expose constants or reduce depth. | Restricted | **Medium to high.** Integer wrapping permits some same-width rules; division, conversions and floating-point operations generally prohibit them. Benefit depends on backend scheduling. |
| Loop-invariant code motion | Find natural loops/dominators; hoist pure expressions whose inputs are unchanged and whose execution is safe on paths where the expression was previously skipped. | Restricted | **High.** Must avoid hoisting faults, calls, loads, cleanup/constructor effects or conditional work. A preheader and valid CFG rewrites are needed. |
| Induction-variable simplification | Recognize loop counters in mutable temporaries, replace derived arithmetic, and simplify exit checks. | Later | **High.** Needs loop analysis, width/overflow proof, and careful handling of `goto`, named break/continue and cleanup edges. |
| Loop unrolling / peeling | Duplicate loop bodies with remapped block and temporary IDs and preserved exit/cleanup behavior. | Later | **High.** Needs trip-count or profitability information and strict code-size limits. Existing lowering already unrolls small array lifetime loops. |
| Loop vectorization | Group independent scalar iterations into vector operations. | Not directly | **Very high.** IR has no vector types/opcodes, dependence analysis, alignment facts or vector backend. |
| Inlining | Clone a defined callee's blocks/objects/temporaries into a caller and bind `Param` values to parameters. | Later | **High.** Requires safe ID remapping, returns/cleanup reconstruction, recursion and growth policy; indirect calls cannot generally be resolved. |
| Interprocedural constant propagation / specialization | Clone functions for known arguments or summarize return values/call effects. | Later | **High.** Needs call graph, address-taken/indirect-call handling and code-growth policy. Module startup and internal destructor helpers are ordinary functions but still have effects. |
| Dead function/global elimination | Remove unreferenced internal definitions after tracing entry, initializer, address-taken functions and registered destructors. | Restricted | **Medium to high.** External linkage and callbacks are roots; startup and registration must remain intact. No linker/codegen benefit yet. |
| Tail-call optimization | Turn a return of a call result into a tail call. | Not directly | **High.** No tail-call opcode or ABI/codegen; pending destruction, cleanup and call argument staging often prevent it. |
| Register allocation / instruction scheduling | Use liveness and dependencies when lowering TAC to machine code. | Later | **High / backend dependent.** These are backend passes; IR has no physical registers or machine latency/cost model. |

## Recommended sequence

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
