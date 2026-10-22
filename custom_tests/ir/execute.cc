#include "driver/gctx.hh"
#include "interpreter.hh"
#include "ir/lowering.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace baka;

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    ir::Module Compile(const std::string& path) {
        std::ifstream file(path);
        Require(bool(file), "missing IR fixture");
        std::string source((std::istreambuf_iterator<char>(file)), {});
        driver::Gctx::AttachLineIndex(std::make_shared<base::LineIndex>(source));
        auto tokens = lexer::Tokenize(source);
        parser::Parser parser(tokens.Tokens, tokens.Locations);
        auto* program = dynamic_cast<types::ProgramNode*>(parser.Parse());
        Require(program && !driver::Gctx::ErrorFound(), "fixture failed parsing");
        semantic::SemanticAnalyzer analyzer;
        if (!analyzer.analyze(*program)) {
            for (const auto& diagnostic : analyzer.diagnostics()) {
                std::cerr << diagnostic.message << '\n';
            }
            throw std::runtime_error("fixture failed semantic analysis");
        }
        ir::Lowerer lowerer(*analyzer.checkedProgram());
        Require(lowerer.lower(), "fixture failed IR lowering");
        return lowerer.takeModule();
    }

    void CheckVerifier() {
        ir::Module module;
        ir::Type integer;
        integer.kind = ir::TypeKind::Integer;
        integer.bits = 32;
        integer.isSigned = true;
        integer.size = integer.alignment = 4;
        module.types.push_back(integer);
        ir::Function function;
        function.id = 1;
        function.name = "main";
        function.signature = module.function(1, {}, false);
        function.defined = true;
        function.blocks.push_back({1, {{ir::Opcode::Return, {}, {ir::OperandKind::Integer, 1, 0, 0}}}});
        module.functions.push_back(std::move(function));
        Require(ir::Verify(module).empty(), "valid module rejected");

        auto bad = module;
        bad.functions[0].blocks[0].instructions.clear();
        Require(!ir::Verify(bad).empty(), "missing terminator accepted");
        bad = module;
        bad.functions[0].temporaries.push_back(1);
        bad.functions[0].blocks[0].instructions[0].left = {ir::OperandKind::Temporary, 1, 1};
        Require(!ir::Verify(bad).empty(), "undefined temporary accepted");
    }
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "verifier") {
            CheckVerifier();
            return 0;
        }
        Require(argc == 4 || argc == 6, "usage: ir_execution_tests source input result [global result]");
        auto module = Compile(argv[1]);
        const int input = std::stoi(argv[2]);
        const int64_t expected = std::stoll(argv[3]);
        for (bool optimize : {false, true}) {
            if (optimize) {
                ir::Optimize(module);
                Require(ir::Verify(module).empty(), "optimized IR failed verification");
            }
            ir_test::Interpreter executor(module);
            const auto actual = executor.run(input);
            if (actual != expected) {
                std::cerr << "input " << input << ": expected " << expected << ", got " << actual
                          << (optimize ? " (optimized)" : "") << '\n';
                return 1;
            }
            if (argc == 6) {
                Require(executor.globalInteger(argv[4]) == std::stoll(argv[5]),
                        "global value after static destruction is incorrect");
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
