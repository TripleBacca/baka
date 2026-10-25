#include "driver/gctx.hh"
#include "interpreter.hh"
#include "ir/lowering.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace baka;

extern "C" void* __baka_allocate(uint64_t bytes, uint64_t alignment) noexcept;
extern "C" void __baka_deallocate(void* pointer) noexcept;

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

    void CheckRecordDefinitions() {
        ir::Module module;
        ir::Type integer;
        integer.kind = ir::TypeKind::Integer;
        integer.bits = 32;
        integer.isSigned = true;
        integer.size = integer.alignment = 4;
        module.types.push_back(integer);
        ir::Type node;
        node.kind = ir::TypeKind::Record;
        node.name = "Node";
        node.complete = true;
        node.size = 16;
        node.alignment = 8;
        node.memberOffsets = {0, 8};
        module.types.push_back(node);
        const auto pointer = module.pointer(2);
        module.types[2].memberTypes = {1, pointer};
        Require(ir::Verify(module).empty(), "self-referential pointer field rejected");
        std::ostringstream output;
        ir::Print(module, output);
        Require(output.str().find("Node = record(i32, ptr<Node>) size 16 align 8") != std::string::npos,
                "named record definition omits its fields");
        Require(output.str().find("record#") == std::string::npos, "numeric record reference still printed");

        auto bad = module;
        bad.types[2].memberTypes.pop_back();
        Require(!ir::Verify(bad).empty(), "missing field type accepted");
        bad = module;
        bad.types[2].memberTypes[1] = 9999;
        Require(!ir::Verify(bad).empty(), "invalid field type ID accepted");
        bad = module;
        bad.types[2].memberTypes[1] = 2;
        bad.types[2].memberOffsets[1] = 0;
        Require(!ir::Verify(bad).empty(), "recursive by-value field accepted");
        bad = module;
        bad.types[2].memberOffsets[1] = 12;
        Require(!ir::Verify(bad).empty(), "field exceeding record size accepted");
        bad = module;
        bad.types[2].base = 2;
        Require(!ir::Verify(bad).empty(), "base type without an offset accepted");

        // Distinct nominal records with the same source name need distinct text names.
        auto duplicate = module;
        duplicate.types.push_back(module.types[2]);
        output.str("");
        ir::Print(duplicate, output);
        Require(output.str().find("Node.2 = record(i32, ptr<Node.2>)") != std::string::npos &&
                    output.str().find("Node.4 = record(i32, ptr<Node.2>)") != std::string::npos,
                "same-named records lost their nominal identities");
        duplicate.types[4].name = "i32";
        output.str("");
        ir::Print(duplicate, output);
        Require(output.str().find("i32.4 = record(") != std::string::npos, "record name collides with primitive type");
    }

    void CheckVerifier() {
        auto* allocation = __baka_allocate(64, 64);
        Require(allocation && reinterpret_cast<uintptr_t>(allocation) % 64 == 0,
                "runtime allocation lost alignment");
        __baka_deallocate(allocation);
        Require(__baka_allocate(UINT64_MAX, 64) == nullptr, "runtime allocation accepted overflow");
        __baka_deallocate(nullptr);

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

        auto rejected = [](const ir::Module& candidate, const char* message) {
            const auto diagnostics = ir::Verify(candidate);
            Require(std::any_of(diagnostics.begin(), diagnostics.end(),
                                [&](const auto& diagnostic) { return diagnostic.message == message; }),
                    message);
        };
        const ir::Operand one{ir::OperandKind::Integer, 1, 0, 1};
        const ir::Operand zero{ir::OperandKind::Integer, 1, 0, 0};
        const ir::Operand temporary{ir::OperandKind::Temporary, 1, 1};

        auto calls = module;
        ir::Function callee;
        callee.id = 2;
        callee.name = "callee";
        callee.signature = calls.function(1, {1}, false);
        const ir::Operand target{ir::OperandKind::Function, calls.pointer(callee.signature), callee.id};
        calls.functions.push_back(callee);
        calls.functions[0].temporaries.push_back(1);
        calls.functions[0].blocks[0].instructions = {{ir::Opcode::Param, {}, one},
                                                     ir::Instruction::call(temporary, target),
                                                     {ir::Opcode::Return, {}, temporary}};
        Require(ir::Verify(calls).empty(), "valid param/call sequence rejected");

        bad = calls;
        bad.functions[0].blocks[0].instructions.erase(bad.functions[0].blocks[0].instructions.begin());
        rejected(bad, "call arity mismatch");
        bad = calls;
        bad.functions[0].blocks[0].instructions.insert(bad.functions[0].blocks[0].instructions.begin(),
                                                       {ir::Opcode::Param, {}, one});
        rejected(bad, "call arity mismatch");
        bad = calls;
        bad.functions[0].blocks[0].instructions[1].right = one;
        rejected(bad, "invalid call operands");
        bad = calls;
        bad.functions[0].blocks[0].instructions.insert(bad.functions[0].blocks[0].instructions.begin() + 1,
                                                       {ir::Opcode::Copy, temporary, one});
        rejected(bad, "param sequence must immediately precede call");
        bad = calls;
        bad.functions[0].blocks[0].instructions[0].left = temporary;
        rejected(bad, "temporary used before definition");
        bad = calls;
        bad.functions[0].blocks[0].instructions[0].result = temporary;
        rejected(bad, "invalid param");
        bad = calls;
        bad.functions[0].blocks[0].instructions = {{ir::Opcode::Param, {}, one}};
        rejected(bad, "param sequence lacks call");

        // Params cannot leak across control flow, even along a single CFG edge.
        bad = calls;
        bad.functions[0].blocks[0].instructions = {{ir::Opcode::Param, {}, one},
                                                   {ir::Opcode::Jump, {}, {ir::OperandKind::Block, 0, 2}}};
        bad.functions[0].blocks.push_back(
            {2, {ir::Instruction::call(temporary, target), {ir::Opcode::Return, {}, temporary}}});
        ir::BuildCFG(bad.functions[0]);
        rejected(bad, "call arity mismatch");

        ir::Type floating;
        floating.kind = ir::TypeKind::Floating;
        floating.bits = 64;
        floating.size = floating.alignment = 8;
        const auto floatingId = static_cast<ir::TypeId>(calls.types.size());
        calls.types.push_back(floating);
        bad = calls;
        bad.functions[0].blocks[0].instructions[0].left = {ir::OperandKind::Floating, floatingId};
        rejected(bad, "call argument type mismatch");

        // Variadic extras remain ordinary params, while fixed params stay typed.
        auto variadic = calls;
        variadic.functions[1].signature = variadic.function(1, {1}, true);
        auto& variadicInstructions = variadic.functions[0].blocks[0].instructions;
        variadicInstructions[1].left.type = variadic.pointer(variadic.functions[1].signature);
        variadicInstructions.insert(variadicInstructions.begin() + 1,
                                    {ir::Opcode::Param, {}, {ir::OperandKind::Floating, floatingId}});
        Require(ir::Verify(variadic).empty(), "valid variadic params rejected");
        bad = variadic;
        bad.functions[0].blocks[0].instructions.erase(bad.functions[0].blocks[0].instructions.begin(),
                                                      bad.functions[0].blocks[0].instructions.begin() + 2);
        rejected(bad, "call arity mismatch");

        // A zero-argument call directly after another call consumes no stale params.
        ir::Function noArguments;
        noArguments.id = 3;
        noArguments.name = "no_arguments";
        noArguments.signature = calls.function(1, {}, false);
        const ir::Operand noArgumentsTarget{ir::OperandKind::Function, calls.pointer(noArguments.signature), 3};
        calls.functions.push_back(noArguments);
        calls.functions[0].blocks[0].instructions.insert(calls.functions[0].blocks[0].instructions.begin() + 2,
                                                         ir::Instruction::call(temporary, noArgumentsTarget));
        Require(ir::Verify(calls).empty(), "consecutive/zero-argument call rejected");

        // Both edges of if_goto/goto participate in CFG validation and folding.
        auto branches = module;
        branches.functions[0].blocks = {{1,
                                         {{ir::Opcode::JumpIf, {}, one, {ir::OperandKind::Block, 0, 2}},
                                          {ir::Opcode::Jump, {}, {ir::OperandKind::Block, 0, 3}}}},
                                        {2, {{ir::Opcode::Return, {}, one}}},
                                        {3, {{ir::Opcode::Return, {}, zero}}}};
        ir::BuildCFG(branches.functions[0]);
        Require(ir::Verify(branches).empty(), "valid conditional jump pair rejected");
        Require(branches.functions[0].blocks[0].successors == std::vector<ir::BlockId>({2, 3}),
                "conditional jump pair lost a CFG edge");
        bad = branches;
        bad.functions[0].blocks[0].instructions.pop_back();
        rejected(bad, "conditional jump must be followed by the block's final goto");
        bad = branches;
        bad.functions[0].temporaries.push_back(1);
        bad.functions[0].blocks[0].instructions.insert(bad.functions[0].blocks[0].instructions.begin() + 1,
                                                       {ir::Opcode::Copy, temporary, one});
        rejected(bad, "conditional jump must be followed by the block's final goto");
        for (uint64_t condition : {0, 1}) {
            auto folded = branches;
            folded.functions[0].blocks[0].instructions[0].left.integer = condition;
            ir::Optimize(folded);
            Require(ir::Verify(folded).empty(), "folded conditional jump pair is invalid");
            Require(folded.functions[0].blocks.size() == 2, "constant branch retained unreachable block");
            ir_test::Interpreter executor(folded);
            Require(executor.run(0) == static_cast<int64_t>(condition), "constant branch chose wrong successor");
        }
    }
}

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "verifier") {
            CheckVerifier();
            CheckRecordDefinitions();
            return 0;
        }
        Require(argc == 4 || argc == 5 || argc == 6,
                "usage: ir_execution_tests source input result [fail-allocation|global result]");
        auto module = Compile(argv[1]);
        const int input = std::stoi(argv[2]);
        const int64_t expected = std::stoll(argv[3]);
        for (bool optimize : {false, true}) {
            if (optimize) {
                ir::Optimize(module);
                Require(ir::Verify(module).empty(), "optimized IR failed verification");
            }
            ir_test::Interpreter executor(module);
            if (argc == 5) {
                Require(std::string(argv[4]) == "fail-allocation", "unknown execution option");
                executor.failNextAllocation();
            }
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
