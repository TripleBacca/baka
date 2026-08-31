# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/z/vcTGT3)

### Usage

You can build the compiler by running the following command:
```bash
make build
```
baka does not have a preprocessor and thus does not support preprocessor directives.

#### Make Recipes
- `make parse file=<source_code_file_path>` : Runs the compiler in verbose mode upto the parser stage.
    For instance, to run the parser on `test.c` and stop after the parser stage, you can use:
    ```bash
    make parse file=test.c
    ```

- `make lex file=<source_code_file_path>` : Runs the compiler in verbose mode upto the lexer stage.
- `make parse-folder dir=<directory>` : Runs the parser on all files (excluding `.out` files) in the given directory recursively. Each file's output is written to an `.out` file with the same name and location (e.g. `test/foo.cpp` -> `test/foo.cpp.out`). Reports a summary of files parsed and errors at the end.
- `make test` : Run tests in the `/test` directory and compare the verbose output to the `.expected` output files.
- `make build` : Build the binary and put it in `/build/baka`
- `make clean` : Clean up the build artifacts

#### Command Line Flags:
- `--lex` - run only the lexer and print/output tokens
- `--parse` - run up through parsing and output the AST
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
- Inheritance
- Function calls with variable arguments
- Function pointers
- Command line input
- typedef
- enum, union
- File manipulation
- Multi-level pointers
- Multi-dimensional arrays

**Extra Non C++ Features**
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
- Access Modifiers: Everything is `public` by default.
- `namespace` and scope resolution(`::`)
- `enum class`

### Stages Status
- [x] Lexer
- [x] Parser
- [ ] Semantic Analysis
- [ ] Optimization Passes
- [ ] Codegen


#### Made By:
- Satindra Ramalingam (24114087)
- Shaurya Goel (24114090)
- Tanmay Arya (24114099)
- Abhinav Shresth (24115008)
