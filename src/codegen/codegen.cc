#include "codegen.hh"
#include "internal.hh"
#include <ostream>

namespace baka {
    namespace codegen {
        namespace {
            std::string Mangle(const std::string& name, uint32_t id) {
                auto symbol = name + "." + std::to_string(id);
                return symbol.front() == '.' ? "__baka" + symbol : symbol;
            }
        }

        std::string FunctionSymbol(const ir::Function& function) {
            return function.defined ? Mangle(function.name, function.id) : function.name;
        }

        std::string ObjectSymbol(const ir::Object& object) {
            return object.external ? object.name : Mangle(object.name, object.id);
        }

        const ir::Function* UserMain(const ir::Module& module) {
            for (const auto& function : module.functions) {
                if (function.defined && !function.internal && function.name == "main") {
                    return &function;
                }
            }
            return nullptr;
        }

        bool Generate(const ir::Module& module, const Options& options, std::ostream& output,
                      std::vector<ir::Diagnostic>& diagnostics) {
            // Every function is selected before anything is printed, so a module the
            // backend cannot represent never produces a partial assembly file.
            LiteralPool pool;
            std::vector<MFunction> functions;
            try {
                for (const auto& function : module.functions) {
                    if (!function.defined) {
                        continue;
                    }
                    auto machine = SelectFunction(module, function, pool);
                    if (options.optimize) {
                        Peephole(machine);
                    }
                    functions.push_back(std::move(machine));
                }
            } catch (const Unsupported& error) {
                diagnostics.push_back({std::string("code generation: ") + error.what(), {}});
                return false;
            }
            output << "# baka: MIPS64r2 n64 little-endian, non-PIC static\n"
                   << "\t.abicalls\n\t.option pic0\n\t.set noreorder\n\t.set nomips16\n";
            output << "\t.text\n";
            for (const auto& function : functions) {
                PrintFunction(function, output);
            }
            PrintRuntime(module, output);
            PrintData(module, pool, output);
            return true;
        }
    }
}
