# 4. IR optimization

With optimization on, the IR is simplified before instruction selection.

## The scope rule

*Analyses* may look at the whole function (CFG, reachability, liveness), but every
*transformation* stays inside one basic block. That keeps each rewrite easy to justify:
nothing has to reason about values flowing along edges. Using global liveness still
avoids the pessimistic textbook assumption that every value is live at the end of a
block.

An earlier version did CFG-wide propagation and jump threading; it was removed in favour
of this rule. The IR test runner now checks that no value crosses a block edge after
optimization.

## The passes

| Pass | What it does | Safety condition |
|---|---|---|
| Constant folding | Evaluates operations on constants at compile time | Uses the same arithmetic rules as run time; never folds things like division by zero |
| Copy propagation | Replaces uses of a copy with its source | A fact is forgotten when the temporary or its source is redefined |
| Algebraic identities | `x+0`, `x*1`, `x-x`, `x^x`, `x*0`, … | Integers only; floats are left alone because of NaN and -0.0 |
| Strength reduction | `x * 2^k` to a shift; unsigned `/` and `%` by powers of two | Signed division is untouched because it rounds toward zero |
| Value numbering (CSE) | Reuses an identical earlier computation | Pure operations only; commutative operands match in either order |
| Redundant loads / store-to-load forwarding | Reuses a value instead of reloading it | Any call, pointer store, zeroing or destructor registration forgets everything |
| Dead-store elimination | Drops a store that is overwritten before being read | Same named object, no possible read in between |
| Dead-code elimination | Drops instructions whose results are never used | Seeded with the block's live-out set |
| Constant branch folding | `if_goto` on a constant becomes `goto` | Unreachable blocks are deleted afterwards |

## Verification

The verifier runs again on the optimized IR, so an optimizer bug that produces malformed
IR is caught before code generation.

## Effect

Combined with the peephole pass, optimization cuts static instruction count by about a
quarter across the execution fixtures, and by over a third on algorithm-heavy ones.
