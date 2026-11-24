# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/z/vcTGT3)

### Usage

You can build the compiler by running the following command:
```bash
make build
```
baka does not have a preprocessor and thus does not support preprocessor directives.

Running `./build/baka path/to/file.cpp` builds a MIPS64 executable named `a.out`. Build the Docker toolchain with `make mips-runtime` first. Use a stage flag to stop earlier.

#### Make Recipes
- `make parse file=<source_code_file_path>` : Runs the compiler in verbose mode upto the parser stage.
    For instance, to run the parser on `test.c` and stop after the parser stage, you can use:
    ```bash
    make parse file=test.c
    ```

- `make lex file=<source_code_file_path>` : Runs the compiler in verbose mode upto the lexer stage.
- `make semantic file=<source_code_file_path>` : Runs through semantic analysis for the supported language subset.
- `make test-semantic` : Runs representative semantic acceptance/rejection programs.
- `make test-ir` : Checks TAC output and execution before and after optimization.
- `make parse-folder dir=<directory>` : Runs the parser on all files (excluding `.out` files) in the given directory recursively. Each file's output is written to an `.out` file with the same name and location (e.g. `test/foo.cpp` -> `test/foo.cpp.out`). Reports a summary of files parsed and errors at the end.
- `make test` : Run custom compiler tests, including native MIPS execution. Run `make mips-runtime` first.
- `make build` : Build the binary and put it in `/build/baka`
- `make clean` : Clean up the build artifacts

#### Command Line Flags:
- `--lex` - run only the lexer and print/output tokens
- `--parse` - run up through parsing and output the AST
- `--semantic` - run declaration registration, resolution/renaming, type annotation, and read-only semantic checks
- `--ir` - print the typed three-address code (unoptimized unless `--optimize`)
- `--codegen` - print MIPS64 (n64, little-endian) assembly (optimized unless `--no-optimize`)
- `--assemble` - write a relocatable MIPS64 object from Baka source or a `.s` file. Defaults to the input's base name with a `.o` extension.
- `--link` - write a static MIPS64 executable from Baka source, a `.s` file, or a `.o` file. This is the default stage.
- `-o path` - set the object or executable output path.
- `--optimize` / `--no-optimize` - force the block-local IR and MIPS peephole optimizations on or off
- `--build_type` - set the build type (e.g. `debug` / `release`)
- `--verbose` - print detailed logs during compilation


To use the compiler, run it with the desired flags and provide the source code file as an argument.
```bash
./baka <options> <source_code_file>
```


#### Lexing

 To run the lexer you can use the `--lex` flag to stop the compiler after the lexing phase.
 To see more verbose logs about the various tokens identified, you can use the `--verbose` flag.

 ```bash
 ./baka --verbose --lex --build_type release <source_code_file>
 ```


#### Parsing

 To run the parser you can use the `--parse` flag to stop the compiler after the parsing phase.
 To see more verbose logs about the AST created, you can use the `--verbose` flag.

 ```bash
 ./baka --verbose --parse --build_type release <source_code_file>
 ```


### Featureset
We are hoping we can support the following featureset:

**Basic Features**
- All arithmetic and logical operators
- if-else
- for loop
- while loop
- do-while loop
- switch cases
- arrays (integer and char)
- pointers
- structures
- printf and scanf
- function calls with arguments
- goto, break and continue
- static keyword

**Advanced Features**
- Recursive function calls
- Classes and objects
- Access modifiers
- Lambdas
- References
- Inheritance
- Function calls with variable arguments
- Function pointers
- Command line input
- typedef
- enum, union
- File manipulation
- Multi-level pointers
- Multi-dimensional arrays
- Access specifiers, lvalue references, and captureless lambdas

**Extra Non C++ Features**
- Quad Equals
- Loop Labels:
    Allows programmers to break or continue out until a specific loop. This feature lets you annotate loops with labels and write cleaner and more readable nested loop control flow.

#### A scenario where this makes code cleaner:

##### Without Loop Annotation Labels:
```
bool found = false;
while(stream.hasItems()){
    int val = stream.getNext();
    for(int i = 0; i < len; i++){
        if(val == candidates[i]){
            found = true;
            break;
        }
    }
    if(found){
        // extra break needed to break out of outer loop
        break;
    }
}
if(found){
    // An item in the stream is in the array `candidates`
}
```


