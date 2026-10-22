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
                    edge(end.left);
                }
                if (end.opcode == Opcode::Branch) {
                    edge(end.right);
                    edge(end.result);
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
    }
}
