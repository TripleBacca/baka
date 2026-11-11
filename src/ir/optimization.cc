#include "constant_folding.hh"
#include "base/numeric.hh"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <stdexcept>

namespace baka {
    namespace ir {
        namespace {
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

            bool IsIntegerZero(const Operand& value) {
                return value.kind == OperandKind::Integer && value.integer == 0;
            }

            // Integer-only rewrites. All constants are compared at the result width, and every
            // rewrite is exact under wrapping arithmetic. Temporaries cannot fault, so dropping
            // an operand (x*0, x-x) never hides a trap. Floating point is excluded entirely.
            void SimplifyIdentity(const Module& module, Instruction& instruction) {
                if (!DefinesTemporary(instruction) || module.type(instruction.result.type).kind != TypeKind::Integer) {
                    return;
                }
                const auto& type = module.type(instruction.result.type);
                const auto all = numeric::mask({type.bits, type.isSigned, false});
                const auto resultType = instruction.result.type;
                const auto& l = instruction.left;
                const auto& r = instruction.right;
                auto constant = [&](uint64_t value) { return Operand{OperandKind::Integer, resultType, 0, value}; };
                auto is = [&](const Operand& v, uint64_t value) {
                    return v.kind == OperandKind::Integer && (v.integer & all) == value;
                };
                auto exponent = [&](const Operand& v) {
                    const auto bits = v.kind == OperandKind::Integer ? v.integer & all : 0;
                    return bits && !(bits & (bits - 1)) ? std::countr_zero(bits) : -1;
                };
                const bool same = l.kind == OperandKind::Temporary && r.kind == OperandKind::Temporary && l.id == r.id;
                Operand replacement;
                switch (instruction.opcode) {
                case Opcode::Add:
                    if (is(r, 0)) replacement = l;
                    else if (is(l, 0)) replacement = r;
                    break;
                case Opcode::BitXor:
                    if (is(r, 0)) replacement = l;
                    else if (is(l, 0)) replacement = r;
                    else if (same) replacement = constant(0);
                    break;
                case Opcode::BitOr:
                    if (is(r, 0)) replacement = l;
                    else if (is(l, 0)) replacement = r;
                    else if (is(r, all) || is(l, all)) replacement = constant(all);
                    break;
                case Opcode::Subtract:
                    if (IsIntegerZero(r)) replacement = l;
                    else if (same) replacement = constant(0);
                    break;
                case Opcode::ShiftLeft:
                case Opcode::ShiftRight:
                    if (IsIntegerZero(r)) replacement = l;
                    break;
                case Opcode::Multiply: {
                    // x * 2^k == x << k modulo 2^width for signed and unsigned alike.
                    const auto k = exponent(r) > 0 ? exponent(r) : exponent(l);
                    if (is(r, 1)) replacement = l;
                    else if (is(l, 1)) replacement = r;
                    else if (is(r, 0) || is(l, 0)) replacement = constant(0);
                    else if (k > 0) {
                        const auto x = exponent(r) > 0 ? l : r;
                        instruction.opcode = Opcode::ShiftLeft;
                        instruction.left = x;
                        instruction.right = constant(k);
                    }
                    break;
                }
                case Opcode::Divide:
                    // Only unsigned: signed division rounds toward zero, a shift toward minus infinity.
                    if (is(r, 1)) replacement = l;
                    else if (!type.isSigned && exponent(r) > 0) {
                        instruction.opcode = Opcode::ShiftRight;
                        instruction.right = constant(exponent(r));
                    }
                    break;
                case Opcode::Remainder:
                    if (is(r, 1)) replacement = constant(0);
                    else if (!type.isSigned && exponent(r) > 0) {
                        instruction.opcode = Opcode::BitAnd;
                        instruction.right = constant((r.integer & all) - 1);
                    }
                    break;
                case Opcode::BitAnd:
                    if (is(r, all)) replacement = l;
                    else if (is(l, all)) replacement = r;
                    else if (is(r, 0) || is(l, 0)) replacement = constant(0);
                    break;
                default:
                    break;
                }
                if (replacement && replacement.type == resultType) {
                    instruction.opcode = Opcode::Copy;
                    instruction.left = replacement;
                    instruction.right = {};
                }
            }

            bool Commutative(Opcode opcode) {
                return opcode == Opcode::Add || opcode == Opcode::Multiply || opcode == Opcode::BitAnd ||
                       opcode == Opcode::BitOr || opcode == Opcode::BitXor || opcode == Opcode::Equal ||
                       opcode == Opcode::NotEqual;
            }

