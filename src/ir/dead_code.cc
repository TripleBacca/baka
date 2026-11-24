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

            // Temporaries live on exit from each block (global analysis). Temporaries
            // are mutable, so any definition kills and any earlier read is exposed.
            LiveSets TemporariesLiveOut(Function& function) {
                BuildCFG(function);
                LiveSets uses, defs;
                for (const auto& block : function.blocks) {
                    auto& read = uses[block.id];
                    auto& written = defs[block.id];
                    for (const auto& instruction : block.instructions) {
                        for (const auto& operand : Uses(instruction)) {
                            if (operand.kind == OperandKind::Temporary && !written.count(operand.id)) {
                                read.insert(operand.id);
                            }
                        }
                        if (DefinesTemporary(instruction)) written.insert(instruction.result.id);
                    }
                }
                return LiveOut(function, uses, defs);
            }
        }

        void RemoveUnreachableBlocks(Module& module) {
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
                            pending.insert(pending.end(), block.successors.begin(), block.successors.end());
                        }
                    }
                }
                std::erase_if(function.blocks, [&](const Block& block) { return !reachable.count(block.id); });
                BuildCFG(function);
            }
        }

        // Removes unused temporary definitions within each block using global live-out
        // information. Keeps instructions that can fault or have other side effects.
        void EliminateDeadCode(Module& module) {
            for (auto& function : module.functions) {
                if (!function.defined || function.blocks.empty()) {
                    continue;
                }
                auto exits = TemporariesLiveOut(function);
                for (auto& block : function.blocks) {
                    // Backward next-use scan from the block's global live-out set; the
                    // deletion itself is local to this block.
                    const auto& exit = exits[block.id];
                    std::set<uint32_t> live(exit.begin(), exit.end());
                    std::vector<Instruction> kept;
                    for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
                        if (DefinesTemporary(*it)) {
                            const auto id = it->result.id;
                            if (!live.count(id) && Removable(it->opcode)) {
                                continue;
                            }
                            live.erase(id);
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
