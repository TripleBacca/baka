# 2. Target and environment

## The target: MIPS64r2, n64 ABI, little-endian

The front end already assumes a 64-bit ("LP64") machine: `long` and pointers are 8
bytes, records are laid out with 8-byte pointers, and the IR uses 64-bit values for
addresses and array offsets. The n64 ABI is LP64 too, so:

- every IR scalar fits in one machine register;
- every struct layout the semantic layer computed is already the native layout;
- no earlier stage had to change.

The 32-bit alternative (o32) would have meant either changing the front end's target and
every IR fixture, or emulating 64-bit values with register pairs.

Little-endian matches the byte order the IR test interpreter assumes, so memory images
agree between the two.

## The environment: Linux user mode, glibc, QEMU

The language auto-declares C library functions (`printf`, `scanf`, `fopen`, `fread`,
`fwrite`, `read`, `write`). Using real glibc means these just work. A MIPS simulator such
as SPIM or MARS would have needed a hand-written libc in assembly.

The toolchain lives in a Docker image (Debian, `gcc-mips64el-linux-gnuabi64` and
`qemu-user`), so nothing is installed on the host. QEMU runs the MIPS binary as an
ordinary Linux process on an x86 machine.

## Static, non-PIC code

- **Static linking:** glibc's code is copied into the executable; nothing is loaded at
  run time.
- **Non-PIC (`.option pic0`):** baka's own code uses absolute addresses. MIPS Linux
  toolchains default to position-independent code, where every global is reached through
  a table (the GOT) via the `$gp` register. Opting out keeps the generated code simpler:
  no GOT loads, no `$gp` setup in every function. The cost is that the binary must be
  linked at a fixed address (`-static -no-pie`).

## The one rule this forces: calls go through `$t9`

glibc inside the static archive is still compiled as PIC. Each glibc function computes
`$gp` on entry from `$t9`, assuming the caller put the function's own address there. So
every call baka emits loads the target into `$t9` and jumps through it
(`dla $t9, f` then `jalr $t9`), never a direct `jal f`. baka's own functions don't read
`$t9`, so calling them the same way is harmless and keeps one calling sequence.

## Branch delay slots

MIPS executes the instruction after a branch, jump or call before the transfer takes
effect. The backend writes assembly under `.set noreorder`, meaning it manages these
"delay slots" itself: every transfer is initially followed by a `nop`, and the peephole
pass later replaces some of those with useful work.
