# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1AB9U8lJL6yAngGVG6AMKpaAVxYMQAJlIOAMngMmABy7gBGmMQgkhqkAA6oCoS2DM5uHt7xickCAUGhLBFRMRaYVjYCQgRMxARp7p4%2BlpjWKVU1BHkh4ZHRsQrVtfUZTYOdgd2FvTEAlBaorsTI7ByBBADUTOjoEGubpOt7YTPrAKQA7ABCpxoAgusP68SYBIsMm2del%2BthpwDM1zuFwAIjdbmC9gpXGFdoJ9oc4cczlcwY8ni83h8ALQ/f6A8HnUFAu4AN1QeHQ61ccXQTAImFhBAAVOtBsAXicLvi0Uy2S8zn9geteTV2RtTl91lw8WCQbK7nsmHE4rQAJ7GMKBGqqxnrVQHPaqg1wgBenJRdzRz1exHeWx29og%2BvWqrm6yhMOdZpmMqBhPltz2VCY1hIeDEuoY5u5jzwVHWEHe/0c/yFXGjqLRj2tmOlAMzDzlluzGNt63eLODoeI4doifWOPTvoJRPBCrh/FoOz2JLEClOAFZLgPB8CM8WHjmy73aP2hxpR59vjO55dJKPm0W24G4SwmIEIOP7rG4VCWAKhY6uAcvD78xOERs0IVUBfNsq1RqtcQdWeDlxYnWP47xjB4gxDcVBXWKsiBrCMBxAgM0T2JMoMkPNQKpGk6QZCUADYo03B9FWIYhB2HRdU2RS5r3WHwgIOaR1hHf172PMCOxcLBKSozsHVIxDiXY9EbXeM8l3WZ8wlfCVvhgiSk0lPjMHQTdCQ4OZaE4AdeE8DgtFIVBOEcd0FiWTBPj%2BHhSAITQNLmABrEAB1iLSOEkXS7MMzheAUEBYls/SNNIOBYCQZ84joSJyEoCKoqiMImAcpg%2BDoeliD8iAwi8zVmB/ThrNy7UAHkwm0FpAus582EEYqGDVLysDCVxgEcMRZwK3gsD3IxxCC0h8GeVoSUwPz%2BswVQWlcelOvIQQyi82g8DCYhtWcLAvIIGsWFmkbiGkpRgUwHrgCWow7LmKgDGABQADU8EwAB3Yq4kYWb%2BEEEQxHYKQZEERQVHUfrdGvAxztMYxzCWsI/MgOZUDiCoGDG3hUD2mssFhw9SnKFJ7AYJwXAaPR/AmAoij0BIkiR4ZPGvKmcgYLpyd6a9mlaSoxlpvR2aR9pamZnoojZrmiYyEWOkFqZhbmBQzOWCRNO0zz%2BqMjh1kuJKmATOIaiULEBDVE4IFwQgSEs9NeECrQZkc5zXM4DzSD0gy1d8/ybIupWOC8FXXZ8z2gtt0g9qSOxJCAA)

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
