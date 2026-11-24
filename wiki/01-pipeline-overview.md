# 1. Pipeline overview

## The whole compiler, end to end

```
source.cpp
  │  lexer → parser → semantic analysis          (front end)
  ▼
typed IR (three-address code)  ── verifier
  │  IR optimization                             (optional, on by default for --codegen)
  ▼
optimized IR               ── verifier again
  │  instruction selection + register allocation  (one pass per function)
  ▼
machine IR (one record per assembly line)
  │  peephole optimization + delay-slot filling   (optional)
  ▼
printer → assembly: functions, runtime, data
  │  mips64el-linux-gnuabi64-gcc -c                (--assemble)
  ▼
relocatable object (.o)
  │  mips64el-linux-gnuabi64-gcc -static -no-pie   (--link, the default)
  ▼
static executable → qemu-mips64el
```

Everything from "typed IR" down is the codegen phase.

## Stage responsibilities

| Stage | Input | Output | Key idea |
|---|---|---|---|
| Lowering | checked AST | IR module | Every expression becomes simple instructions on temporaries and memory objects |
| Verification | IR module | diagnostics | Rejects malformed IR before anything consumes it |
| IR optimization | IR | smaller IR | Each rewrite stays inside one basic block |
| Selection + allocation | one IR function | machine function | Picks instructions and registers together, block by block |
| Peephole | machine function | shorter machine function | Local cleanups, then fill delay slots |
| Printing | machine functions + module | assembly text | Functions, then runtime, then data |

## How the driver wires it

- `--ir` stops after lowering and prints the IR. `--ir --optimize` prints the optimized IR.
- `--codegen` stops at assembly text. `--assemble` and `--link` use the MIPS Docker
  toolchain to write a file, with its path set by `-o`. Optimization defaults to **on**
  at these stages. `--no-optimize` turns off both the IR passes and the peephole.
- The register allocator is not optional: even unoptimized code uses it.

## All-or-nothing output

Every function is selected before anything is printed. If the backend meets IR it cannot
represent, it reports a diagnostic and prints nothing, so a half-written `.s` file is
never produced.

## Where the code lives

- `src/ir/`: lowering, verifier, IR optimizer, CFG and liveness analysis
- `src/codegen/`: machine IR, selection/allocation, peephole, printer, runtime, data
- `tools/mips/`: Docker toolchain and the `run` script
- `custom_tests/`: fixtures for the IR, optimizer, peephole and execution suites
