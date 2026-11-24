# 6. Register allocation

The allocator follows the block-local algorithm from the course lectures (next-use
information and `getReg`), applied to every basic block, with global liveness added so
that dead values are not saved.

## Allocatable registers

| Class | Caller-saved (lost across calls) | Callee-saved (preserved across calls) |
|---|---|---|
| Integer | `$t0`–`$t3`, `$t8` | `$s0`–`$s7` |
| Floating | `$f2`–`$f11`, `$f20`–`$f23` | `$f24`–`$f31` |

Not allocated: `$v0`/`$v1`/`$f0`/`$f1` (scratch and return values), `$t9` (addresses and
call targets), `$a*`/`$f12`–`$f19` (used only while setting up a call), `$at` (reserved
for the assembler), `$sp`, `$ra`, `$gp`.

## "Names"

The allocator tracks two kinds of names:

- **Temporaries.**
- **Promoted variables:** scalar locals whose address is only ever used for a direct load
  or store of the whole value. They cannot be aliased, so they can live in registers like
  temporaries. Everything else (globals, arrays, structs, locals whose address escapes)
  is accessed in memory every time.

## Step 1: liveness and next use

- **Globally:** a backward dataflow analysis over the CFG gives the set of names live on
  exit from each block.
- **Per block:** a backward scan records, for each instruction, the next instruction that
  uses each live name. It also records where the calls are, so the allocator knows when a
  value must survive one.

## Step 2: descriptors

- A **register descriptor** lists the names whose current value each register holds.
- An **address descriptor** lists where each name's current value can be found: which
  registers, and whether its memory slot is up to date.

## Step 3: `getReg`

For each **operand**:
1. If a register already holds it, use that.
2. Else take an empty register.
3. Else evict the cheapest register: one whose values are all dead, saved in memory or
   held elsewhere. Break ties by evicting the value used furthest in the future. Store
   anything that is still needed and has no other copy (a spill).

For the **result**:
1. A register holding only the old value of the result, or
2. an operand's register whose value dies at this instruction, or
3. the operand rule above.

**Values live across a call prefer callee-saved registers**, so a call only forces
caller-saved registers to be saved.

## Step 4: calls

Before a call, every value in a caller-saved register that is still needed afterwards,
and has no surviving copy, is stored to memory. After the call, those registers are
forgotten.

## Step 5: block ends

Before each block's terminator, every value that is live into a successor and exists only
in a register is stored to its memory slot. The next block starts with empty descriptors
and loads values on demand. Dead values are never written back.

## Consequence

Values never stay in registers across block boundaries. In a loop, the loop variable is
stored at the end of each iteration and reloaded at the start of the next. That is the
main performance cost of the design, traded for a simple, predictable allocator. For
comparison, production compilers (LLVM's greedy allocator, GCC's IRA) allocate over whole
functions and split live ranges around loops.
