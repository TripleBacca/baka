#include "driver.hh"
#include "gctx.hh"
#include "args.cc"
#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <string_view>
#include <vector>
#include "heuristics.hh"
#include "lexer/lexer.hh"
#include "base/base.hh"
#include "parser/parser.hh"
#include "semantic/semantic_analyzer.hh"
#include "ir/lowering.hh"
#include "types/driver/defs.hh"
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

    size_t PrintErrorsIfAny() {
        auto lg = baka::driver::Gctx::GetROLock();
        auto& gctx = baka::driver::Gctx::GetGctxRO();

        auto& LineErrors = gctx.CompilerLineErrors;
        auto& LineWarnings = gctx.CompilerLineWarnings;
        auto& GlobalErrors = gctx.CompilerGlobalErrors;
        auto& GlobalWarnings = gctx.CompilerGlobalWarnings;

        for (auto& i : GlobalWarnings) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;
        for (auto& i : LineWarnings) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;

        for (auto& i : GlobalErrors) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;
        for (auto& i : LineErrors) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;

        return GlobalErrors.size() + LineErrors.size();
    }

}


int baka::driver::run(int argc, char* argv[]) {
    ParseArgs(argc, argv);

    ValidateSourceFile(); // check if exists and has perms

    bool isVerbose = Gctx::isVerbose();

    std::string_view SourceCode;
    {
        // read source code
        SourceCode = ReadSourceFile();

        auto OptimisticDefects = heuristics::OptimisticCountDefects(SourceCode);
        if(isVerbose) {
            std::cout << "Driver::OptimisticDefects: " << OptimisticDefects << std::endl;
        }
        Gctx::ReserveDefectsVec(OptimisticDefects);
        if(isVerbose) {
            std::cout << "Driver::SourceCode: " << '\n' << SourceCode << std::endl;
        }
    }

    // Preprocessor(SourceCode); dont plan to do this

    lexer::Lexed Lexed;

    {
        // lexing
        if (Gctx::TillStage() >= Stage::LEX) {
            Lexed = lexer::Tokenize(SourceCode);
        }
        else {
            return 0;
        }
        if (isVerbose) {
            std::cout << "Lexer::LexerOutput: " << Lexed.Tokens.size() << " Tokens" << '\n';
        }
        if (Gctx::TillStage() != Stage::IR || isVerbose) {
            PrintTokenTableHeader(std::cout);
            for (const auto& token : Lexed.Tokens) {
                std::cout << token << '\n';
            }
        }
        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    {
        // post lexer
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            // need to stop here
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Lexer: Lexing stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    types::ASTNode* Root = nullptr;

    {
        // parsing
        if (Gctx::TillStage() < Stage::PARSE) {
            return 0;
        }


        parser::Parser Parser(Lexed.Tokens, Lexed.Locations);
        Root = Parser.Parse();

        if(isVerbose) {
            std::cout << "Parser::AST: " << '\n';
            Root->Print();
            std::cout << std::endl;
        }
    }

    {
        // post parser
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            // need to stop here
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Parser: Parsing stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    {
        // semantic checking
        if (Gctx::TillStage() < Stage::SEMANTIC) {
            return 0;
        }

        auto* Program = dynamic_cast<types::ProgramNode*>(Root);
        if (!Program) {
            Gctx::GenerateGlobalError("semantic stage requires a program AST", Stage::SEMANTIC);
        } else {
            semantic::SemanticAnalyzer Analyzer;
            bool Analyzed = Analyzer.analyze(*Program);
            if (Analyzed && (Gctx::TillStage() != Stage::IR || isVerbose)) {
                std::cout << "Semantic::AST: " << '\n';
                Program->Print();
                std::cout << std::endl;
            }
            if (!Analyzed && Analyzer.diagnostics().empty()) {
                Gctx::GenerateGlobalError("semantic analysis failed", Stage::SEMANTIC);
            }

            // Lower while both the annotated tree and its semantic arenas are
            // alive. IR owns its copied types, identities and strings afterward.
            if (Analyzed && Gctx::TillStage() == Stage::IR) {
                ir::Lowerer Lowerer(*Analyzer.checkedProgram());
                if (Lowerer.lower()) {
                    ir::Print(Lowerer.module(), std::cout);
                } else {
                    for (const auto& Entry : Lowerer.diagnostics()) {
                        size_t LineNo = Entry.source.line ? Entry.source.line : 1;
                        size_t ColNo = Entry.source.column ? Entry.source.column : 1;
                        base::LineCtx Context{LineNo - 1, Gctx::GetLineIndex().get()};
                        Gctx::GenerateLineError(LineNo, ColNo, Context, Entry.message, Stage::IR);
                    }
                }
            }

            for (const auto& Entry : Analyzer.diagnostics()) {
                auto* Source = reinterpret_cast<const types::ASTNode*>(Entry.source);
                auto Span = Source ? Source->getSourceSpan() : types::ASTSourceSpan{};
                size_t LineNo = Span.Valid ? Span.Line : 1;
                size_t ColNo = Span.Valid ? Span.Col : 1;
                base::LineCtx Context{LineNo - 1, Gctx::GetLineIndex().get()};
                Gctx::GenerateLineError(LineNo, ColNo, Context, Entry.message, Stage::SEMANTIC);
                auto* Previous = reinterpret_cast<const types::ASTNode*>(Entry.previous);
                if (Previous) {
                    auto PreviousSpan = Previous->getSourceSpan();
                    if (PreviousSpan.Valid) {
                        base::LineCtx PreviousContext{PreviousSpan.Line - 1, Gctx::GetLineIndex().get()};
                        Gctx::GenerateLineWarning(PreviousSpan.Line, PreviousSpan.Col, PreviousContext,
                                                  "previous declaration is here", Stage::SEMANTIC);
                    }
                }
            }
        }
    }

    {
        // post semantic
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Semantic: Checking stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    return 0;
}
