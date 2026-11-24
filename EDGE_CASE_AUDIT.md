# Edge-case audit — baka compiler (`saturn/slop2`, commit `1cb767a`)

Scope: surface-level, language-level edge cases after parsing (semantic analysis, IR
lowering + block-local optimizer, MIPS64 codegen). MIPS-internal details (immediate
ranges, delay slots, frame displacement limits, ABI slot numbering) were deliberately
not probed.

~350 test programs across four areas. Every program was run unoptimized and optimized
and compared against native `g++ -fwrapv -ffp-contract=off` and/or the IR interpreter.
Codegen tests ran for real under qemu-mips64el (Docker image `baka-mips64el`).

**Bottom line:** no miscompiles, no compiler crashes/hangs on valid output paths, no
verifier failures on generated IR, and optimized output always matched unoptimized.
The one real problem is compile time.

---

## 1. Compile time is super-linear in function size (most important)

| Repeated `if` statements in one function | `--ir` | `--ir --optimize` |
|---|---|---|
| 25  | 0.28 s | — |
| 50  | 2.1 s  | — |
| 100 | 16.9 s | 32.8 s |
| 1500 | > 20 s (timeout) | > 20 s (timeout) |

Test line repeated N times: `a = a * 3 + b; b = b ^ a; if (a > K) a = a % 977;`

- Also: 40 nested blocks each holding an object with a destructor → 10 s (`--ir`),
  20 s (`--ir --optimize`); a 10,000-term expression → 8.5 s in `--codegen`.
- **Cause (gdb samples):** `Verifier::checkDefinitions` in `src/ir/verifier.cc`
  (definite-assignment check), called from `Verify` ← `Lowerer::lower`. Each block
  keeps full `std::set<uint32_t>` in/out sets that are copied and intersected on every
  pass; iterates to a fixed point without a worklist or reverse-postorder. The verifier
  runs again after optimization, so `--optimize` doubles the time.
- **Contributing:** `src/CMakeLists.txt` applies `-O2` only to the `baka` target; the
  `ir`, `semantic`, etc. libraries build unoptimized (empty `CMAKE_BUILD_TYPE`).
- Separately, `--semantic --verbose` on the 10k-term expression prints 1.4 GB of AST
  (6.5 s); analysis itself is fast (`--ir` 0.27 s).

## 2. Front-end bugs

| Issue | Example | Behaviour |
|---|---|---|
| Function returning a function pointer doesn't compile | `int (*pick(int i))(int p, int q) { return add; }` | IR: "function parameter count/type mismatch"; using `i` in the body gives "undeclared name 'i'" — the inner `(int p, int q)` is taken as `pick`'s parameters. Workaround: wrap the pointer in a struct. |
| String literal for a `char[]` member in a brace initializer is rejected | `class R { int a; char c[3]; }; R g = {7, "hi"};` | "invalid member initializer (SEMANTIC)". `{104,105,0}` works. |
| Const bypass through method calls | `const C c = ...; c.set(5);` (also via `const C*`, const globals) | Accepted, though `c.v = 5` is rejected. The language has no `const` methods, so there is no correct way to express this. |
| Implicit pointer→bool rejected (inconsistent) | `bool b = p;`, `b = p;`, `f(&x)` with `f(bool)` | Rejected, but `if (p)`, `!p`, `p && n`, `p ? :` are accepted. |
| `enum E e = 3;` accepted | | C rule, not C++. Low. |
| No missing-return diagnostic | non-void function with a path lacking `return` | Silently returns 0 (documented). |
| No warning for constant division by zero | `1 / 0`, `n % 0` | Low. |
| Unknown type gives a confusing error | `Foo x;` | "expected ';' after expression (PARSE)" instead of "unknown type". |
| `-2147483648` as a case label | `case -2147483648:` | Lexer error "Integer literal overflows int". |
| `A arr[5] = {A(10), A(20)};` rejected | | "invalid member initializer" (may be the documented ban on braced construction). |

## 3. Undefined behaviour where codegen and the IR interpreter disagree

Legal (UB in C++), but compiler and interpreter should probably agree.

| Case | Codegen (opt = noopt) | Interpreter |
|---|---|---|
| `int` shift by ≥ 32 | amount mod 32 (`1 << 33` = 2, `-1000 >> 32` = -1000) | 0 / -1 |
| `unsigned int` shift by ≥ 32 | 0 | 0 |
| Division by zero | no trap: `x / 0` = x, `x % 0` = 0 | throws |
| Out-of-range float → int | 64-bit truncate then low bits: `(int)NaN` = -1, `(int)inf` = -1, `(int)3e10` = low 32 bits | throws |

