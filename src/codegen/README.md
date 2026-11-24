# MIPS64 code generation

`--codegen` lowers the verified typed TAC (`src/ir`) to one GNU assembler file
for **MIPS64r2, n64 ABI, little-endian**, linked statically against glibc and
run under `qemu-mips64el`. This document records every backend design decision
and the reason for it. The IR contract it consumes is in `ir_context.md`; the
source-language rules are in `src/semantic/README.md`.

```sh
./build/baka --codegen file.cpp > file.s            # optimized (default for codegen)
./build/baka --codegen --no-optimize file.cpp       # no IR passes, no peephole
./build/baka --ir --optimize file.cpp               # print the optimized TAC
tools/mips/run file.s [n]                           # assemble, link and run in Docker
```

## Decisions

### Target: MIPS64 n64, little-endian (chosen over MIPS32 o32)

The frontend fixes an LP64 target (`src/base/target.hh`): `long` and pointers
are 64-bit, records are laid out with 8-byte pointers, and the IR carries
`i64`/`u64`/`ptr` values everywhere (array indexing uses `i64` byte offsets).
n64 is LP64 too, so every IR type maps onto one machine register and every
layout the semantic layer computed is the native layout. **No previous stage
changes.** o32 would have required either changing `target.hh` (and every IR
fixture) or emulating 64-bit integers and pointers with register pairs.
Little-endian matches the byte order assumed by the IR test interpreter.

### Environment: Linux user mode under QEMU, glibc, static, non-PIC

The language auto-declares `printf`, `scanf`, `atoi`, `fopen`, `fread`, `fwrite`,
`read` and `write`. With a real libc these work as-is; SPIM/MARS would have
needed a hand-written libc in assembly. The toolchain is a Debian image
(`tools/mips/Dockerfile`: `gcc-mips64el-linux-gnuabi64` and `qemu-user`), so
nothing is installed on the host.

The generated code is **non-PIC** (`.option pic0`) and linked `-static`:
addresses are formed with `dla` (lui/daddiu/dsll sequences) and there is no
GOT, `$gp` setup or `.cpload` in our functions. glibc itself is PIC inside the
static archive, and its function prologues derive `$gp` from `$t9`. Therefore
**every call goes through `$t9`** (`dla $t9, f; jalr $t9`), which is also the
n64 PIC calling convention, and our own functions tolerate it.

### Program entry

Semantic analysis accepts exactly two entry shapes: `main()` and
`main(int argc, char** argv)`, returning an integer type or `void`. A program
that needs an input reads it from the command line, e.g.
`int n = atoi(argv[1]);` (`atoi` is a built-in). The backend emits:

* `__baka_start(argc, argv)`: calls the module initializer (`.module_init`, which
  runs global initializers in source order), then the user's `main` (with
  `argc`/`argv` when it takes them), and returns its result sign-extended to 32 bits
  (0 for a `void` main).
* a **weak** C `main(argc, argv)`: a tail jump to `__baka_start` with the real
  command line. The process exit status is the low 8 bits of the result.
  Weak linkage lets the test harness (`custom_tests/codegen/harness.c`) supply
  its own `main`, which calls `__baka_start(argc, argv)` itself and prints the full
  32-bit result and a chosen global after static destructors have run.
* The IR test interpreter runs `main` the same way as `prog <n>`: `argc = 2` and
  `argv = {"prog", "<n>", null}`, and it implements `atoi`.
* `register_dtor f, p` lowers to `__cxa_atexit(f, p, &__dso_handle)`. glibc runs
  these after `main` returns, in reverse registration order, which is exactly
  the IR's static-destructor contract.

User-visible symbols keep the IR's unique `name.id` spelling (`main.31`,
`A.get.23`); a leading dot becomes `__baka.` (`__baka.module_init.35`).
Declared-only functions keep their plain names (`printf`). This avoids every
collision with libc and with the C `main`.

### Calling convention (n64 with two documented extensions)

* Argument slot *i* (0-7) uses `$a<i>` for integer/pointer values and `$f(12+i)`
  for fixed `f32`/`f64` parameters; slots 8+ go to the caller's outgoing area at
  `8*(i-8)($sp)`. Variadic extras always use GPRs/stack (doubles are moved with
  `dmfc1`), as n64 requires for `printf`.
* All 32-bit integers cross call boundaries sign-extended (n64 rule, including
  `u32`); narrower types are extended by their own signedness. Inside a function
  registers hold the *canonical* form described below, and the receiver
  re-normalizes.
