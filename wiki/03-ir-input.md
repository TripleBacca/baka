# 3. The IR the backend consumes

The backend's input is a verified, typed three-address code (TAC) module. The full
contract is in `ir_context.md`; this page covers what matters for code generation.

## Module contents

- **Types:** integers of 1 (bool), 8, 16, 32 and 64 bits, signed or unsigned; 32- and
  64-bit floats; pointers; functions; records (structs/classes) and arrays with computed
  size, alignment and field offsets.
- **Globals:** each with a type, an initial byte image, and optional initial values that
  are addresses of other globals or functions.
- **Functions:** each with parameters, local objects, temporaries and a list of basic
  blocks. Functions that are only declared (library functions) have no body.
- **A module initializer:** a generated function that runs global constructors in source
  order and registers static destructors.

## Two kinds of storage

| Kind | Meaning | Backend treatment |
|---|---|---|
| Temporaries | IR "registers": the results of instructions | Candidates for machine registers |
| Objects | Named memory: parameters, locals, globals | Get a memory slot; may be promoted to registers if never aliased |

Temporaries are **mutable**: the same temporary can be assigned more than once (for
example, in both arms of a conditional). The IR is not SSA.

## Instruction families

- **Arithmetic and comparisons:** add, subtract, multiply, divide, remainder, shifts,
  bitwise ops, negate, complement, logical not, and the six comparisons.
- **Memory:** `load`, `store`, `zero_memory`, and address offsets for fields and
  elements.
- **Conversions:** casts between any scalar types.
- **Calls:** a run of `param` instructions immediately followed by `call`. No other
  instruction can appear between them, which simplifies argument handling.
- **`register_dtor f, p`:** arrange for `f(p)` to run after `main` returns.
- **Terminators:** `goto`, `if_goto`, `ret`. Every block ends with exactly one.

## Semantics the backend must honour

- Integer arithmetic wraps at the type's width; there is no undefined overflow.
- Booleans are 0 or 1.
- Records are first-class values: they can be loaded, stored, copied, passed and
  returned by value.
- Static destructors run after `main`, in reverse registration order.
- Arguments are evaluated left to right.

## What the backend gets from the IR layer for free

- **A CFG** (successors and predecessors of each block).
- **Global liveness:** which temporaries and variables are live on exit from each block.
  The allocator uses this to avoid saving dead values at block ends.
