#include "constant_folding.hh"
#include <map>

namespace baka {
    namespace ir {
        namespace {
            void SubstituteOperand(Operand& operand, const std::map<uint32_t, Operand>& values) {
                if (operand.kind != OperandKind::Temporary) {
                    return;
                }
                auto found = values.find(operand.id);
                if (found != values.end()) {
                    operand = found->second;
                }
            }

            void SubstituteUses(Instruction& instruction, const std::map<uint32_t, Operand>& values) {
                SubstituteOperand(instruction.left, values);
                SubstituteOperand(instruction.right, values);
            }
        }

        void ConstantFold(Module& module) {
            for (auto& function : module.functions) {
                for (auto& block : function.blocks) {
                    std::map<uint32_t, Operand> constants;
                    for (auto& in : block.instructions) {
                        SubstituteUses(in, constants);
                        if (DefinesTemporary(in)) {
                            constants.erase(in.result.id);
                            if (auto value = FoldConstant(module, in)) {
                                in.opcode = Opcode::Copy;
                                in.left = *value;
                                in.right = {};
                            }
                            if (in.opcode == Opcode::Copy &&
                                (in.left.kind == OperandKind::Integer || in.left.kind == OperandKind::Floating)) {
                                constants[in.result.id] = in.left;
                            }
                        }
                    }
                    // A conditional block ends with if_goto + goto. Fold the pair
                    // as a unit so no instructions can follow an unconditional jump.
                    if (block.instructions.size() >= 2) {
                        auto& conditional = block.instructions[block.instructions.size() - 2];
                        if (conditional.opcode == Opcode::JumpIf && conditional.left.kind == OperandKind::Integer &&
                            block.instructions.back().opcode == Opcode::Jump) {
                            if (conditional.left.integer) {
                                block.instructions.back() = {
                                    Opcode::Jump, {}, conditional.right, {}, conditional.source};
                            }
                            block.instructions.erase(block.instructions.end() - 2);
                        }
                    }
                }
            }
        }

        void CopyPropagate(Module& module) {
            for (auto& function : module.functions) {
                for (auto& block : function.blocks) {
                    std::map<uint32_t, Operand> copies;
                    for (auto& in : block.instructions) {
                        SubstituteUses(in, copies);
                        if (!DefinesTemporary(in)) {
                            continue;
                        }
                        const auto id = in.result.id;
                        for (auto it = copies.begin(); it != copies.end();) {
                            if (it->first == id || (it->second.kind == OperandKind::Temporary && it->second.id == id)) {
                                it = copies.erase(it);
                            } else {
                                ++it;
                            }
                        }
                        if (in.opcode == Opcode::Copy &&
                            !(in.left.kind == OperandKind::Temporary && in.left.id == id)) {
                            copies[id] = in.left;
                        }
                    }
                }
            }
        }

        void Optimize(Module& module) {
            ConstantFold(module);
            CopyPropagate(module);
            EliminateDeadCode(module);
            for (auto& function : module.functions) {
                BuildCFG(function);
            }
        }
    }
}
