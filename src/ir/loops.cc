#include "ir.hh"
#include <algorithm>

namespace baka {
    namespace ir {
        namespace {
            // Visit reachable blocks in reverse postorder so forward predecessors usually
            // have an immediate dominator by the time a block is first considered.
            std::vector<const Block*> ReversePostorder(const Function& function) {
                std::unordered_map<BlockId, const Block*> blocks;
                for (const auto& block : function.blocks) blocks.emplace(block.id, &block);
                const auto* entry = &function.blocks.front();
                std::set<BlockId> visited{entry->id};
                std::vector<std::pair<const Block*, size_t>> pending{{entry, 0}};
                std::vector<const Block*> order;
                while (!pending.empty()) {
                    auto& [block, next] = pending.back();
                    if (next == block->successors.size()) {
                        order.push_back(block);
                        pending.pop_back();
                    } else {
                        const auto successor = block->successors[next++];
                        if (visited.insert(successor).second) pending.emplace_back(blocks.at(successor), 0);
                    }
                }
                std::reverse(order.begin(), order.end());
                return order;
            }

            using BlockRanks = std::unordered_map<BlockId, size_t>;

            // Intersect immediate-dominator chains until all reachable blocks have stable
            // parents. Each parent precedes its child in reverse postorder.
            std::vector<size_t> ImmediateDominators(const std::vector<const Block*>& order, const BlockRanks& ranks) {
                const size_t unknown = order.size();
                std::vector<size_t> parent(order.size(), unknown);
                parent.front() = 0;
                bool changed;
                do {
                    changed = false;
                    for (size_t i = 1; i < order.size(); ++i) {
                        size_t common = unknown;
                        for (auto predecessor : order[i]->predecessors) {
                            auto rank = ranks.find(predecessor);
                            if (rank == ranks.end() || parent[rank->second] == unknown) continue;
                            size_t ancestor = rank->second;
                            if (common == unknown) {
                                common = ancestor;
                            } else {
                                while (ancestor != common) {
                                    if (ancestor > common) ancestor = parent[ancestor];
                                    else common = parent[common];
                                }
                            }
                        }
                        if (parent[i] != common) {
                            parent[i] = common;
                            changed = true;
                        }
                    }
                } while (changed);
                return parent;
            }

            void AddBackEdge(NaturalLoop& loop, size_t latch, const std::vector<const Block*>& order,
                             const BlockRanks& ranks) {
                loop.latches.push_back(order[latch]->id);
                if (!loop.blocks.insert(order[latch]->id).second) return;
                // Stop walking predecessors at the header, which is already in the set.
                std::vector<size_t> pending{latch};
                while (!pending.empty()) {
                    const auto* block = order[pending.back()];
                    pending.pop_back();
                    for (auto predecessor : block->predecessors) {
                        auto rank = ranks.find(predecessor);
                        if (rank != ranks.end() && loop.blocks.insert(predecessor).second) {
                            pending.push_back(rank->second);
                        }
                    }
                }
            }
        }

        // An edge is a natural-loop back edge when its destination dominates its source.
        // Combine all back edges to the same header, retaining their separate latches.
        std::vector<NaturalLoop> FindNaturalLoops(const Function& function) {
            if (function.blocks.empty()) return {};
            const auto order = ReversePostorder(function);
            BlockRanks ranks;
            for (size_t i = 0; i < order.size(); ++i) ranks.emplace(order[i]->id, i);
            const auto parent = ImmediateDominators(order, ranks);
            std::map<BlockId, NaturalLoop> loops;
            for (size_t latch = 0; latch < order.size(); ++latch) {
                for (auto successor : order[latch]->successors) {
                    const auto header = ranks.at(successor);
                    auto ancestor = latch;
                    while (ancestor > header) ancestor = parent[ancestor];
                    if (ancestor != header) continue;
                    auto& loop = loops.try_emplace(successor, NaturalLoop{successor, {successor}, {}}).first->second;
                    AddBackEdge(loop, latch, order, ranks);
                }
            }
            std::vector<NaturalLoop> result;
            result.reserve(loops.size());
            for (auto& [header, loop] : loops) {
                std::sort(loop.latches.begin(), loop.latches.end());
                result.push_back(std::move(loop));
            }
            return result;
        }
    }
}
