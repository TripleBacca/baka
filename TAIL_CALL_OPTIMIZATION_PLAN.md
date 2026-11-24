# Tail-call optimization plan

## Current behavior

At the baseline commit `a5bbcea`, Baka did not optimize tail calls in user code. Compiling an
accumulator-style recursive function and a forwarding wrapper with
`--ir --optimize` leaves `Param`, `Call`, `Return` in both. With
`--codegen --optimize`, both still use `jalr $t9` and return through their own
epilogues. The weak C `main` already jumps to `__baka_start` with `jr $t9`.
That jump is written directly in the runtime emitter.
([IR optimizer](src/ir/optimization.cc), [call and return selection](src/codegen/select.cc),
[runtime emitter](src/codegen/runtime.cc))

Phase 1 is implemented in the IR optimizer. Phase 2 remains planned for calls
between functions, including mutual recursion. Both use the existing optimization
flag and leave the ordinary call path available.

## 1. Turn direct self-recursion into a loop in the IR

For example, `return sum(n - 1, acc + n)` can evaluate both new arguments, write
them to the parameter objects, and jump back to the first IR block. The function
prologue runs on initial entry. The back edge targets the first body label, so
each iteration reuses the same stack frame. The eventual base case still returns
through the normal epilogue.

The [tail recursion pass](src/ir/tail_recursion.cc) separates the matcher, frame
check, and rewrite. The pass first collects sites without changing the function,
checks the frame once, then applies each rewrite to its own block.

1. **Match the verified IR.** Require a block ending in contiguous `Param`
   instructions, a direct `Call`, and a `Return` of that exact result temporary.
   Identify a self-call by function ID, which also distinguishes overloads.
   Start with fixed-arity functions with integer or floating-point parameters
   and result. Post-call arithmetic, conversions, and destructor cleanup leave
   the original call in place. Return lowering exposes cleanup as instructions
   and control flow before the final `Return`. [Return lowering](src/ir/jumps.cc)
