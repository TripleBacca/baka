# 11. Testing and tooling

## Running your own program

```sh
make build                                   # build ./build/baka
make mips-image                              # one-time: build the Docker toolchain image

./build/baka --ir file.cpp                   # print the IR
./build/baka --ir --optimize file.cpp        # print the optimized IR
./build/baka --codegen file.cpp > file.s     # print optimized MIPS assembly
./build/baka --codegen --no-optimize file.cpp

tools/mips/run file.s 5                      # assemble, link and run with input 5
make run file=file.cpp args='5'             # compile and run in one step
make run-mips-binary file=prog.mips args='5 "two words"'
```

`tools/mips/run` copies the `.s` into a temporary directory, runs the cross-compiler and
QEMU inside the Docker image, and exits with the program's exit status. `-c harness.c`
links the test harness instead of the generated `main`.

## Test suites

| Suite | What it checks | How |
|---|---|---|
| Lexer and parser | Representative syntax and diagnostics | Compare stdout snapshots, check exit status and stderr, and time out a stuck compiler |
| IR | IR text for each fixture, and execution results | Compares `--ir` output with saved `.expected` files; runs each fixture in the IR interpreter, unoptimized and optimized |
| IR optimizer | Each optimization pass, including cases where it must not fire | Compares `--ir --optimize` output with saved `.expected` files |
| Peephole | Each machine-level rule and its negative cases | Unit test on hand-written machine code |
| Codegen | Programs compile, link and produce the right results on MIPS | Runs IR, IR optimizer, and codegen fixtures under QEMU |
| Semantic | Front-end acceptance and rejection | Compares diagnostics with saved output |

```sh
make test-ir
make test-ir-opt
make test-codegen                            # needs Docker
ctest --test-dir build                       # all registered suites
```

## The IR interpreter

A small interpreter in the test directory runs IR directly, with no MIPS involved. It
models memory byte by byte, implements the heap helpers and `atoi`, and runs static
destructors after `main`. It calls `main` as if the program were run as `prog <n>`
(`argc = 2`, `argv = {"prog", "<n>", null}`). It is the reference for what a program
*should* do.

The two backends are never compared with each other directly. Both are checked against
the same expected values in `custom_tests/ir/cases.json`. Codegen-only fixtures are not
run in the interpreter, and the interpreter can't check stdout because it can't call
`printf`.

The shared IR manifest includes execution fixtures for access specifiers
(`26_access_modifiers.cpp`), lvalue references (`27_references.cpp`), and
captureless lambdas (`28_lambdas.cpp`). Both runners execute them with and without
optimization.

## How the codegen suite works

- **Cases:** each fixture lists runs as `[input, expected result]`; the input is passed
  as `argv[1]`, and the program reads it with `atoi(argv[1])`. Cases may also list
  expected stdout, text to feed on stdin, and a global whose value is checked after
  destructors run. It reuses every IR fixture plus codegen-specific ones covering libc
  I/O, all integer widths, floats, records, many arguments, recursion, register pressure,
  big frames and conversions.
- **Build:** every fixture is compiled twice, optimized and unoptimized, and run under
  QEMU in one Docker invocation. The harness reports the full return value. Standalone
  startup fixtures use the generated entry point and check the process exit status.
  The heap fixture also runs with the first allocation forced to fail.
- **Expected values** for codegen fixtures were produced by compiling equivalent C++
  natively with wrapping arithmetic (`-fwrapv`) and no fused multiply-add.

## Regenerating expected output

`./custom_test_hook bless <suite>` rewrites the `.expected` files from the current
compiler output. Review the diff before committing.