Note the signed/unsigned 32-bit shift inconsistency with each other (`sllv`/`srav`
for `int` vs 64-bit `dsllv`/`dsrlv` for `unsigned`, `src/codegen/select.cc:1328-1331`).

## 4. Differences from g++ that are not bugs

- `INT_MIN / -1`, `LONG_MIN / -1` wrap (quotient MIN, remainder 0), matching the
  interpreter; g++/x86 traps.
- Arguments evaluated left to right (as `ir_context.md` specifies); g++ goes right to left.
- Returning a local object prints an extra destructor line: no NRVO (copy elision is
  optional). Ctor/dtor counts balance; `g++ -fno-elide-constructors` matches.
- NaN prints as `nan` on MIPS vs `-nan` on x86.

## 5. Unsupported syntax (clean errors, mostly documented)

- type spellings: bare `unsigned`, `signed char`, `signed int`, `long int`, `short int`,
  `unsigned short int`
- direct initialization `C c(4)` (use `C c = C(4)`)
- references, including `T(const T&)`
- `const` methods, `private:` / `public:`, `int* const p`
- unnamed function-pointer parameters `int (*f)(int, int)`; `typedef` of a function-pointer type
- `::name`
- declaration directly after a `case` label
- `(T(3), T(4)).v` (parsed as a cast)

## 6. Confirmed correct

**Semantic:** scoping/shadowing, redeclaration, use-before-declaration, overload
resolution and ambiguity, wrong arity/types, return-type checks, lvalue checks, const on
variables/pointers/members, conversions (int/float/char/bool, signed/unsigned, null,
`void*`), pointer arithmetic, multi-dim arrays, self-referential and nested structs,
inheritance, ctors/dtors/`this`, break/continue placement, switch rules (duplicate and
non-constant labels, two defaults), goto labels, 3,000 nested parentheses. Every invalid
program produced a located diagnostic.

**IR / optimizer:** nested break/continue, `for` step on continue, do-while, switch
fallthrough/default-in-middle/large and negative labels, goto, short-circuit side
effects, ternary, comma, compound assignment with side-effecting index evaluated once,
destructors on every exit (return/break/continue/goto, temporaries, loops, arrays,
statics, globals in order), integer wrap at all widths, negative div/mod, shifts in
range, promotions, NaN/inf/-0.0 not wrongly folded, copy propagation after
redefinition, aliasing stores, loads after calls, dead stores read via alias, strength
reduction incl. negatives/INT_MIN, constant branch folding, unions.

**Codegen arithmetic:** overflow wrap for every width/signedness, div/mod with
negatives, comparisons against all constant sizes, 64-bit constants, `unsigned int`
≥ 2^31 across calls, all int↔float conversions incl. `unsigned long` ≥ 2^63, NaN/inf in
every comparison and branch, printf with doubles past 8 args, register pressure (40 ints
+ 32 doubles across loops/calls/recursion).

**Codegen ABI / runtime:** calls with 0–20 mixed-type args, printf with 12 doubles,
100k-deep recursion (also with 4 KB frames), function pointers everywhere, structs by
value 1–1000 bytes (incl. in many-arg calls and deep recursion), 150 KB+ local arrays,
global initializers incl. pointers between globals and runtime-initialized globals,
static locals, static destructor order identical to g++, all `main` variants and return
values, heap new/delete incl. allocation failure, fopen/fread/fwrite, write, scanf,
printf formatting.

---

## Reproducing

All four worktrees were created from stale `master` (`b6f9a26`); each agent detached to
`1cb767a`. Nothing was committed. Tests and runners live in:

| Area | Path | Runner |
|---|---|---|
| Semantic | `.claude/worktrees/agent-a11230207af985ff2/edge_audit/semantic/` | generator/runner scripts were in a session scratch dir (now gone) |
| IR / optimizer | `.claude/worktrees/agent-a625df14b67ca7371/edge_audit/ir/` | `python3 edge_audit/ir/check.py [files]` |
| Codegen arithmetic | `.claude/worktrees/agent-ae11df94c30099935/edge_audit/codegen_arith/` | `python3 edge_audit/codegen_arith/audit.py [stems]` (`KEEP=1` keeps `.s`) |
| Codegen ABI/runtime | `.claude/worktrees/agent-aa21c611a8388e081/edge_audit/codegen_abi/` | `python3 edge_audit/codegen_abi/audit.py [names]` |

Remove the worktrees with `git worktree remove <path>` when done.
