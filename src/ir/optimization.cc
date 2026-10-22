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
                for (auto& argument : instruction.arguments) {
                    SubstituteOperand(argument, values);
                }
                if (instruction.opcode == Opcode::Store || instruction.opcode == Opcode::ZeroMemory) {
                    SubstituteOperand(instruction.result, values);
                }
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
                        if (in.opcode == Opcode::Branch && in.left.kind == OperandKind::Integer) {
                            auto destination = in.left.integer ? in.right : in.result;
                            in = {Opcode::Jump, {}, destination, {}, in.source};
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
