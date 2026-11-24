# 5. Instruction selection

## Machine IR

The backend does not print text directly. It builds a small machine IR in which **one
record equals one printed assembly line**: an opcode, its operands (register, immediate,
memory `offset(base)`, label or symbol), and optional comments.

Every opcode has a table entry describing which operands it reads and writes, whether it
loads or stores, and whether it branches, calls, returns or has a delay slot. Calls and
returns also list the registers they implicitly read and clobber. That table is what lets
the peephole pass compute exact register liveness.

Two pseudo-registers model hidden machine state (the HI/LO multiply-divide result and the
floating-point condition flag), so they are tracked like ordinary registers.

## Selection and allocation are one pass

Selection walks each IR block in order. For each instruction it asks the register
allocator ([page 6](06-register-allocation.md)) for registers holding the operands and a
register for the result, then emits the matching MIPS instructions. There is no separate
"virtual register" stage.

## The canonical integer form

Each integer register always holds its value extended to 64 bits according to its own
type: signed types sign-extended, unsigned types zero-extended, booleans 0 or 1.

- One 64-bit compare (`slt` or `sltu`) is then correct for every width.
- `int` arithmetic uses MIPS's 32-bit instructions, whose results are defined to be
  sign-extended, so it costs nothing extra.
- Other widths compute in 64 bits and re-normalize (sign-extend a byte or halfword, mask,
  or zero-extend) only when the operation can produce out-of-range bits.

## How IR maps to MIPS, broadly

| IR | MIPS |
|---|---|
| Integer add/sub/mul/div/rem | 32-bit forms for `int`, 64-bit forms otherwise; immediate forms when a constant fits |
| Shifts | Immediate or variable shifts, choosing arithmetic or logical by signedness |
| Comparisons producing a value | `slt`/`sltu`, plus `xor`/`sltiu` tricks for `==` and `!=` |
| `if_goto` on a compare in the same block | Fused into one compare-and-branch (`beq`, `bne`, `bltz`, …) |
| Float arithmetic and compares | Single or double FPU instructions; compares set the condition flag |
| Loads and stores | Width- and signedness-correct load/store instructions |
| Integer ↔ float casts | FPU convert instructions, with extra steps for unsigned 64-bit values above 2^63 |
| Record copies | Unrolled loads/stores up to 256 bytes, a loop above that |
| `zero_memory` | Unrolled stores of zero, or a loop for large or variable sizes |
| `x = y` (plain copy) | No code: `x` shares `y`'s register |

## Constants

- Integer constants that fit in 16 bits become immediate operands; larger ones are
  materialized with the assembler's `dli` macro. The constant zero uses `$zero`.
- Floating constants cannot be immediates. Each distinct value is stored once in a
  read-only **literal pool** and loaded by label.

## Scratch registers

A few registers are never allocated and are only valid within one instruction's
expansion: `$v0`/`$v1` and `$f0`/`$f1` for intermediate values, and `$t9` for addresses
and call targets.
