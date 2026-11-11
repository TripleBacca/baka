#include "internal.hh"
#include <ostream>

namespace baka {
    namespace codegen {
        namespace {
            std::string Text(const MOperand& operand) {
                switch (operand.kind) {
                case MOperand::Kind::Register:
                    return reg::Name(operand.reg);
                case MOperand::Kind::Immediate:
                    return std::to_string(operand.imm);
                case MOperand::Kind::Memory:
                    return std::to_string(operand.imm) + "(" + reg::Name(operand.reg) + ")";
                case MOperand::Kind::Label:
                    return operand.name;
                case MOperand::Kind::Symbol:
                    if (operand.imm == 0) {
                        return operand.name;
                    }
                    return operand.name + (operand.imm > 0 ? "+" : "-") +
                           std::to_string(operand.imm > 0 ? operand.imm : -operand.imm);
                default:
                    return {};
                }
            }

            // Division is printed with an explicit $zero destination: the two-operand
            // form would make the assembler insert divide-by-zero checks and rewrite it.
            bool IsDivide(MOp op) {
                return op == MOp::Div || op == MOp::Divu || op == MOp::Ddiv || op == MOp::Ddivu;
            }

            void PrintInstruction(const MInst& inst, std::ostream& output) {
                if (inst.op == MOp::Label) {
                    output << inst.operands.at(0).name << ":";
                } else if (inst.op == MOp::Comment) {
                    output << "\t# " << inst.comment;
                } else {
                    output << "\t" << Info(inst.op).mnemonic;
                    std::string separator = "\t";
                    if (IsDivide(inst.op)) {
                        output << separator << "$zero";
                        separator = ", ";
                    }
                    for (const auto& operand : inst.operands) {
                        output << separator << Text(operand);
                        separator = ", ";
                    }
                    if (inst.op == MOp::Movf || inst.op == MOp::Movt) {
                        output << separator << "$fcc0";
                    }
                }
                if (inst.op != MOp::Comment && !inst.comment.empty()) {
                    output << "\t# " << inst.comment;
                }
                output << "\n";
            }
        }

        void PrintFunction(const MFunction& function, std::ostream& output) {
            if (function.global) {
                output << "\t.globl\t" << function.symbol << "\n";
            }
            output << "\t.p2align\t3\n\t.type\t" << function.symbol << ", @function\n\t.ent\t" << function.symbol
                   << "\n" << function.symbol << ":\n";
            for (const auto& inst : function.code) {
                PrintInstruction(inst, output);
            }
            output << "\t.end\t" << function.symbol << "\n\t.size\t" << function.symbol << ", .-" << function.symbol
                   << "\n";
        }
    }
}
