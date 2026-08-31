# baka
baka is a compiler for a C++ like language.

### Usage

You can build the compiler by running the following command:
```bash
make build
```
baka does not have a preprocessor and thus does not support preprocessor directives.

##### Command Line Flags:
- `--lex` - run only the lexer and print/output tokens
- `--parse` - run up through parsing and output the AST
- `--codegen` - run the full pipeline and emit generated code
- `--build_type` - set the build type (e.g. `debug` / `release`)
- `--verbose` - print detailed logs during compilation


To use the compiler, run it with the desired flags and provide the source code file as an argument.
```bash
./baka <options> <source_code_file>
```


##### Lexing

 To run the lexer you can use the `--lex` flag to stop the compiler after the lexing phase.
 To see more verbose logs about the various tokens identified, you can use the `--verbose` flag.

 ```bash
 ./baka --verbose --lex --build_type release <source_code_file>
 ```


##### Parsing

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

**Extra Features**
- Loop Labels
Loop labels allow the user to break or continue out until a specific loop.
This allows the user to annotate loops and write cleaner nested loops.

e.g.
without labels
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
        break;//extra break needed to break out of outer loop
    }
}
if(found){
//An item in the stream is in the array `candidates`
}
```
with labels
```
bool found = false;
while(stream.hasItems();loopy){
    int val = stream.getNext();
    for(int i = 0; i < len; i++){
        if(val == candidates[i]){
            found = true;
            break loopy;
        }
    }
}
if(found){
//An item in the stream is in the array `candidates`
}
```

### Stages Status
- [x] Lexer
- [x] Parser
- [ ] Semantic Analysis
- [ ] Optimization Passes
- [ ] Codegen
