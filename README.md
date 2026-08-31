# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/#z:OYLghAFBqd5QCxAYwPYBMCmBRdBLAF1QCcAaPECAMzwBtMA7AQwFtMQByARg9KtQYEAysib0QXACx8BBAKoBnTAAUAHpwAMvAFYTStJg1AB9U8lJL6yAngGVG6AMKpaAVxYM9DgDJ4GmADl3ACNMYhAAVlIAB1QFQlsGZzcPPVj4mwFffyCWUPCoy0xrRKECJmICZPdPLgtMK0yGMoqCbMCQsJBpBXLK6tS63tb23PzugEoLVFdiZHYOAFINAEFllb8CAGpiQ3QICa3FgHYAIXWty63hm2Qtza3VI4BmABEtiMXn89WLq6fFhFsF93gDno4vo4tlxnl8fisro8jkCQUivsDnthocc4X9LgCUW80eDIR9catEcRMARZgxHuS1sdXusWatesRXNYtmUqawjmc8fdBNc8AAvTAvd5cDQMxHBVAuLYIJgKACSBEwLAUB35px21NpIvFAFpjXD%2BcyKVcHsBqQFMKoCDqTnqqTTiHTdgx9hNzSdLYzmd9WRthSwmH4IA8KsBzFtkMriAAqJPxvZ4dBMDUKQ4ukOInmYPnsossSW6/2yq7yxX8Vze8tUMRKBmCgDuCDomAgJdYADplWqNVqDubvArot4mKFaFxfQKrYjLg8AG5icu9lh920Ee2O0fBxdL/jEKPCvDlmXfe4vKEx5DmvCLABM51f8/hS6/9yoEDXtElVFelQPAIFEb0MyzTAFEBU4nwiV4Jg/QVv2PGYG1RAgOUwKtUK/YJeQAay2cdUEnacGi4XC8MrI9EVohErgYxE8F/OtvWQuirgAem4rYVjpQhNSFLYCAQCVN3uBQRLEiUKl2ABPI4ADYNHA/BM2zRZVMFBiGI4KZaE4CJeE8DgtFIVBOChBQZjmCUX2eHhSAITQDKmQjIg0fROEkUy3MszheAUEBvNc8yDNIOBYCQNAWGiLsyAoMDUHixKQGCJhCKYPg6A1YgQogYIAuCPwKgUzhnNK5hiAUgB5YJtGKcLnLithBDqhhaAqiLSCwYJXGARwxFoELuF4LBwyMcRevwKkShXaCAodYpXA1SreE2BoAtoPACPK5wsACrC8BYDbSEW4h5SUV5NUMYBdqMNypioAxgAUAA1PBMDbOrokYc7%2BEEEQxHYKQZEERQVHUXrdDqAwntMYxzF24IQsgKYyKaMbeFQS7iAzJaMfqRpEnsb0BlqUgfD8Do8i6Op0gSARKbSOJmYYUZOnCIYGma0o%2BiqFwaj0IoSgEFpKi5%2BmeYsQXWaGQXpfGOdplmeYJEM4z/N6qyOC2U4sqYLYIGiColGNARusOCBcEIEgjmfJyJl4cKtCQ0hPIibyjI4PzSDMiy9eC0KXOerWOGfHWg6CsOIo9y74jsSQgA%3D)


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