* Results return in `$v0` or `$f0`.
* **Records by value** (an IR feature with no native ABI yet): an argument is
  passed as a pointer to a caller-owned snapshot and the callee copies it into
  its parameter object; a record result uses a hidden destination pointer in
  `$a0` (all other arguments shift by one slot). This is simpler than n64's
  register-chunk struct passing and is only observable when calling an *external*
  C function with a record by value, which the language never declares.
* `$s0-$s7` and `$f24-$f31` are callee-saved; a function saves exactly the ones
  it allocated, plus `$ra` if it makes a call.

### Canonical register form for integers

Each integer value is kept extended to 64 bits according to its own type: signed
types sign-extended, unsigned types zero-extended, booleans 0/1. Then 64-bit
compares (`slt`/`sltu`) are correct for every width without per-use extension.
`i32` arithmetic uses the 32-bit instructions (`addu`, `subu`, `mul`, `sll`, ...),
whose results MIPS64 defines as sign-extended, so it costs nothing. Other widths
compute in 64 bits and re-normalize the result (`seb`, `seh`, `andi`, `dext`,
`sll x,x,0`) only when the operation can produce out-of-range bits.

### Frame layout

```
high  | saved $ra, $s*, $f24-$f31        |
      | spill / home slots (on demand)   |
      | IR objects (params, locals)      |
$sp → | outgoing stack arguments         |
low
```

`$sp` is the only frame base (no `$fp`); the size is fixed after allocation and
rounded to 16 bytes. Every IR object gets a slot with its own alignment.
Parameters arriving in registers are stored to their slots in the prologue (the
peephole pass removes the store/reload pairs that this creates). The prologue and
epilogue are generated after the body, once the set of spill slots and saved
registers is known.

## Register allocation: next-use + register/address descriptors

Allocation follows the block-local algorithm from the course slides (Lectures
33-34: next-use information, `getReg`), applied to every IR basic block.

* **Liveness and next-use.** A global backward liveness analysis over the CFG
  (`ir::LiveOut`) gives the names (temporaries and promoted variables) live on exit
  from each block. A backward scan over the block then attaches to each instruction
  the live/next-use state of its result and operands, then kills the result and marks
  operands live with next use here, starting from that live-out set. This replaces
  the slides' simplification that every user variable is live at block exit. The scan
  also records whether a call lies between a definition and its next use.
* **Descriptors.** A register descriptor lists the names whose current value a
  register holds; an address descriptor lists where a name's current value lives
  (registers and/or its memory home).
* **getReg(x = y op z).** For an operand, reuse a register already holding it;
  else take a free register; else evict the register whose names are all
  safe (stored elsewhere, equal to x, or dead), spilling otherwise, preferring
  the one with the farthest next use. For the result, prefer a register holding
  only x, else an operand register whose value is dead after this instruction,
  else the operand rule. `x = y` emits no code: x joins `y`'s register.
  Values that are live across a call prefer callee-saved registers (`$s*`,
  `$f24-$f31`), so a call only forces spills of caller-saved registers.
* **Variables.** Scalar IR objects whose address is only ever used directly by
  `load`/`store` of the object's own type (never offset, passed, stored or cast)
  cannot be aliased. They are treated as variables: `store x, v` is a copy and
  `load x` reuses the register. Dirty values are written back to the object's
  slot at block end. All other memory (globals, escaping locals, pointers) is
  accessed with a real load/store each time, and nothing cached is assumed
  valid across a store through a pointer or a call.
* **Block end.** Before the terminator, every *globally live-out* name whose only
  copy is in a register is stored to its home, so each block starts with empty
  descriptors (a live-in name is live-out of every predecessor, hence in memory).
  Dead variables are never written back. Integer `if_goto` on an integer compare defined in the same
  block, with no other use, is fused into one `beq`/`bne`/`slt`+branch.

Allocatable registers are `$t0-$t3`, `$t8`, `$s0-$s7` (integer) and
`$f2-$f11`, `$f20-$f31` (floating). `$v0`, `$v1` and `$f0`, `$f1` are
selection scratch and return registers; `$t9` holds call targets and computed
addresses; `$a*`/`$f12-$f19` are used only while staging a call; `$at` is the
assembler's.

## IR optimization

The optimizer first discovers natural loops and fully unrolls eligible
straight-line counted loops with 2–4 iterations and at most 16 body instructions.
The following local passes then simplify the resulting blocks. Dead-code
elimination uses global liveness:

