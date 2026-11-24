# Compiler wiki

An overview of how baka turns a checked program into a running MIPS64 executable.
Pages 15–19 document the C++ features added on top (access control, references,
lambdas, virtual functions, record equality). These pages cover breadth, not line-by-line detail; read them in order for the full
picture, or jump to a topic.

| # | Page | What it covers |
|---|---|---|
| 1 | [Pipeline overview](01-pipeline-overview.md) | The stages from typed IR to a running binary, and how the driver wires them |
| 2 | [Target and environment](02-target-and-environment.md) | Why MIPS64 n64, little-endian, glibc, static non-PIC, QEMU |
| 3 | [The IR the backend consumes](03-ir-input.md) | What three-address code looks like and what the backend can rely on |
| 4 | [IR optimization](04-ir-optimization.md) | The block-local passes that run before selection, and function inlining |
| 5 | [Instruction selection](05-instruction-selection.md) | Machine IR and how each IR instruction becomes MIPS |
| 6 | [Register allocation](06-register-allocation.md) | Next-use information, descriptors, `getReg`, spills |
| 7 | [Calls, frames and the ABI](07-calls-frames-abi.md) | Calling convention, records by value, stack layout, prologue/epilogue |
| 8 | [Data and globals](08-data-and-globals.md) | `.data`/`.rodata`/`.bss`, initializers, relocations, the literal pool |
| 9 | [Runtime and program startup](09-runtime-and-startup.md) | `__baka_start`, the weak `main`, static destructors, the heap |
| 10 | [Peephole optimization](10-peephole.md) | Machine-level cleanup and delay-slot filling |
| 11 | [Testing and tooling](11-testing-and-tooling.md) | How to run code, and how the test suites check it |
| 12 | [Limitations and known gaps](12-limitations.md) | Design trade-offs and what the edge-case audit found |
| 13 | [Implementation guide](13-implementation-guide.md) | Data structures and algorithms across the whole compiler, plus "how did we implement X?" answers (`static`, `switch`, destructors, overloading, `new`/`delete`, …) |
| 14 | [Function overloading](14-function-overloading.md) | How overloads are declared, looked up and resolved to one declaration, followed from source to assembly |
| 15 | [Access modifiers](15-access-modifiers.md) | `public`/`protected`/`private`, defaults, inheritance access, where access is checked, design and implementation |
| 16 | [References](16-references.md) | `T&` rules, binding and temporaries, how references become pointers in the IR, design and implementation |
| 17 | [Captureless lambdas](17-captureless-lambdas.md) | Lambda syntax, deduced results, hoisting to internal functions, design and implementation |
| 18 | [Virtual functions](18-virtual-functions.md) | Every class method is virtual, pure methods and abstract classes, vtables, vptr handling and dispatch |
| 19 | [Record equality](19-record-equality.md) | `==`/`!=` on structs and classes: field-by-field comparison, the vptr check for classes, generated `.eq` functions |

## One-paragraph summary

After semantic analysis, the program is lowered to a typed three-address IR and
verified. With optimization on, a set of block-local passes simplifies that IR. The
backend then walks each function block by block, choosing MIPS instructions and
assigning physical registers in the same pass, using next-use information and
register/address descriptors in the style of the course lectures. A prologue and
epilogue are added once the frame size is known. A peephole pass cleans up the machine
code and fills branch delay slots. Finally the printer writes one GNU assembler file
containing every function, a small runtime (program entry and heap helpers) and all
global data. That file is assembled and statically linked against glibc with a MIPS
cross-compiler and run under QEMU.
