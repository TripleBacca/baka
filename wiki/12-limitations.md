# 12. Limitations and known gaps

## Design trade-offs (intentional)

- **Block-local register allocation.** Values never stay in registers across block
  boundaries, so loop variables are stored and reloaded every iteration. Global liveness
  only avoids storing values that are dead.
- **Limited CFG optimization.** Full unrolling handles small fixed-count loops.
  Loop-invariant code motion, global constant propagation and jump threading remain open.
- **Records by value use a private convention** (pointer to a copy; hidden result
  pointer), so calling an external C function that takes or returns a struct by value
  would not be ABI-compatible. The language never declares such functions.
- **Non-PIC, static only.** The output cannot become a shared library or a PIE.
- **Record copies** are unrolled up to 256 bytes and looped above that.
- **Inlining has no size limit.** Any callee with peak register pressure of at most 6 is
  copied into every call site, however long it is, so heavily used helpers can make the
  optimized code several times larger (page 4).
- **Struct `==` needs identical types.** A struct has no runtime type, so `SD == SB` (even
  through `SB&` naming the same object) is an error, while the same comparison on classes
  works through the vptr. See page 19.
- **Unions cannot be compared** with `==`, even as a nested field, because the active
  member is unknown.
- **`==` and `====` trust named objects.** A named object is treated as having exactly its
  declared class even while a base constructor runs on it. From code reached during `d`'s
  construction, `d ==== other` can fold to `true`, and `d == other` can compare `d` as its
  final class and read fields that are not yet initialized. Call devirtualization has the
  same blind spot. See pages 19 and 20.

## Findings from the edge-case audit

About 350 test programs were run across the front end, IR and codegen, each optimized and
unoptimized, against `g++` and the IR interpreter. No miscompiles were found. The full
report is in `EDGE_CASE_AUDIT.md` at the repository root; the highlights are below.

### Compile time (fixed)

Compile time used to grow steeply with function size: about 17 seconds for a function
with 100 `if` statements. The cause was the IR verifier's definite-assignment check, which
copied whole `std::set`s on every pass. It now uses one bit vector per block, and the same
function compiles in about 0.15 seconds (fixed).

### Unusual `main` signatures (fixed)

The audit found that `double main`, `int main(double)`, a struct-returning `main` and a
three-parameter `main` all compiled but misbehaved at run time. Commit `92ecefb` fixed
this: semantic analysis now accepts only `main()` and `main(int argc, char** argv)`
returning an integer type or `void`.

### Undefined behaviour handled differently from the interpreter

| Case | Codegen | Interpreter |
|---|---|---|
| `int` shift by 32 or more | amount taken mod 32 | 0 or -1 |
| `unsigned` shift by 32 or more | 0 | 0 |
| Division by zero | no trap (`x / 0` = x, `x % 0` = 0) | error |
| Out-of-range float → int | low bits of a 64-bit truncation | error |

### Front-end gaps that affect what you can compile

- A function returning a function pointer doesn't compile.
- A string literal can't initialize a `char[]` member in a brace initializer.
- A method that modifies an object can be called on a `const` object.
- Implicit pointer-to-`bool` is rejected in assignments and arguments but accepted in
  conditions.
- Unsupported syntax includes bare `unsigned`, `signed char`, `const` methods, rvalue
  references, lambda captures, constructor initializer lists and direct
  initialization `C c(4)`. Access specifiers, lvalue references, captureless lambdas and
  virtual functions are supported (pages 15–18). Classes differ from C++ by design: every
  method is virtual and every class carries a vptr (page 18).

### Not bugs, but different from `g++`

- `INT_MIN / -1` wraps instead of trapping.
- Arguments are evaluated left to right.
- No return-value copy elision, so returning a local object runs one extra
  copy-and-destroy.
