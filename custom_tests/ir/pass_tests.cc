#include "ir/lowering.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include "driver/gctx.hh"
#include "interpreter.hh"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace baka;

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    std::string dump(const types::ProgramNode& program) {
        std::ostringstream output;
        auto* previous = std::cout.rdbuf(output.rdbuf());
        program.Print();
        std::cout.rdbuf(previous);
        return output.str();
    }

    size_t count(const ir::Module& module) {
        size_t result = 0;
        for (const auto& function : module.functions) {
            result += ir::Linearize(function).size();
        }
        return result;
    }

    ir::Module fixture(const std::string& name) {
        std::ifstream file(std::string(IR_TEST_ROOT) + "/" + name + ".cpp");
        require(bool(file), "missing IR test fixture");
        std::string source((std::istreambuf_iterator<char>(file)), {});
        driver::Gctx::AttachLineIndex(std::make_shared<base::LineIndex>(source));
        auto tokens = lexer::Tokenize(source);
        parser::Parser parser(tokens.Tokens, tokens.Locations);
        auto* program = dynamic_cast<types::ProgramNode*>(parser.Parse());
        require(program && !driver::Gctx::ErrorFound(), "IR fixture failed parsing");
        semantic::SemanticAnalyzer analyzer;
        if (!analyzer.analyze(*program)) {
            for (const auto& diagnostic : analyzer.diagnostics()) {
                std::cerr << diagnostic.message << '\n';
            }
            throw std::runtime_error("IR fixture failed semantic analysis");
        }
        auto before = dump(*program);
        auto typeCount = analyzer.types().typeCount(), symbolCount = size_t(analyzer.symbols().symbolCount());
        ir::Lowerer lowerer(analyzer);
        if (!lowerer.lower(*program, false)) {
            for (const auto& diagnostic : lowerer.diagnostics()) {
                std::cerr << diagnostic.message << '\n';
            }
            throw std::runtime_error("IR lowering failed");
        }
        require(before == dump(*program), "IR mutated the semantic AST");
        require(typeCount == analyzer.types().typeCount() && symbolCount == analyzer.symbols().symbolCount(),
                "IR mutated semantic arenas");
        require(ir::Verify(lowerer.module()).empty(), "unoptimized IR failed verification");
        return lowerer.takeModule();
    }

    void execution(const std::string& name) {
        auto module = fixture(name); // frontend tables/source go out of scope here
        auto optimized = module;
        ir::Optimize(optimized);
        require(ir::Verify(optimized).empty(), "optimized IR failed verification");
        require(count(optimized) <= count(module), "optimization grew instruction count");
        std::ostringstream text;
        ir::Print(optimized, text);
        require(text.str().find("IR::TAC:") == 0, "IR did not retain owned printable data");
        std::vector<std::pair<int, int64_t>> examples;
        if (name == "01_scalars") {
            examples = {{0, 10}, {2, 11}, {-2, 11}};
        }
        if (name == "02_control") {
            examples = {{-1, 1}, {0, 2}, {1, 3}};
        }
        if (name == "03_calls") {
            examples = {{0, 4}, {2, 20}, {3, 28}};
        }
        if (name == "04_arrays") {
            examples = {{0, 128}, {2, 128}};
        }
        if (name == "05_sequence") {
            examples = {{0, 9}, {2, 12}, {-1, 6}};
        }
        if (name == "06_switch") {
            examples = {{0, 5}, {1, 5}, {2, 3}, {3, 5}};
        }
        if (name == "07_globals") {
            examples = {{0, 218}, {2, 224}, {-1, 215}};
        }
        if (name == "08_inheritance") {
            examples = {{0, 5}, {2, 7}};
        }
        if (name == "09_returns") {
            examples = {{0, 1}, {2, 1}};
        }
        if (name == "10_lvalues") {
            examples = {{0, 10}, {1, 11}};
        }
        if (name == "11_wrap") {
            examples = {{0, 2}, {8, 4}, {-4, 1073741825}};
        }
        if (name == "12_arguments") {
            examples = {{0, 56}, {4, 56}};
        }
        if (name == "13_operators") {
            examples = {{0, 15}, {1, 14}, {2, 19}};
        }
        if (name == "14_labeled") {
            examples = {{0, 1}, {2, 1}};
        }
        if (name == "15_once") {
            examples = {{0, 355}, {2, 377}};
        }
        if (name == "16_constants") {
            examples = {{0, 25}, {2, 27}};
        }
        if (name == "17_relocations") {
            examples = {{0, 8}, {2, 12}};
        }
        if (name == "18_static_conversions") {
            examples = {{0, 5}, {2, 5}};
        }
        if (name == "19_variadic") {
            examples = {{0, 0}, {2, 2}, {-1, -1}};
        }
        if (name == "20_casts") {
            examples = {{0, 91}, {2, 95}, {-2, 87}};
        }
        if (name == "21_mutation_conversions") {
            examples = {{0, 101}, {2, 101}};
        }
        if (name == "22_forward_types") {
            examples = {{0, 10}, {2, 10}};
        }
        require(!examples.empty(), "unknown execution scenario");
        for (auto [input, expected] : examples) {
            auto original = ir_test::Interpreter(module).run(input);
            auto transformed = ir_test::Interpreter(optimized).run(input);
            if (original != expected || transformed != expected) {
                std::cerr << name << " input " << input << ": expected " << expected << ", raw " << original
                          << ", optimized " << transformed << '\n';
                throw std::runtime_error("TAC execution produced wrong result");
            }
        }
    }

    ir::Module minimal() {
        ir::Module module;
        ir::Type integer;
        integer.kind = ir::TypeKind::Integer;
        integer.bits = 32;
        integer.isSigned = true;
        integer.size = integer.alignment = 4;
        module.types.push_back(integer);
        auto signature = module.function(1, {}, false);
        ir::Function function;
        function.id = 1;
        function.name = "main";
        function.signature = signature;
        function.defined = true;
        function.blocks.push_back({1, {{ir::Opcode::Return, {}, {ir::OperandKind::Integer, 1, 0, 0}, {}, {}}}, {}, {}});
        module.functions.push_back(function);
        ir::BuildCFG(module.functions.front());
        return module;
    }

    void verifier() {
        auto valid = minimal();
        require(ir::Verify(valid).empty(), "minimal module rejected");
        auto bad = valid;
        bad.functions[0].blocks[0].instructions.clear();
        require(!ir::Verify(bad).empty(), "missing terminator accepted");
        bad = valid;
        bad.functions[0].temporaries.push_back(1);
        bad.functions[0].blocks[0].instructions[0].left = {ir::OperandKind::Temporary, 1, 1};
        require(!ir::Verify(bad).empty(), "undefined temporary accepted");
        bad = valid;
        bad.functions[0].blocks[0].instructions[0] = {ir::Opcode::Jump, {}, {ir::OperandKind::Block, 0, 99}};
        ir::BuildCFG(bad.functions[0]);
        require(!ir::Verify(bad).empty(), "missing block accepted");
        bad = valid;
        bad.functions[0].blocks[0].successors.push_back(1);
        require(!ir::Verify(bad).empty(), "inconsistent CFG accepted");
        bad = valid;
        bad.functions[0].blocks[0].instructions[0].left.type = 99;
        require(!ir::Verify(bad).empty(), "bad operand type accepted");
        bad = valid;
        bad.functions[0].blocks[0].instructions.insert(
            bad.functions[0].blocks[0].instructions.begin(),
            {ir::Opcode::Argument, {}, {ir::OperandKind::Integer, 1, 0, 1}, {ir::OperandKind::Integer, 1, 0, 0}});
        require(!ir::Verify(bad).empty(), "dangling arguments accepted");
    }

    void cfg() {
        auto module = minimal();
        auto& function = module.functions[0];
        const auto zero = ir::Operand{ir::OperandKind::Integer, 1, 0, 0};
        function.blocks = {
            {1, {{ir::Opcode::Branch, {ir::OperandKind::Block, 0, 3}, zero, {ir::OperandKind::Block, 0, 2}}}},
            {2, {{ir::Opcode::Jump, {}, {ir::OperandKind::Block, 0, 4}}}},
            {3, {{ir::Opcode::Jump, {}, {ir::OperandKind::Block, 0, 4}}}},
            {4, {{ir::Opcode::Return, {}, zero}}}};
        ir::BuildCFG(function);
        require(function.blocks[0].successors == std::vector<ir::BlockId>{2, 3}, "branch edges are wrong");
        require(function.blocks[3].predecessors == std::vector<ir::BlockId>{2, 3}, "join predecessors are wrong");
        require(ir::Verify(module).empty(), "valid diamond CFG rejected");
        auto flat = ir::Linearize(function);
        require(flat.size() == 4 && flat[2].block == 3, "linearization lost block ownership");
        function.temporaries.push_back(1);
        const auto temporary = ir::Operand{ir::OperandKind::Temporary, 1, 1};
        function.blocks[1].instructions.insert(function.blocks[1].instructions.begin(),
                                               {ir::Opcode::Copy, temporary, zero});
        function.blocks[3].instructions[0].left = temporary;
        require(!ir::Verify(module).empty(), "join accepted a temporary defined on only one incoming path");
    }

    void contracts() {
        semantic::SemanticAnalyzer analyzer;
        types::ProgramNode program;
        ir::Lowerer lowerer(analyzer);
        require(!lowerer.lower(program), "IR accepted an unchecked program");
        require(!lowerer.diagnostics().empty(), "unchecked program lacked an IR diagnostic");
    }

    void optimization() {
        auto module = minimal();
        auto& function = module.functions[0];
        function.temporaries = {0, 1, 1, 1};
        ir::Operand a{ir::OperandKind::Temporary, 1, 1}, b{ir::OperandKind::Temporary, 1, 2},
            c{ir::OperandKind::Temporary, 1, 3};
        function.blocks[0].instructions = {
            {ir::Opcode::Copy, a, {ir::OperandKind::Integer, 1, 0, 1}},
            {ir::Opcode::Copy, b, a},
            {ir::Opcode::Copy, a, {ir::OperandKind::Integer, 1, 0, 2}},
            {ir::Opcode::Add, c, {ir::OperandKind::Integer, 1, 0, 0x7fffffff}, {ir::OperandKind::Integer, 1, 0, 1}},
            {ir::Opcode::Return, {}, b}};
        ir::Optimize(module);
        require(ir::Verify(module).empty(), "optimized mutable copies failed verification");
        require(module.functions[0].blocks[0].instructions.size() == 1, "dead copies not removed");
        require(module.functions[0].blocks[0].instructions.back().left.integer == 1,
                "propagation ignored temporary reassignment");
        module = minimal();
        module.functions[0].temporaries = {0, 1};
        module.functions[0].blocks[0].instructions = {
            {ir::Opcode::Add, a, {ir::OperandKind::Integer, 1, 0, 0x7fffffff}, {ir::OperandKind::Integer, 1, 0, 1}},
            {ir::Opcode::Return, {}, a}};
        ir::Optimize(module);
        require(module.functions[0].blocks[0].instructions.back().left.integer == 0x80000000u,
                "constant folding did not wrap signed overflow");
    }
}

int main(int argc, char** argv) {
    try {
        require(argc == 2, "supply one IR test scenario");
        std::string scenario = argv[1];
        if (scenario == "verifier") {
            verifier();
        } else if (scenario == "cfg") {
            cfg();
        } else if (scenario == "contracts") {
            contracts();
        } else if (scenario == "optimization") {
            optimization();
        } else {
            execution(scenario);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