##### With Loop Annotation Labels:
```
bool found = false;
while(stream.hasItems(); LoopLabel1){
    int val = stream.getNext();
    for(int i = 0; i < len; i++){
        if(val == candidates[i]){
            found = true;
            break LoopLabel1;
        }
    }
}
if(found){
    // An item in the stream is in the array `candidates`
}
```

### Assumptions
There are certain features we are not supporting:
- Unnamed Parameters in Function Signature
- Preprocessor directives
- `signed`
- `const` pointer
- Multiple inheritance
- `namespace` and scope resolution(`::`)
- `enum class`

**Optimisations**
- **Semantic analysis**
-     Constant expression folding

- **IR lowering**
-     Devirtualization
-     `====` / `!===` compile-time folding
-     Record `==` class-check folding
-     On-demand equality functions
-     Trivial destructor skipping

- **IR optimizer**
-     Unreachable-block removal
-     Loop unrolling
-     Constant folding
-     Copy and constant propagation
-     Algebraic identity simplification
-     Strength reduction
-     Value numbering (common subexpression elimination)
-     Redundant-load removal and store-to-load forwarding
-     Dead-store elimination
-     Constant branch folding
-     Dead-code elimination
-     Tail-recursion elimination
-     Function inlining

- **Instruction selection and register allocation**
-     Keeping scalars in registers
-     `getReg` register allocation
-     Callee-saved register preference
-     Liveness-based spill avoidance
-     Fused compare-and-branch
-     Zero-register reuse
-     Unrolled record copies and zero-fill

- **Peephole optimiser**
-     Redundant reload removal
-     Dead stack-store removal
-     Result retargeting
-     Dead-instruction removal
-     No-op removal
-     Copy propagation
-     Address folding
-     Immediate-form selection
-     Multiply-to-shift reduction
-     Control-flow cleanup
-     Delay-slot filling

### Stages Status
- [x] Lexer
- [x] Parser
- [x] Semantic Analysis for the supported subset ([passes and policies](src/semantic/README.md), [remaining TODOs](src/semantic/TODO.md))
- [x] Optimization Passes (block-local TAC passes and MIPS peephole; [IR_OPTIMIZATION_PLAN.md](IR_OPTIMIZATION_PLAN.md))
- [x] Codegen: MIPS64 n64


#### Made By:
- Satindra Ramalingam (24114087)
- Shaurya Goel (24114090)
- Tanmay Arya (24114099)
- Abhinav Shresth (24115008)


## Semantic analysis and TAC

After building, use `./build/baka --semantic source.cpp` to inspect the normalized
AST and `./build/baka --ir source.cpp` to print checked typed TAC. Run
`ctest --test-dir build --output-on-failure` for both stage suites, or
`make test-semantic` / `make test-ir` to run either suite separately.

The current architecture and language policies are documented in
[src/semantic/README.md](src/semantic/README.md)

## MIPS64 code generation

`./build/baka --codegen file.cpp > file.s` emits GNU assembly for MIPS64r2 (n64 ABI,
little-endian), linked statically against glibc. The cross toolchain and `qemu-mips64el`
live in a Docker image, so nothing is installed on the host:

```bash
make mips-runtime                     # docker build -t baka-mips64el tools/mips
./build/baka -o prog.mips prog.cpp    # compile, assemble and link
make assemble file=prog.cpp out=prog.o
make link file=prog.o out=prog.mips
make run file=prog.cpp args='5 "two words"'  # compile and run from source
make test-codegen                     # every fixture, optimized and unoptimized
```

`make assemble` accepts source or assembly. `make link` also accepts an existing
object file. Omit `out` to use the compiler defaults: the input's base name with
`.o` for assembly, and `a.out` for linking.

`make run` forwards standard streams. Use shell quoting inside `args` to pass an
argument with spaces, or use `n=5` as a shortcut for one argument. Make reports a
nonzero program status as a recipe error and exits with status `2`.

To run an existing MIPS executable, create a standalone launcher:

```bash
make shim out=run-prog
BAKA_BINARY=./prog.mips ./run-prog 5 "two words"
```

Without `out`, `make shim` creates `./run-baka-mips`. The launcher reads
`BAKA_BINARY` each time it runs and uses the `baka-mips64el` Docker image.
It forwards program arguments, standard streams, and exit status.
The exit status is the low 8 bits of `main`'s result on normal exit and
`128 + host signal number` for an uncaught signal. The launcher reports a missing
binary with status `2` and an invalid executable format with status `1`.
See the [codegen design notes](src/codegen/README.md) for details.

Run `ctest --test-dir build -R '^mips_runtime$' --output-on-failure` to verify
runtime behavior against the Docker image.
