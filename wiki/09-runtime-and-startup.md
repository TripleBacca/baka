# 9. Runtime and program startup

Every generated `.s` file ends with a small hand-written runtime.

## Startup sequence

```
OS → glibc startup → main(argc, argv)        (generated, weak)
                       → __baka_start(argc, argv)
                           → module initializer   (global initializers, destructor registration)
                           → user's main
                       ← return value
     glibc exit() → registered destructors (reverse order) → flush stdout → exit status
```

### Allowed `main` signatures

`main()` or `main(int argc, char** argv)`, returning an integer type or `void`. Semantic
analysis rejects anything else, so the startup code never has to handle float, struct or
extra parameters. Programs that take input read it with `int n = atoi(argv[1]);`, using
the built-in `atoi`.

### `__baka_start`

The real entry into baka code:
1. Runs the module initializer.
2. Calls the user's `main`, passing `argc` and `argv` if it takes them.
3. Returns the result as a 32-bit integer, or 0 if `main` is `void`.

### The weak C `main`

glibc always calls a C function named `main(argc, argv)`. The user's `main` cannot be
that function, because it is renamed in the assembly. So the backend generates a C `main`
that just jumps to `__baka_start(argc, argv)`.

It is marked **weak**, meaning the linker silently uses any other `main` it is given
instead. That lets the same `.s` file run on its own (`./prog 5`) or be linked with the
test harness. The harness's own `main` calls `__baka_start` with the real `argc`/`argv`,
reports the full 32-bit return value, and can inspect a global after the destructors have
run. On its own, the process exit status keeps only the low 8 bits of the result.

## Static destructors

The IR's `register_dtor f, p` becomes a call to glibc's `__cxa_atexit(f, p, &__dso_handle)`,
the same mechanism C++ compilers use. glibc runs the registered functions when `main`
returns or `exit()` is called, in reverse registration order. That is exactly the IR's
contract. `__dso_handle` identifies the module that registered the destructor; it matters
for shared libraries and is just passed along in a static program.

## Heap helpers

Emitted only if the program allocates:

- `__baka_allocate(size, alignment)` returns null for a bad alignment, treats a zero-size
  request as 1 byte, uses `posix_memalign` for over-aligned requests and `malloc`
  otherwise. It returns null on failure.
- `__baka_deallocate(p)` calls `free`.

These match the IR interpreter's heap behaviour, so both agree on allocation failure.