            // Local value numbering. Pure operations are matched on opcode and operand identity
            // (commutative operands in either order, except for floating point). Reusing an earlier
            // division/remainder is safe: if it faulted, the later copy is never reached.
            void ValueNumber(const Module& module, Block& block) {
                std::vector<Instruction> available;
                for (auto& instruction : block.instructions) {
                    if (!DefinesTemporary(instruction)) continue;
                    const auto id = instruction.result.id;
                    std::erase_if(available, [&](const Instruction& prior) {
                        return prior.result.id == id ||
                               (prior.left.kind == OperandKind::Temporary && prior.left.id == id) ||
                               (prior.right.kind == OperandKind::Temporary && prior.right.id == id);
                    });
                    const auto opcode = instruction.opcode;
                    if (opcode != Opcode::Cast && opcode != Opcode::AddressOffset &&
                        (opcode < Opcode::Add || opcode > Opcode::LogicalNot)) continue;
                    const bool swap = Commutative(opcode) && module.type(instruction.left.type).kind != TypeKind::Floating;
                    const auto found = std::find_if(available.begin(), available.end(), [&](const Instruction& prior) {
                        return prior.opcode == opcode && prior.result.type == instruction.result.type &&
                               ((SameOperand(prior.left, instruction.left) && SameOperand(prior.right, instruction.right)) ||
                                (swap && SameOperand(prior.left, instruction.right) &&
                                 SameOperand(prior.right, instruction.left)));
                    });
                    if (found != available.end()) {
                        instruction.opcode = Opcode::Copy;
                        instruction.left = found->result;
                        instruction.right = {};
                    } else if (!(instruction.left.kind == OperandKind::Temporary && instruction.left.id == id) &&
                               !(instruction.right.kind == OperandKind::Temporary && instruction.right.id == id)) {
                        available.push_back(instruction);
                    }
                }
            }

            // Tracks which operand currently holds the contents of an address operand. Distinct
            // objects never overlap, so a store to one object only forgets that object's facts and
            // those reached through temporaries; a store through a temporary, a call, bulk zero or
            // destructor registration forgets everything. Redefining a temporary forgets facts
            // naming it as address or value.
            // Records are snapshots, so forwarding a record value through Copy is exact.
            void ForwardMemory(Block& block) {
                std::vector<std::pair<Operand, Operand>> known;
                for (auto& instruction : block.instructions) {
                    // Captured before a load may be rewritten into a copy of the forwarded value.
                    const auto opcode = instruction.opcode;
                    const auto address = instruction.left;
                    if (opcode == Opcode::Call || opcode == Opcode::ZeroMemory || opcode == Opcode::RegisterDestructor) {
                        known.clear();
                    } else if (opcode == Opcode::Store) {
                        const auto target = instruction.left;
                        std::erase_if(known, [&](const auto& fact) {
                            return target.kind != OperandKind::Object || fact.first.kind != OperandKind::Object ||
                                   fact.first.id == target.id;
                        });
                        known.emplace_back(instruction.left, instruction.right);
                    } else if (opcode == Opcode::Load) {
                        const auto found = std::find_if(known.begin(), known.end(), [&](const auto& fact) {
                            return SameOperand(fact.first, address) && fact.second.type == instruction.result.type;
                        });
                        if (found != known.end()) {
                            instruction.opcode = Opcode::Copy;
                            instruction.left = found->second;
                            instruction.right = {};
                        }
                    }
                    if (!DefinesTemporary(instruction) || opcode == Opcode::Call) continue;
                    const auto id = instruction.result.id;
                    std::erase_if(known, [&](const auto& fact) {
                        return (fact.first.kind == OperandKind::Temporary && fact.first.id == id) ||
                               (fact.second.kind == OperandKind::Temporary && fact.second.id == id);
                    });
                    if (opcode == Opcode::Load && !(address.kind == OperandKind::Temporary && address.id == id))
                        known.emplace_back(address, instruction.result);
                }
            }

            // Backward scan: a store to a named object is dead when the identical address is stored
            // again before anything could read it. A load of another named object cannot read it;
            // a load through a temporary might. Stores through temporaries are never removed.
            void EliminateDeadStores(Block& block) {
                std::vector<Operand> overwritten;
                std::vector<bool> dead(block.instructions.size());
                for (size_t i = block.instructions.size(); i-- > 0;) {
                    const auto& instruction = block.instructions[i];
                    switch (instruction.opcode) {
                    case Opcode::Load:
                        std::erase_if(overwritten, [&](const Operand& later) {
                            return instruction.left.kind != OperandKind::Object || later.id == instruction.left.id;
                        });
                        break;
                    case Opcode::Call:
                    case Opcode::Param:
                    case Opcode::ZeroMemory:
                    case Opcode::RegisterDestructor:
                        overwritten.clear();
                        break;
                    case Opcode::Store:
                        if (instruction.left.kind != OperandKind::Object) break;
                        if (std::any_of(overwritten.begin(), overwritten.end(),
                                        [&](const Operand& later) { return SameOperand(later, instruction.left); }))
                            dead[i] = true;
                        else
                            overwritten.push_back(instruction.left);
                        break;
                    default:
                        break;
                    }
                }
                size_t index = 0;
                std::erase_if(block.instructions, [&](const Instruction&) { return dead[index++]; });
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
                // Every remaining pass looks at one basic block at a time.
                for (auto& function : module.functions) {
                    for (auto& block : function.blocks) {
                        for (auto& instruction : block.instructions) SimplifyIdentity(module, instruction);
                        ValueNumber(module, block);
                        ForwardMemory(block);
                        EliminateDeadStores(block);
                    }
                    SimplifyBranches(function);
                }
                RemoveUnreachableBlocks(module);
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
