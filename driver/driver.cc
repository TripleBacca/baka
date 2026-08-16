#include "driver.hh"
#include "gctx.hh"
#include "args.cc"
#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <string_view>
#include <vector>
#include "lexer/lexer.hh"
#include "base/base.hh"
#include "types/driver/defs.hh"
#include "types/token/all.hh"
#include "line_index.hh"
#include "mmap_file.hh"
#include "types/exceptions.hh"


namespace {
    void ValidateSourceFile() {
        std::filesystem::path path = baka::driver::Gctx::GetSourceFilePath();

        if (!baka::base::ValidateFile(path)) {
            throw baka::exceptions::DriverError("source file does not exist: " + path.string());
        }
    }


    std::string_view ReadSourceFile() {
        std::filesystem::path path = baka::driver::Gctx::GetSourceFilePath();

        std::shared_ptr<baka::base::MappedFile> MappedFilePtr = std::make_shared<baka::base::MappedFile>(path);
        baka::driver::Gctx::AttachMappedFile(MappedFilePtr);

        std::shared_ptr<baka::base::LineIndex> LineIndexPtr = std::make_shared<baka::base::LineIndex>(
            MappedFilePtr->View());
        baka::driver::Gctx::AttachLineIndex(LineIndexPtr);

        return MappedFilePtr->View();
    }
}


int baka::driver::run(int argc, char* argv[]) {
    ParseArgs(argc, argv);

    ValidateSourceFile(); // check if exists and has perms

    std::string_view SourceCode;
    {
        // read source code
        SourceCode = ReadSourceFile();
        if (Gctx::isVerbose()) {
            std::cout << "Driver::SourceCode: " << '\n' << SourceCode << std::endl;
        }
    }

    // Preprocessor(SourceCode); dont plan to do this

    std::vector<types::Token> tokens;

    {
        // lexing
        if (Gctx::TillStage() >= Stage::LEX) {
            tokens = lexer::Tokenize(SourceCode);
        }
        else {
            return 0;
        }
        if (Gctx::isVerbose()) {
            std::cout << "Lexer::LexerOutput: " << tokens.size() << " Tokens" << '\n';
            for (const auto& token : tokens) {
                std::cout << token << '\n';
            }
            std::cout << std::endl;
        }
    }

    {
        // post lexer
        auto lg = Gctx::GetROLock();
        auto& gctx = Gctx::GetGctxRO();

        auto& Errors = gctx.CompilerErrors;
        auto& Warnings = gctx.CompilerWarnings;

        for (auto& i : Warnings) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;

        for (auto& i : Errors) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;


        if (!Errors.empty()) {
            // need to stop here
            size_t ErrorCount = Errors.size();
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (gctx.VerboseMode) {
                std::cout << "Lexer: Lexing stage completed" << '\n';
            }
        }

        std::cout << std::endl;
    }


    return 0;
}
