# 7. Calls, frames and the ABI

## Calling convention (n64)

- **Arguments:** the first eight go in registers: integers and pointers in `$a0`–`$a7`,
  `float`/`double` parameters in `$f12`–`$f19`. Further arguments go on the stack in the
  caller's outgoing-argument area, 8 bytes each.
- **Variadic arguments** (for example to `printf`) always use integer registers or the
  stack, even for doubles, as n64 requires.
- **Results** come back in `$v0` (integers, pointers) or `$f0` (floats).
- **Width rules:** 32-bit integers cross call boundaries sign-extended, including
  `unsigned int`, as n64 requires; narrower types are extended by their own signedness.
  The receiver re-establishes the canonical form ([page 5](05-instruction-selection.md)).
- **Saved registers:** a function saves exactly the callee-saved registers it uses, plus
  `$ra` if it makes any call.

## Records by value: baka's own extension

n64 has rules for passing structs in registers, but baka uses something simpler:

- **A record argument** is passed as a pointer to a caller-owned copy. The callee copies
  it into its own parameter object.
- **A record result** is written through a hidden pointer the caller passes in `$a0`.
  All other arguments shift along by one.

This is only incompatible with real C code if baka calls an external C function that
takes or returns a struct by value, which the language never declares.

## Stack frame

```
high addresses
  saved $ra, $s*, $f24–$f31
  spill slots / homes for temporaries   (created on demand)
  IR objects: parameters and locals
  outgoing stack arguments              ← $sp
low addresses
```

- `$sp` is the only frame base; there is no frame pointer.
- The frame size is fixed for the whole function and rounded to 16 bytes.
- Every object gets a slot with its own alignment. Temporaries get an 8-byte "home" only
  if they are ever spilled or saved at a block end.
- Large frames are fine: offsets that don't fit an instruction's 16-bit displacement are
  formed in a scratch register first.

## Prologue and epilogue

These are generated **after** the body, once the frame size and the set of used
callee-saved registers are known.

- **Prologue:** move `$sp` down, save registers, then store incoming parameters (from
  argument registers or the caller's stack area) into their slots. Record parameters are
  copied from the caller's snapshot.
- **Body:** every `return` jumps to a single shared exit label.
- **Epilogue:** restore registers, move `$sp` back up, return through `$ra`.

The peephole pass later removes most of the redundant store-then-reload pairs that the
prologue creates.

## Function pointers

Calls through a pointer work the same way: the pointer value is moved into `$t9` and
called with `jalr`.
