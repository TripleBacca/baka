#include "driver.hh"
#include "gctx.hh"
#include "args.cc"
#include <filesystem>
#include <iostream>
#include <string_view>
#include "defs.hh"
#include "lexer/lexer.hh"
#include "base/base.hh"
#include "../types/token/all.hh"
#include "types/exceptions.hh"


namespace {
    void ValidateSourceFile() {
        std::filesystem::path path = baka::driver::Gctx::GetSourceFilePath();

        if(!baka::base::ValidateFile(path)) {
            throw baka::exceptions::DriverError("source file does not exist: " + path.string());
        }
    }


    std::string_view ReadSourceFile() {

    }
}



void baka::driver::run(int argc, char* argv[]) {
    ParseArgs(argc, argv);

    ValidateSourceFile(); // check if exists and has perms
    std::string_view SourceCode = ReadSourceFile(); // TODO: need to implement this
    // Preprocessor(SourceCode); dont plan to do this

    { // lexing
        std::vector<types::Token> tokens;
        if(Gctx::TillStage() >= Stage::LEX) {
            tokens = lexer::Tokenize(SourceCode);
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
