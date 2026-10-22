#include "ir.hh"
#include <algorithm>
#include <map>
#include <set>

namespace baka {
    namespace ir {
        namespace {
            bool Removable(Opcode opcode) {
                // Preserve potentially faulting loads/divisions and all memory/call/control effects.
                return opcode == Opcode::Copy || opcode == Opcode::Cast || opcode == Opcode::AddressOffset ||
                       (opcode >= Opcode::Add && opcode <= Opcode::LogicalNot && opcode != Opcode::Divide &&
                        opcode != Opcode::Remainder);
            }
        }

        void EliminateDeadCode(Module& module) {
            for (auto& function : module.functions) {
                if (!function.defined || function.blocks.empty()) {
                    continue;
                }
                BuildCFG(function);
                std::set<BlockId> reachable;
                std::vector<BlockId> pending{function.blocks.front().id};
                while (!pending.empty()) {
                    auto id = pending.back();
                    pending.pop_back();
                    if (!reachable.insert(id).second) {
                        continue;
                    }
                    for (const auto& block : function.blocks) {
                        if (block.id == id) {
                            for (auto successor : block.successors) {
                                pending.push_back(successor);
                            }
                        }
                    }
                }
                std::erase_if(function.blocks, [&](const Block& block) { return !reachable.count(block.id); });
                BuildCFG(function);
                std::map<BlockId, std::set<uint32_t>> liveIn, liveOut;
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (auto it = function.blocks.rbegin(); it != function.blocks.rend(); ++it) {
                        std::set<uint32_t> out;
                        for (auto successor : it->successors) {
                            out.insert(liveIn[successor].begin(), liveIn[successor].end());
                        }
                        auto in = out;
                        for (auto instruction = it->instructions.rbegin(); instruction != it->instructions.rend();
                             ++instruction) {
                            if (DefinesTemporary(*instruction)) {
                                in.erase(instruction->result.id);
                            }
                            for (auto operand : Uses(*instruction)) {
                                if (operand.kind == OperandKind::Temporary) {
                                    in.insert(operand.id);
                                }
                            }
                        }
                        if (liveIn[it->id] != in || liveOut[it->id] != out) {
                            changed = true;
                        }
                        liveIn[it->id] = std::move(in);
                        liveOut[it->id] = std::move(out);
                    }
                }
                // Repeat local deletion to remove newly dead chains, using interblock
                // liveness each time through Optimize. Stores are never guessed dead.
                for (auto& block : function.blocks) {
                    auto live = liveOut[block.id];
                    std::vector<Instruction> kept;
                    for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
                        if (DefinesTemporary(*it) && !live.count(it->result.id) && Removable(it->opcode)) {
                            continue;
                        }
                        if (DefinesTemporary(*it)) {
                            live.erase(it->result.id);
                        }
                        for (auto operand : Uses(*it)) {
                            if (operand.kind == OperandKind::Temporary) {
                                live.insert(operand.id);
                            }
                        }
                        kept.push_back(*it);
                    }
                    std::reverse(kept.begin(), kept.end());
                    block.instructions = std::move(kept);
                }
            }
        }

    }
}
