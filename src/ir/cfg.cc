#include "ir.hh"
#include <algorithm>
#include <unordered_map>

namespace baka {
    namespace ir {
        void BuildCFG(Function& function) {
            std::unordered_map<BlockId, Block*> blocks;
            for (auto& block : function.blocks) {
                block.predecessors.clear();
                block.successors.clear();
                blocks.emplace(block.id, &block);
            }
            for (auto& block : function.blocks) {
                if (block.instructions.empty()) {
                    continue;
                }
                const auto& end = block.instructions.back();
                auto edge = [&](Operand label) {
                    if (label.kind != OperandKind::Block) {
                        return;
                    }
                    if (std::find(block.successors.begin(), block.successors.end(), label.id) ==
                        block.successors.end()) {
                        block.successors.push_back(label.id);
                    }
                    auto found = blocks.find(label.id);
                    if (found != blocks.end()) {
                        found->second->predecessors.push_back(block.id);
                    }
                };
                if (end.opcode == Opcode::Jump) {
                    if (block.instructions.size() >= 2) {
                        const auto& conditional = block.instructions[block.instructions.size() - 2];
                        if (conditional.opcode == Opcode::JumpIf) {
                            edge(conditional.right);
                        }
                    }
                    edge(end.left);
                }
            }
            for (auto& block : function.blocks) {
                std::sort(block.predecessors.begin(), block.predecessors.end());
                block.predecessors.erase(std::unique(block.predecessors.begin(), block.predecessors.end()),
                                         block.predecessors.end());
            }
        }

        std::vector<FlatInstruction> Linearize(const Function& function) {
            std::vector<FlatInstruction> result;
            for (const auto& block : function.blocks) {
                for (const auto& instruction : block.instructions) {
                    result.push_back({block.id, instruction});
                }
            }
            return result;
        }
    
        LiveSets LiveOut(const Function& function, const LiveSets& uses, const LiveSets& defs) {
            static const std::set<uint64_t> none;
            auto lookup = [](const LiveSets& sets, BlockId id) -> const std::set<uint64_t>& {
                auto found = sets.find(id);
                return found == sets.end() ? none : found->second;
            };
            LiveSets in, out;
            bool changed = true;
            while (changed) {
                changed = false;
                // Reverse block order converges quickly for this backward problem.
                for (auto block = function.blocks.rbegin(); block != function.blocks.rend(); ++block) {
                    std::set<uint64_t> live;
                    for (auto successor : block->successors) {
                        const auto& next = lookup(in, successor);
                        live.insert(next.begin(), next.end());
                    }
                    auto entry = live;
                    for (auto name : lookup(defs, block->id)) entry.erase(name);
                    const auto& read = lookup(uses, block->id);
                    entry.insert(read.begin(), read.end());
                    if (entry != lookup(in, block->id) || live != lookup(out, block->id)) {
                        in[block->id] = std::move(entry);
                        out[block->id] = std::move(live);
                        changed = true;
                    }
                }
            }
            return out;
        }
    }
}
