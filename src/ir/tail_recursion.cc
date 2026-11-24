#include "ir.hh"
#include <algorithm>
#include <unordered_set>

namespace baka {
    namespace ir {
        namespace {
            // Post-call cleanup or computation keeps the call and return apart.
            std::optional<size_t> MatchTailSelfCall(const Function& function, const Block& block) {
                const auto& code = block.instructions;
                if (code.size() < 2 || code.back().opcode != Opcode::Return) return {};

                const size_t callIndex = code.size() - 2;
                const auto& call = code[callIndex];
                if (call.opcode != Opcode::Call || call.left.kind != OperandKind::Function ||
                    call.left.id != function.id || call.left.integer != 0 ||
                    !call.result || code.back().left != call.result) return {};

                size_t firstParam = callIndex;
                while (firstParam > 0 && code[firstParam - 1].opcode == Opcode::Param) --firstParam;
                if (callIndex - firstParam != function.parameters.size()) return {};
                return firstParam;
            }

            // The old invocation stays alive across a real call. Reusing it is safe here
            // only when every frame object is replaced and its address is unobservable.
            bool CanReuseFrame(const Module& module, const Function& function) {
                const auto& signature = module.type(function.signature);
                const auto isNumeric = [&](TypeId type) {
                    const auto kind = module.type(type).kind;
                    return kind == TypeKind::Integer || kind == TypeKind::Floating;
                };
                if (signature.variadic || !isNumeric(signature.element) ||
                    !std::all_of(signature.parameters.begin(), signature.parameters.end(), isNumeric)) return false;

                const std::unordered_set<uint32_t> parameters(function.parameters.begin(), function.parameters.end());
                if (parameters.size() != function.objects.size() || parameters.size() != function.parameters.size() ||
                    !std::all_of(function.objects.begin(), function.objects.end(),
                                 [&](const Object& object) { return parameters.contains(object.id); })) return false;

                const auto isParameterAddress = [&](const Operand& operand) {
                    return operand.kind == OperandKind::Object && parameters.contains(operand.id);
                };
                for (const auto& block : function.blocks) {
                    for (const auto& inst : block.instructions) {
                        if (isParameterAddress(inst.right)) return false;
                        if (isParameterAddress(inst.left) &&
                            (inst.left.integer != 0 || (inst.opcode != Opcode::Load && inst.opcode != Opcode::Store))) {
                            return false;
                        }
                    }
                }
                return true;
            }

            void RewriteTailSelfCall(Block& block, size_t firstParam, const std::vector<Operand>& parameters,
                                     BlockId entry) {
                auto& code = block.instructions;
                const auto returnSource = code.back().source;
                // Param operands already contain the evaluated argument values, so writes can
                // reuse their slots even when arguments depend on each other's old values.
                for (size_t i = 0; i < parameters.size(); ++i) {
                    auto& argument = code[firstParam + i];
                    argument = Instruction::store(parameters[i], argument.left, argument.source);
                }
                code.resize(firstParam + parameters.size());
                code.push_back({Opcode::Jump, {}, {OperandKind::Block, 0, entry}, {}, returnSource});
            }
        }

        void EliminateTailRecursion(Module& module) {
            for (auto& function : module.functions) {
                if (!function.defined || function.blocks.empty()) continue;

                std::vector<std::pair<Block*, size_t>> sites;
                for (auto& block : function.blocks) {
                    if (auto firstParam = MatchTailSelfCall(function, block)) sites.emplace_back(&block, *firstParam);
                }
                if (sites.empty() || !CanReuseFrame(module, function)) continue;

                std::vector<Operand> addresses;
                addresses.reserve(function.parameters.size());
                for (auto id : function.parameters) {
                    const auto type = module.object(function, id)->type;
                    addresses.push_back({OperandKind::Object, module.pointer(type), id});
                }
                for (const auto& [block, firstParam] : sites) {
                    RewriteTailSelfCall(*block, firstParam, addresses, function.blocks.front().id);
                }
                BuildCFG(function);
            }
        }
    }
}
