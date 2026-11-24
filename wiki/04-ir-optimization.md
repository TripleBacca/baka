# 4. IR optimization

With optimization on, the IR is simplified before instruction selection.

## Loop unrolling

The unroller finds natural loops from CFG edges whose destination dominates their
source. It fully unrolls straight-line `for (int i = 0; i < N; i++)` loops with
2–4 iterations and at most 16 body instructions. A candidate must have a private
counter that the body cannot change, and its body must have no calls. The copies
use constant counter values and join the original exit. The passes below then
simplify instructions within each block, using global liveness for dead code.

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

## Function inlining

`ir::Inline` (`src/ir/inlining.cc`) replaces a direct call with a copy of the callee when
the callee needs **at most 6 registers**, measured as its *peak register pressure*: the
largest number of values live at the same time. Values are temporaries plus the scalar
locals and parameters that the backend keeps in registers (those whose address is only
used by whole-value loads and stores). Records, arrays and address-taken objects live in
stack memory and cost no register. The count comes from a backward walk of each block
starting at its global live-out set (`ir::LiveOut`).

```cpp
int sq(int x) { return x * x; }     // peak 2  → inlined
int wide(int a, int b, int c, int d) {
    int p = a * b; int q = c * d; int r = a + c;
    return p + q + r + (b ^ d) + (a - d) + (b - c);
}                                    // peak 7  → stays a call
```

How a call is expanded:

1. The caller's block is split at the call. The head stores each argument into a copy of
   the matching parameter object, then jumps to the copied entry block.
2. The callee's blocks, objects and temporaries are copied into the caller under fresh
   ids. Each `ret v` becomes `copy result, v; goto continuation`.
3. The instructions after the call move to the new continuation block.

| Rule | Why |
|---|---|
| Measured after a first round of simplification | Unoptimized IR reloads every variable, which overstates pressure. |
| Only direct calls to defined, fixed-arity functions | Indirect and virtual calls have no known target; declarations have no body. |
| Never directly recursive functions, the module initializer, or anything registering a static destructor | Recursion cannot be expanded finitely, and the other two run once at startup. |
| Up to 3 rounds, with callee bodies snapshotted per round | A helper that calls another small helper is fully expanded by the next round. Copied bodies are not rescanned in the same round, so two small mutually recursive functions stop once each becomes directly recursive. |
| Callees stay defined | Their address may still be taken (function pointers, vtables). |
| Simplification runs again afterwards | Arguments and results now flow through ordinary locals and copies that the block-local passes clean up. Jumps between the copied blocks remain, because merging blocks would be a cross-block transformation. |
| No code-size limit (deliberate) | Register pressure is the only criterion, as chosen for this project. A long callee that needs few registers is copied into every call site, and nested helpers multiply across the three rounds. For example, `custom_tests/codegen/28_random_mix.cpp` calls small helpers many times, and its optimized IR grows from about 2,700 to 7,000 lines. If code size ever matters, add a limit on callee instruction count next to `InlineRegisterLimit`. |

`custom_tests/ir_opt/12_inline.cpp` pins the result: small leaves, a multi-block callee,
a void callee that writes a global, a nested helper, records by value, a too-wide
function, direct and mutual recursion, and a function pointer to an inlined function.

## Verification

The verifier runs again on the optimized IR, so an optimizer bug that produces malformed
IR is caught before code generation. Its definite-assignment check keeps one bit vector of
written temporaries per block, so large inlined functions verify quickly. (It previously
used `std::set` copies, and a heavily inlined function took over 20 seconds.)

## Effect

Combined with the peephole pass, optimization cuts static instruction count by about a
quarter across the execution fixtures, and by over a third on algorithm-heavy ones.
