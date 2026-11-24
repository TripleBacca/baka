# 12. Limitations and known gaps

## Design trade-offs (intentional)

- **Block-local register allocation.** Values never stay in registers across block
  boundaries, so loop variables are stored and reloaded every iteration. Global liveness
  only avoids storing values that are dead.
- **Block-local IR optimization.** No loop-invariant code motion, global constant
  propagation or jump threading.
- **Records by value use a private convention** (pointer to a copy; hidden result
  pointer), so calling an external C function that takes or returns a struct by value
  would not be ABI-compatible. The language never declares such functions.
- **Non-PIC, static only.** The output cannot become a shared library or a PIE.
- **Record copies** are unrolled up to 256 bytes and looped above that.

## Findings from the edge-case audit

About 350 test programs were run across the front end, IR and codegen, each optimized and
unoptimized, against `g++` and the IR interpreter. No miscompiles were found. The full
report is in `EDGE_CASE_AUDIT.md` at the repository root; the highlights are below.

### Compile time

Compile time grows steeply with function size: about 17 seconds for a function with 100
`if` statements. The cause is the IR verifier's definite-assignment check, which copies
whole sets on every pass and iterates without a worklist. It also runs a second time after
optimization.

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
- Unsupported syntax includes bare `unsigned`, `signed char`, references, `const` methods,
  access specifiers and direct initialization `C c(4)`.

### Not bugs, but different from `g++`

- `INT_MIN / -1` wraps instead of trapping.
- Arguments are evaluated left to right.
- No return-value copy elision, so returning a local object runs one extra
  copy-and-destroy.