2. **Prove the frame can be reused.** For the first version, allow only parameter
   objects in the frame and inspect all their uses. The [parameter address
   rule](#parameter-addresses-and-frame-reuse) establishes whether a later
   invocation could observe the previous invocation's storage.
3. **Update parameters as a parallel assignment.** The lowerer has already
   evaluated argument expressions before emitting `Param`. Reuse the captured
   operands, then emit the typed stores and a `Jump` to the first IR block.
   Preserve the evaluation order and avoid reading a parameter after overwriting
   it, including for `return f(n - 1, b, a)`. Rewrite each eligible tail site
   independently, keeping base cases and ordinary calls. This uses existing IR
   opcodes and does not need a new calling convention.
   [IR instruction contract](src/ir/ir.hh)
4. **Integrate with optimization and verification.** Call `EliminateTailRecursion`
   after counted-loop unrolling and the first simplification, so a removable result
   copy can disappear before matching. Run it before inlining, which can add local
   objects to a frame. Call `BuildCFG` on changed functions and use the existing
   post-optimization `Verify` in the lowerer. The new back edge is then visible to
   IR and backend liveness.
   [CFG builder](src/ir/cfg.cc) · [Lowering pipeline](src/ir/lowering.cc)

### Parameter addresses and frame reuse

The IR represents a parameter slot's address as an `Object` operand. A `Load`
through that address produces a separate value in a temporary. `Param` can pass
either that loaded value or the address itself, depending on the source expression.

| Source use | Simplified IR | What a later invocation can observe |
| --- | --- | --- |
| `walk(n - 1, n)` | `load t, &n`, then `param t` | The copied value stays the same after the store of the next `n`. |
| `walk(n - 1, &n)` | `param &n` | Reusing the slot would make the pointer read the new `n` instead of the previous invocation's value. |
| `saved = &n; return walk(n - 1)` | `store &saved, &n` before the call | The callee can retrieve the address through `saved`, even when every call argument is numeric. |

The pass scans the entire function. A parameter address can appear only as the
address operand of a whole-object `Load` or `Store` at offset zero. A store to
`n` uses its address as the destination and is allowed. A store of `&n` uses
its address as the value and makes the function ineligible. Passing, copying,
returning, or doing address arithmetic on `&n` also makes it ineligible. This
conservative rule catches the first use that makes access through another
pointer possible, so the pass needs no pointer tracking. The recursive frame
remains available whenever a use falls outside the rule. The regression fixture
checks the example where a global saves `&n`. [Address-escape regression](custom_tests/ir_opt/14_tail_recursion_skip.cpp)

The numeric-only signature check also excludes pointer parameters that could
safely point to globals, the heap, or an older caller's frame. To allow pointer
parameters, prove that the values they carry cannot refer to the frame being
reused. To add scalar locals, require that each local be initialized on every path
before any read in a new invocation, along with the same address-use check. The
verifier currently checks definite assignment of temporaries.
[Frame check](src/ir/tail_recursion.cc) · [IR verifier](src/ir/verifier.cc)

## 2. Emit sibling tail jumps in the MIPS backend

`return other(x)` can return directly to the original caller if Baka stages
`other`'s arguments, releases the current frame, and transfers control without
overwriting `$ra`. Keep `Param`, `Call`, `Return` in the IR for this phase and
select the tail jump in the backend, where the calling convention is known.
[GCC sibling-call eligibility](https://gcc.gnu.org/onlinedocs/gcc-7.3.0/gccint/Tail-Calls.html)

1. **Limit the first ABI case.** Use the same terminal call/return match under
   `Options.optimize`. Require a directly named, defined Baka callee, a
   non-variadic signature, identical caller and callee result types, and only
   integer or floating-point values. Require all arguments to fit in the first
   eight n64 slots. Require proof that no caller-frame object remains observable
   by the callee, conservatively using the same direct-load/store address rule.
   Retain normal calls for stack-passed arguments, records and hidden result
   pointers, variadics, externals, function pointers, and virtual dispatch until
   each has its own ABI and lifetime checks.
   [Baka call ABI](wiki/07-calls-frames-abi.md) · [MIPS n64 interface](https://pixelbart.net/SGI/IRIX/i/65/30/doc/usr/share/Insight/library/SGI_bookshelves/SGI_Developer/books/Mpro_n32_ABI/sgi_html/ch02.html)
2. **Stage arguments and add a tail exit.** Reuse the register argument staging
   from `emitCall`, including the n64 boundary extension for 32-bit integers.
   Route the site to a tail exit emitted by `finish` after the frame layout is
   known. Restore the saved registers and any saved `$ra`, restore `$sp`, then
   load the callee symbol into `$t9` and emit `jr $t9` with a safe delay slot.
   Load `$t9` after the restores because the current large-offset and stack
   adjustment helpers can use it as scratch. Keep `$ra` saving for any ordinary
   calls the function still makes. [MIPS selector](src/codegen/select.cc) ·
   [Sibling-call epilogue requirements](https://gcc.gnu.org/onlinedocs/gccint/General-Standard-Pattern-Names.html)
3. **Model the exit in machine liveness.** At the tail-exit label and transfer,
   keep staged argument registers live along with the stack and link state. The
   existing machine `Jr` is a terminal transfer and accepts implicit register
   uses. Check its liveness, copy propagation, and delay-slot rules for the
   `$t9` case, starting with a `nop` if needed. A machine-level test should catch
   any removed argument setup or reordered frame teardown.
   [Machine peephole](src/codegen/peephole.cc)

## Acceptance tests

- **IR shape and results.** In `custom_tests/ir_opt`, check for the parameter
  stores and back edge, and for the absence of the self-call at the rewritten
  site. Use the interpreter to compare optimized and unoptimized results for
  an accumulator, swapped arguments, and a function with multiple tail sites.
  Check that a post-call addition, an actual return conversion in a sibling
  call, a destructor after the call, and an address escape retain their calls.
- **Assembly and ABI.** In `custom_tests/codegen`, check that the optimized
  self-call becomes a branch within the function and the sibling call ends in
  `jr $t9` after frame teardown. Check that `--no-optimize` keeps `jalr`. Assemble,
  link, and run both modes under QEMU. Exercise mixed integer and floating-point
  register slots, high-bit unsigned results, a caller that makes an ordinary
  call before its tail exit, and the fallback for more than eight outgoing
  arguments, records, and indirect calls.
- **Bounded stack.** Add a QEMU test with deep self-recursion and, once sibling
  calls land, deep mutual recursion. Use a controlled guest stack limit so the
  unoptimized frame-per-call version cannot pass the deep case. Keep the
  optimized/unoptimized result comparisons at small depths, within the IR
  interpreter's step limit.
