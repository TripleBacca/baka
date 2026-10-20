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
        if (name == "35_static_lifetimes") {
            auto malformed = module;
            bool found = false;
            for (auto& function : malformed.functions) for (auto& block : function.blocks) for (auto& in : block.instructions)
                if (in.opcode == ir::Opcode::RegisterDestructor) { in.right = in.left; found = true; }
            require(found && !ir::Verify(malformed).empty(), "mismatched destructor registration accepted");
        }
        if (name == "29_runtime_global_order" || name == "30_runtime_initializers") {
            require(module.initializer != 0, "runtime globals lack an entry initializer");
            const auto* startup = module.functionById(module.initializer);
            require(startup && startup->defined && startup->internal, "missing startup function body");
            bool runtimeArithmetic = false;
            for (const auto& item : ir::Linearize(*startup)) {
                const auto& in = item.instruction;
                if (in.opcode == ir::Opcode::Add && in.left.kind == ir::OperandKind::Integer &&
                    in.left.integer == 3 && in.right.kind == ir::OperandKind::Integer && in.right.integer == 4) {
                    runtimeArithmetic = true;
                }
            }
            if (name == "30_runtime_initializers") require(runtimeArithmetic, "constant initializer was not emitted as runtime arithmetic");
            for (const auto& object : module.globals) {
                if (object.runtimeInitialized) require(object.initialValues.empty() && object.bytes.empty(),
                                                       "source initializer was pre-filled into data");
            }
            auto malformed = module;
            malformed.initializer = 0xffffffff;
            require(!ir::Verify(malformed).empty(), "unknown startup function accepted");
            malformed = module;
            for (auto& function : malformed.functions) {
                if (function.id == malformed.initializer) function.signature = module.functions.front().signature;
            }
            require(!ir::Verify(malformed).empty(), "non-void startup signature accepted");
            if (name == "29_runtime_global_order") {
                ir_test::Interpreter executor(module);
                require(executor.run(0) == 7 && executor.run(0) == 7, "startup was rerun on a later entry call");
            }
        }
        if (name == "26_aggregate_returns") {
            bool sawAggregateCall = false, sawAggregateReturn = false;
            for (const auto& function : module.functions) {
                if (function.name == "relay") {
                    require(module.type(module.type(function.signature).element).kind == ir::TypeKind::Record,
                            "aggregate return was rewritten to an ABI pointer");
                    require(function.parameters.size() == 1, "hidden return parameter leaked into early TAC");
                    for (const auto& item : ir::Linearize(function)) {
                        const auto& in = item.instruction;
                        if (in.opcode == ir::Opcode::Call) {
                            sawAggregateCall = module.type(in.result.type).kind == ir::TypeKind::Record;
                            require(in.right.integer == 1, "aggregate call acquired a hidden argument");
                        }
                        if (in.opcode == ir::Opcode::Return) {
                            sawAggregateReturn = module.type(in.left.type).kind == ir::TypeKind::Record;
                        }
                    }
                }
            }
            require(sawAggregateCall && sawAggregateReturn, "missing aggregate-valued call/ret");
            auto malformed = module;
            for (auto& function : malformed.functions) {
                if (function.name != "relay") continue;
                for (auto& block : function.blocks) {
                    for (auto& in : block.instructions) {
                        if (in.opcode == ir::Opcode::Return) {
                            auto scalar = module.type(function.signature).parameters.front();
                            in.left = {ir::OperandKind::Integer, scalar, 0, 0};
                        }
                    }
                }
            }
            require(!ir::Verify(malformed).empty(), "scalar return accepted for record signature");
        }
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
        if (name == "23_array_address") examples = {{0,71},{2,73},{-2,69}};
        if (name == "24_default_arguments") examples = {{0,214},{2,234},{-1,204}};
        if (name == "25_aggregate_parameters") examples = {{0,116},{2,118},{-1,115}};
        if (name == "26_aggregate_returns") examples = {{0,15},{2,21},{-1,12}};
        if (name == "27_aggregate_copies") examples = {{0,8},{2,58},{-1,52}};
        if (name == "28_aggregate_layouts") examples = {{0,105},{2,109},{-1,103}};
        if (name == "29_runtime_global_order") examples = {{0,7},{2,7},{-1,7}};
        if (name == "30_runtime_initializers") examples = {{0,1022},{2,1026},{-1,1020}};
        if (name == "31_runtime_static_paths") examples = {{0,376},{2,376},{-1,376}};
        if (name == "32_runtime_startup_cfg") examples = {{0,418},{2,418},{-1,418}};
        if (name == "33_constructors") examples = {{0,122255},{2,122255}};
        if (name == "34_subobject_lifetimes") examples = {{0,123456},{2,123456}};
        if (name == "35_static_lifetimes") examples = {{0,132},{2,132}};
        if (name == "36_cleanup_jumps") examples = {{0,112},{2,112}};
        if (name == "37_temporary_lifetimes") examples = {{0,1},{2,1}};
        if (name == "38_array_lifetimes") examples = {{0,12213443},{2,12213443}};
        if (name == "39_goto_lifetimes") examples = {{0,31},{2,31}};
        if (name == "40_for_initializer_lifetimes") examples = {{0,3},{2,3}};
        if (name == "41_aggregate_ownership") examples = {{0,453},{2,453}};
        if (name == "42_default_member_lifetimes") examples = {{0,716},{2,716}};
        if (name == "43_construction_destinations") examples = {{0,477},{2,576}};
        if (name == "44_registration_before_temporaries") examples = {{0,0},{2,0}};
        require(!examples.empty(), "unknown execution scenario");
        for (auto [input, expected] : examples) {
            ir_test::Interpreter rawExecutor(module), optimizedExecutor(optimized);
            auto original = rawExecutor.run(input);
            auto transformed = optimizedExecutor.run(input);
            if (name == "35_static_lifetimes") {
                require(rawExecutor.globalInteger("trace") == 132231 && optimizedExecutor.globalInteger("trace") == 132231,
                        "static destructors did not run in reverse construction order");
            }
            if (name == "41_aggregate_ownership") {
                require(rawExecutor.globalInteger("destroyed") == 6 && optimizedExecutor.globalInteger("destroyed") == 6,
                        "aggregate ownership was destroyed twice or leaked");
                require(rawExecutor.globalInteger("ids") == 20 && optimizedExecutor.globalInteger("ids") == 20,
                        "temporary method receiver did not retain its object storage");
            }
            if (name == "44_registration_before_temporaries") {
                require(rawExecutor.globalInteger("trace") == 21 && optimizedExecutor.globalInteger("trace") == 21,
                        "temporary cleanup reordered static destructor registration");
            }
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
