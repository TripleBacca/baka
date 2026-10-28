#include "constant_folding.hh"
#include "base/numeric.hh"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace baka {
    namespace ir {
        namespace {
            using Facts = std::map<uint32_t, Operand>;

            bool SameOperand(const Operand& a, const Operand& b) {
                if (a.kind != b.kind || a.type != b.type || a.id != b.id || a.integer != b.integer) return false;
                if (a.kind != OperandKind::Floating) return true;
                return (a.floating == b.floating && std::signbit(a.floating) == std::signbit(b.floating)) ||
                       (std::isnan(a.floating) && std::isnan(b.floating));
            }

            bool SameInstruction(const Instruction& a, const Instruction& b) {
                return a.opcode == b.opcode && SameOperand(a.result, b.result) &&
                       SameOperand(a.left, b.left) && SameOperand(a.right, b.right) && a.source == b.source;
            }

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

            void Kill(Facts& facts, uint32_t id) {
                for (auto it = facts.begin(); it != facts.end();) {
                    if (it->first == id || (it->second.kind == OperandKind::Temporary && it->second.id == id)) {
                        it = facts.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            void Transfer(Instruction instruction, Facts& facts) {
                SubstituteUses(instruction, facts);
                if (!DefinesTemporary(instruction)) {
                    return;
                }
                const auto id = instruction.result.id;
                Kill(facts, id);
                if (instruction.opcode == Opcode::Copy && instruction.left.kind != OperandKind::None &&
                    !(instruction.left.kind == OperandKind::Floating && !std::isfinite(instruction.left.floating)) &&
                    !(instruction.left.kind == OperandKind::Temporary && instruction.left.id == id)) {
                    facts[id] = instruction.left;
                }
            }

            void PropagateAcrossBlocks(Function& function) {
                if (!function.defined || function.blocks.empty()) {
                    return;
                }
                BuildCFG(function);
                std::map<BlockId, Facts> incoming, outgoing;
                const auto entry = function.blocks.front().id;
                bool changed;
                do {
                    changed = false;
                    for (const auto& block : function.blocks) {
                        Facts in;
                        bool first = true;
                        if (block.id == entry) {
                            first = false;
                        } else {
                            for (auto predecessor : block.predecessors) {
                                if (first) {
                                    in = outgoing[predecessor];
                                    first = false;
                                } else {
                                    for (auto it = in.begin(); it != in.end();) {
                                        const auto found = outgoing[predecessor].find(it->first);
                                        if (found == outgoing[predecessor].end() ||
                                            !SameOperand(found->second, it->second)) {
                                            it = in.erase(it);
                                        } else {
                                            ++it;
                                        }
                                    }
                                }
                            }
                        }
                        if (first) {
                            in.clear();
                        }
                        auto out = in;
                        for (const auto& instruction : block.instructions) {
                            Transfer(instruction, out);
                        }
                        auto sameFacts = [](const Facts& a, const Facts& b) {
                            if (a.size() != b.size()) return false;
                            for (const auto& [id, value] : a) {
                                auto found = b.find(id);
                                if (found == b.end() || !SameOperand(value, found->second)) return false;
                            }
                            return true;
                        };
                        if (!sameFacts(incoming[block.id], in) || !sameFacts(outgoing[block.id], out)) {
                            incoming[block.id] = std::move(in);
                            outgoing[block.id] = std::move(out);
                            changed = true;
                        }
                    }
                } while (changed);
                for (auto& block : function.blocks) {
                    auto facts = incoming[block.id];
                    for (auto& instruction : block.instructions) {
                        const auto original = instruction;
                        SubstituteUses(instruction, facts);
                        Transfer(original, facts);
                    }
                }
            }

            bool IsIntegerZero(const Operand& value) {
                return value.kind == OperandKind::Integer && value.integer == 0;
            }

            bool IsIntegerOne(const Operand& value) {
                return value.kind == OperandKind::Integer && value.integer == 1;
            }

            void SimplifyIdentity(const Module& module, Instruction& instruction) {
                if (!DefinesTemporary(instruction) || module.type(instruction.result.type).kind != TypeKind::Integer) {
                    return;
                }
                Operand replacement;
                switch (instruction.opcode) {
                case Opcode::Add:
                case Opcode::BitOr:
                case Opcode::BitXor:
                    if (IsIntegerZero(instruction.right)) replacement = instruction.left;
                    else if (IsIntegerZero(instruction.left)) replacement = instruction.right;
                    break;
                case Opcode::Subtract:
                case Opcode::ShiftLeft:
                case Opcode::ShiftRight:
                    if (IsIntegerZero(instruction.right)) replacement = instruction.left;
                    break;
                case Opcode::Multiply:
                    if (IsIntegerOne(instruction.right)) replacement = instruction.left;
                    else if (IsIntegerOne(instruction.left)) replacement = instruction.right;
                    break;
                case Opcode::BitAnd: {
                    const auto& type = module.type(instruction.result.type);
                    const auto all = numeric::mask({type.bits, type.isSigned, false});
                    if (instruction.right.kind == OperandKind::Integer && instruction.right.integer == all)
                        replacement = instruction.left;
                    else if (instruction.left.kind == OperandKind::Integer && instruction.left.integer == all)
                        replacement = instruction.right;
                    break;
                }
                default:
                    break;
                }
                if (replacement && replacement.type == instruction.result.type) {
                    instruction.opcode = Opcode::Copy;
                    instruction.left = replacement;
                    instruction.right = {};
                }
            }

            void ValueNumber(const Module& module, Block& block) {
                std::vector<Instruction> available;
                for (auto& instruction : block.instructions) {
                    if (DefinesTemporary(instruction)) {
                        const auto id = instruction.result.id;
                        std::erase_if(available, [&](const Instruction& prior) {
                            return prior.result.id == id ||
                                   (prior.left.kind == OperandKind::Temporary && prior.left.id == id) ||
                                   (prior.right.kind == OperandKind::Temporary && prior.right.id == id);
                        });
                    }
                    if (!DefinesTemporary(instruction) || instruction.opcode < Opcode::Add ||
                        instruction.opcode > Opcode::BitXor || instruction.opcode == Opcode::Divide ||
                        instruction.opcode == Opcode::Remainder ||
                        module.type(instruction.result.type).kind != TypeKind::Integer) continue;
                    const auto found = std::find_if(available.begin(), available.end(), [&](const Instruction& prior) {
                        return prior.opcode == instruction.opcode && prior.result.type == instruction.result.type &&
                               SameOperand(prior.left, instruction.left) && SameOperand(prior.right, instruction.right);
                    });
                    if (found != available.end()) {
                        instruction.opcode = Opcode::Copy;
                        instruction.left = found->result;
                        instruction.right = {};
                    } else {
                        available.push_back(instruction);
                    }
                }
            }

            void SimplifyBranches(Function& function) {
                for (auto& block : function.blocks) {
                    if (block.instructions.size() < 2) continue;
                    auto& condition = block.instructions[block.instructions.size() - 2];
                    auto& alternative = block.instructions.back();
                    if (condition.opcode != Opcode::JumpIf || alternative.opcode != Opcode::Jump) continue;
                    if (condition.right == alternative.left || condition.left.kind == OperandKind::Integer ||
                        condition.left.kind == OperandKind::Floating) {
                        auto target = alternative.left;
                        if (condition.right == alternative.left ||
                            (condition.left.kind == OperandKind::Integer && condition.left.integer) ||
                            (condition.left.kind == OperandKind::Floating && condition.left.floating != 0)) {
                            target = condition.right;
                        }
                        alternative.left = target;
                        block.instructions.erase(block.instructions.end() - 2);
                    }
                }
            }

            void ThreadJumps(Function& function) {
                std::map<BlockId, BlockId> redirects;
                for (const auto& block : function.blocks) {
                    if (block.instructions.size() == 1 && block.instructions.front().opcode == Opcode::Jump) {
                        redirects[block.id] = block.instructions.front().left.id;
                    }
                }
                auto resolve = [&](Operand& target) {
                    if (target.kind != OperandKind::Block) return;
                    std::set<BlockId> seen;
                    auto id = target.id;
                    while (redirects.count(id) && seen.insert(id).second && !seen.count(redirects.at(id))) {
                        id = redirects.at(id);
                    }
                    target.id = id;
                };
                for (auto& block : function.blocks) {
                    for (auto& instruction : block.instructions) {
                        if (instruction.opcode == Opcode::Jump) resolve(instruction.left);
                        else if (instruction.opcode == Opcode::JumpIf) resolve(instruction.right);
                    }
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
            size_t maxIterations = 64;
            for (const auto& function : module.functions)
                for (const auto& block : function.blocks) maxIterations += block.instructions.size() + 1;
            for (size_t iteration = 0; iteration < maxIterations; ++iteration) {
                std::vector<std::pair<BlockId, std::vector<Instruction>>> before;
                for (const auto& function : module.functions)
                    for (const auto& block : function.blocks) before.emplace_back(block.id, block.instructions);
                ConstantFold(module);
                CopyPropagate(module);
                EliminateDeadCode(module);
                for (auto& function : module.functions) {
                    PropagateAcrossBlocks(function);
                    for (auto& block : function.blocks) {
                        for (auto& instruction : block.instructions) SimplifyIdentity(module, instruction);
                        ValueNumber(module, block);
                    }
                    SimplifyBranches(function);
                    ThreadJumps(function);
                    BuildCFG(function);
                }
                EliminateDeadCode(module);
                std::vector<std::pair<BlockId, std::vector<Instruction>>> after;
                for (const auto& function : module.functions)
                    for (const auto& block : function.blocks) after.emplace_back(block.id, block.instructions);
                bool equal = before.size() == after.size();
                for (size_t i = 0; equal && i < before.size(); ++i) {
                    equal = before[i].first == after[i].first &&
                            before[i].second.size() == after[i].second.size();
                    for (size_t j = 0; equal && j < before[i].second.size(); ++j)
                        equal = SameInstruction(before[i].second[j], after[i].second[j]);
                }
                if (equal) return;
            }
            throw std::runtime_error("IR optimization did not converge");
        }
    }
}
