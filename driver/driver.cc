#include "driver.hh"
#include "gctx.hh"
#include "args.cc"
#include <iostream>
#include "defs.hh"
#include "lexer/lexer.hh"
#include "../types/token/all.hh"

void baka::driver::run(int argc, char* argv[]) {
    ParseArgs(argc, argv);

    // ValidateSourceFile(); check if file exists and all
    // std::string SourceCode = ReadSourceFile(); // TODO: need to implement this
    // Preprocessor(source); dont plan to do this

    { // lexing
        std::vector<types::Token> tokens;
        if(Gctx::TillStage() >= Stage::LEX) {
            // tokens = lexer::Tokenize(SourceCode);
        } else {
            // spit out all errors
            // TODO: collect all erorrs first
            return;
        }

        if(Gctx::isVerbose()) {
            std::cout << "Lexer Output: " << tokens.size() << " Tokens" << std::endl;
            for(const auto& token : tokens) {
                std::cout << token << std::endl;
            }
        }
    }
}
