# 10. Peephole optimization

After a function's machine code is complete, a peephole pass cleans it up. It runs only
when optimization is on.

## How it works

- **Exact register liveness.** Every machine instruction declares the registers it reads
  and writes, so the pass knows when a register's value is dead.
- **Liveness hints at block labels.** Because the allocator flushes values to memory at
  every IR block boundary, only the stack pointer, return address and callee-saved
  registers can be live at a block label. Selection records this on each label, which lets
  the dead-code rules fire at block exits too.
- **Iterates to a fixed point.** The rules run repeatedly, up to 32 rounds, until nothing
  changes. Delay-slot filling then runs once at the end.
- **Local scope.** Rewrites never look past a label or past a branch and its delay slot.

## The rules

| Rule | Example |
|---|---|
| Redundant reloads | `sd $t0, 16($sp)` … `ld $t1, 16($sp)` becomes `move $t1, $t0` |
| Dead stores | a stack-slot store overwritten before any possible read is deleted |
| Dead code | an instruction whose results are never read is deleted |
| No-op removal | `move r, r`, `daddiu r, r, 0` |
| Copy propagation | after `move a, b`, later reads of `a` read `b` |
| Retargeting | `lw $t0, …; move $v0, $t0` becomes `lw $v0, …` |
| Address folding | `daddiu t, b, 8; ld r, 4(t)` becomes `ld r, 12(b)` |
| Immediate forms | `dli t, 5; daddu x, s, t` becomes `daddiu x, s, 5` |
| Strength reduction | multiply by a power of two becomes a shift |
| Control flow | drop a branch to the next label; invert a branch over a jump; delete unreachable code after a jump; delete unused local labels |
| **Delay-slot filling** | the nearest independent preceding instruction replaces the `nop` after a branch, call or return |

## Delay-slot filling, briefly

An instruction is moved into a delay slot only if:
- it is a single real instruction (not a multi-instruction macro like `dli` or `dla`);
- the branch does not read a register it writes;
- it is not a multiply or divide, and is not itself in a delay slot;
- it does not cross a label.

## Testing

A dedicated unit test checks each rule, and the cases where it must not fire, on
hand-written machine code.