| Pass | Safety condition |
| --- | --- |
| Constant folding, copy propagation | Shared `numeric` evaluation; a fact dies when its temporary or source is redefined. |
| Algebraic identities, strength reduction | Integers only (`x*2^k` to shift, unsigned `/`/`%` by powers of two, `x-x`, `x^x`, `x*0`, ...). Signed division is untouched because it rounds toward zero. |
| Value numbering (DAG CSE) | Pure operations, comparisons, casts and address offsets; commutative integer operands match in either order; floating only for identical operands. |
| Redundant load / store-to-load forwarding | Identical address operand and type; a call, zeroing, destructor registration or a store through a pointer forgets every fact. |
| Dead-store elimination | Identical named-object address stored again with no possible read between. |
| Dead-code elimination | Backward next-use scan seeded with the block's global live-out temporaries (`ir::LiveOut`; temporaries are mutable, so every definition kills). |
| Constant branch folding | Looks only at the block's own `if_goto`/`goto` tail; unreachable blocks are then deleted as cleanup. |

After that, **function inlining** (`src/ir/inlining.cc`) replaces a direct call with a
copy of the callee when the callee's peak register pressure is at most 6 live values
(temporaries plus register-resident scalar objects, counted with `ir::LiveOut`). This
measures what the allocator actually needs; counting every distinct temporary would
reject even trivial functions, since each subexpression is a new temporary. Inlining only
copies code, and the local passes run again to clean up. Details: wiki page 4.

Copy and constant propagation stay local to a block. `custom_tests/ir_opt` pins the optimized TAC for each pass,
including the negative cases (no forwarding across a call or a pointer store, no reuse
after a redefinition).

## MIPS peephole optimization

`src/codegen/peephole.cc` iterates the rules below inside machine basic blocks
(labels and branches are boundaries), using exact register liveness from
`Uses`/`Defs`. Selection marks every IR block label with the registers live there
(only the frame, `$ra` and callee-saved registers, because descriptors are flushed at
block ends), so dead-code rules can fire at block exits too.

* redundant reloads of a `$sp` slot become `move`/`mov.d` or disappear; a store
  overwritten before any possible read is deleted (slides: "redundant loads and stores");
* dead instructions, `move r, r`, `daddiu r, r, 0`; forward copy propagation and
  backward retargeting (`lw $t0, ..; move $v0, $t0` into `lw $v0, ..`);
* address folding `daddiu t, b, k; ld r, j(t)` into `ld r, k+j(b)`;
* immediate forms (`dli t, k; daddu x, s, t` into `daddiu x, s, k`);
* strength reduction of `mul`/`dmult` by powers of two;
* flow of control: branches to the next label, branch-over-jump inversion, unreachable
  code after an unconditional jump, unreferenced local labels;
* finally **delay-slot filling**: the nearest independent preceding instruction replaces
  the `nop` after a branch, call or return (never a multi-instruction `dli`/`dla` macro,
  never an instruction a branch reads, and never across a label).

`custom_tests/codegen/peephole_tests.cc` (CTest `peephole`) checks each rule and its
negative cases on hand-written machine code.

Across all execution fixtures, optimization reduces static instruction count by about
23% (18,602 to 14,373 lines; 35% on `15_algorithms.cpp`).

## Language rules that matter here

* The entry is `main()` or `main(int argc, char** argv)`; a test reads its input with
  `atoi(argv[1])` (see Program entry).
* Integer literals outside `int` range need an `L`/`UL` suffix; string literals have
  type `const char*`; braced arrays of records cannot be initialized from variables.
* Integer arithmetic wraps at its width (no undefined overflow); booleans are 0/1 in
  one byte. Records are first-class values in the IR, hence the record calling rules.
* Static destructors run after `main` returns, in reverse registration order.

## Testing

```bash
make test-codegen                              # or:
python3 custom_tests/codegen/run.py --mode both [cases...] [--keep]
ctest --test-dir build                         # ir, ir_opt, peephole, codegen, semantic
```

`custom_tests/codegen/run.py` compiles every IR fixture (`custom_tests/ir/cases.json`)
and every codegen fixture (`custom_tests/codegen/cases.json`: libc I/O, widths, floats,
records, many arguments, recursion, register pressure, big frames, conversions), with and
without optimization, links each against `harness.c` and runs it under QEMU in one Docker
invocation. It checks `main`'s full return value, a named global after static destructors
have run, and exact stdout. Expected values for the codegen fixtures were produced by
compiling equivalent C++ natively (`-fwrapv -ffp-contract=off`).


## Limitations

* Allocation is block-local by design: a value live across a block boundary is stored
  at the end of the block and reloaded in the next, so loop variables stay in memory.
  Global liveness only removes the stores of values that are dead there.
* Record arguments use the pointer-to-snapshot rule above, not n64's register chunks;
  calling an *external* C function that takes or returns a record by value is not ABI
  compatible.
* Record copies are unrolled up to 256 bytes and looped above that; the prologue
  always unrolls (incoming `$a*` registers are still live there).
