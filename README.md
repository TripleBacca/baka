# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1AB9U8lJL6yAngGVG6AMKpaAVxYM9DgDJ4GmADl3ACNMYhAAZgAOUgAHVAVCWwZnNw89eMSbAV9/IJZQ8OiLTCtshiECJmICVPdPLhKy5MrqglzAkLDImIUqmrr0xr62jvzCnoBKC1RXYmR2DgBSDQBBFdW/AgBqYkN0CEntpYB2ACEN7avtkZtkba3t1WOIgBFtjSWIi7XL6%2BelgBWbBfd4AiKOL6ObZcCJfH6ra5PY7A0HIr4giLYGEneF/K4A1FvdEQqHbQF4tZI4iYAhzBhPSnrE6vDZstZ9YiuazbSo01jHc74h6CG54ABemBe7y4n2%2BwuCqBc2wQTAUAEkCJgWApDoKzrtafSxZKALSm%2BGC1lU66PYC0gKYVQEPWnA00unEBl7BgHSaW07W5ms%2BW/NaPFhMPwQR7VYDmbbIVXEABUKcT%2Bzw6CYWoURzd7MR1z5mAFnNLLGl%2BsDTKRiuV/FcvqrVDESiZwoA9J3tgQEHgFA9BwA3MRZ7b8YjbWLVJRTgS0ACewoA7v36BBy6wAHSqjVanWHS3eJWxbxMUK0Lj%2BoU2pFXR6j2hVrcsbf2giO51H0NF%2B8TkgY1FPAqzlA0QLJONkEtPAlgAJguBCbwRf9/zwKgICfaU0T6VA8AgURfSzHNMAUIEzlgwFXkmZDhVQ%2B9G2bNECC5TBa3o1Dgn5ABrbYT1QM8L1KLh2I4q06OuGs73E6T0OoWZfVo6SkW7bZVgZQhtRFXsEClV8h20vspWqPZF2OAA2DRCPwbNcyWSzhSk4MNg4aZaE4QFeE8DgtFIVBOGhBRZnmKV4IiHhSAITRXOmbiQEBDR9E4SQvOivzOF4BQQESqKfNc0g4FgJA0BYWI6DCchKBKsr6HCYImG4pg%2BDoLViCyiBgjS4I/GqRdOAi7rmGIRcAHlgm0TBrH63gSrYQQRoYJc0qwYJXGARwxFoLLuF4LBIyMcQ8tIfAaWsPBh1ItKnUm1wtWm8hBFKNLaDwLjeucLA0pYvAWHui7iEVJRXm1QxgBeoxoumKgDGABQADU8EwFcRtiRh7v4QQRDEdgpBkQRFBUdQjt0RoDAh0xjHMF7giyyBpgE8ptt4VB/uILNLrpppJvKexfUGBpSB8PxOgKbpGkyJIBH5jIEklhgxi6cJhlKbmWn6WoXHqPRLFVgRWhqBXRaVix1el4Z1cNiZrxmOYFgkNyPNSo7/I4bYzgaphtggGc2swU0F0XI4IFwQgSGOODwsmXhcq0GjSDihKko4FLSG83yXcy7LIshh2ODgp304y7O8rj/7EjsSQgA%3D%3D)

### Usage

You can build the compiler by running the following command:
```bash
make build
```
baka does not have a preprocessor and thus does not support preprocessor directives.

#### Command Line Flags:
- `--lex` - run only the lexer and print/output tokens
- `--parse` - run up through parsing and output the AST
- `--codegen` - run the full pipeline and emit generated code
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

**Extra Features**
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

### Stages Status
- [x] Lexer
- [x] Parser
- [ ] Semantic Analysis
- [ ] Optimization Passes
- [ ] Codegen
