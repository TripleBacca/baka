#include "ir.hh"
#include "base/target.hh"
#include <algorithm>

namespace baka {
    namespace ir {
        namespace {
            constexpr uint64_t MinIterations = 2, MaxIterations = 4;
            constexpr size_t MaxBodyInstructions = 16;

            using Blocks = std::unordered_map<BlockId, Block*>;

            struct Candidate {
                const Block* body;
                Operand counter;
                Operand bound;
            };

            bool IntegerEquals(const Operand& operand, TypeId type, uint64_t value) {
                return operand.kind == OperandKind::Integer && operand.type == type && operand.integer == value;
            }

            bool JumpsOnlyTo(const Block& block, BlockId target) {
                return block.successors.size() == 1 && block.successors.front() == target &&
                       block.instructions.back().opcode == Opcode::Jump &&
                       (block.instructions.size() == 1 ||
                        block.instructions[block.instructions.size() - 2].opcode != Opcode::JumpIf);
            }

            // Match a zero-initialized local int, an exact `i < N` test and an exact
            // increment. Keep the original exit so any cleanup there still runs.
            std::optional<Candidate> MatchLoop(const Module& module, const Function& function,
                                               const NaturalLoop& loop, const Blocks& blocks) {
                if (loop.blocks.size() != 3 || loop.latches.size() != 1) return {};
                const auto& header = *blocks.at(loop.header);
                const auto& latch = *blocks.at(loop.latches.front());
                const auto& test = header.instructions;
                if (header.predecessors.size() != 2 || header.successors.size() != 2 || test.size() != 4 ||
                    test[0].opcode != Opcode::Load || test[1].opcode != Opcode::Less ||
                    test[1].left != test[0].result || test[2].opcode != Opcode::JumpIf ||
                    test[2].left != test[1].result || test[3].opcode != Opcode::Jump) return {};

                const auto bodyId = test[2].right.id;
                if (bodyId == header.id || bodyId == latch.id || !loop.blocks.contains(bodyId) ||
                    loop.blocks.contains(test[3].left.id)) return {};
                const auto& body = *blocks.at(bodyId);
                if (body.predecessors.size() != 1 || body.predecessors.front() != header.id ||
                    latch.predecessors.size() != 1 || latch.predecessors.front() != bodyId ||
                    !JumpsOnlyTo(body, latch.id) || !JumpsOnlyTo(latch, header.id) ||
                    body.instructions.size() - 1 > MaxBodyInstructions) return {};

                const auto counter = test[0].left;
                const auto type = test[0].result.type;
                const auto& counterType = module.type(type);
                if (counter.kind != OperandKind::Object || counter.integer != 0 ||
                    counterType.kind != TypeKind::Integer || !counterType.isSigned ||
                    counterType.bits != target::IntBits) return {};
                const auto* object = module.object(function, counter.id);
                if (!object || object->global || object->external || object->readOnly || object->type != type) return {};
                const auto bound = test[1].right;
                if (bound.kind != OperandKind::Integer || bound.type != type ||
                    bound.integer < MinIterations || bound.integer > MaxIterations) return {};

                const auto beforeId = header.predecessors.front() == latch.id ? header.predecessors.back()
                                                                              : header.predecessors.front();
                const auto& before = *blocks.at(beforeId);
                if (loop.blocks.contains(beforeId) || !JumpsOnlyTo(before, header.id) ||
                    before.instructions.size() < 2) return {};
                // An assignment initializer (`i = 0`) also loads its result. Leave
                // those reads in place and find the store that precedes them.
                auto init = before.instructions.rbegin() + 1;
                while (init != before.instructions.rend() && init->opcode == Opcode::Load && init->left == counter) ++init;
                if (init == before.instructions.rend() || init->opcode != Opcode::Store || init->left != counter ||
                    !IntegerEquals(init->right, type, 0)) return {};

                const auto& step = latch.instructions;
                if (step.size() != 4 || step[0].opcode != Opcode::Load || step[0].left != counter ||
                    step[0].result.type != type || step[1].opcode != Opcode::Add || step[1].left != step[0].result ||
                    !IntegerEquals(step[1].right, type, 1) || step[2].opcode != Opcode::Store ||
                    step[2].left != counter || step[2].right != step[1].result) return {};
                return Candidate{&body, counter, bound};
            }

            // Check the entire function for address escapes and uses of loop temporaries.
            // Loop definitions must be local to one block and precede each read there.
            bool CanClone(const Function& function, const NaturalLoop& loop, const Blocks& blocks,
                          const Candidate& candidate) {
                std::map<uint32_t, BlockId> definitions;
                for (auto block : loop.blocks) {
                    for (const auto& in : blocks.at(block)->instructions) {
                        if (DefinesTemporary(in) && !definitions.emplace(in.result.id, block).second) return false;
                    }
                }
                auto isCounter = [&](const Operand& operand) {
                    return operand.kind == OperandKind::Object && operand.id == candidate.counter.id;
                };
                for (const auto& block : function.blocks) {
                    std::set<uint32_t> definedHere;
                    const bool body = block.id == candidate.body->id;
                    for (const auto& in : block.instructions) {
                        if (body && (in.opcode == Opcode::Call || in.opcode == Opcode::Param ||
                                     in.opcode == Opcode::RegisterDestructor)) return false;
                        if (isCounter(in.right)) return false;
                        if (isCounter(in.left)) {
                            const bool load = in.opcode == Opcode::Load && in.result.type == candidate.bound.type;
                            const bool store = !body && in.opcode == Opcode::Store && in.right.type == candidate.bound.type;
                            if (in.left != candidate.counter || !(load || store)) return false;
                        }
                        for (const auto* operand : {&in.left, &in.right}) {
                            if (operand->kind == OperandKind::Temporary && definitions.contains(operand->id) &&
                                !definedHere.contains(operand->id)) return false;
                        }
                        if (DefinesTemporary(in)) {
                            auto found = definitions.find(in.result.id);
                            if (found != definitions.end()) {
                                if (found->second != block.id) return false;
                                definedHere.insert(in.result.id);
                            }
                        }
                    }
                }
                return true;
            }

            void ReplaceLoop(Function& function, Block& header, const Candidate& candidate) {
                const auto& body = candidate.body->instructions;
                std::vector<Instruction> replacement;
                replacement.reserve(candidate.bound.integer * (body.size() - 1) + 2);
                auto index = candidate.bound;
                for (index.integer = 0; index.integer < candidate.bound.integer; ++index.integer) {
                    std::map<uint32_t, uint32_t> copies;
                    auto remap = [&](Operand& operand) {
                        if (operand.kind != OperandKind::Temporary) return;
                        if (auto found = copies.find(operand.id); found != copies.end()) operand.id = found->second;
                    };
                    for (auto it = body.begin(); it != body.end() - 1; ++it) {
                        auto in = *it;
                        remap(in.left);
                        remap(in.right);
                        if (in.opcode == Opcode::Load && in.left == candidate.counter) {
                            in.opcode = Opcode::Copy;
                            in.left = index;
                        }
                        if (DefinesTemporary(in)) {
                            const auto id = static_cast<uint32_t>(function.temporaries.size());
                            copies.emplace(in.result.id, id);
                            function.temporaries.push_back(in.result.type);
                            in.result.id = id;
                        }
                        replacement.push_back(std::move(in));
                    }
                }
                // The exit can observe the final counter value even when the body never reads it.
                const auto exit = header.instructions.back();
                replacement.push_back(Instruction::store(candidate.counter, candidate.bound, exit.source));
                replacement.push_back(exit);
                header.instructions = std::move(replacement);
            }
        }

        void UnrollSmallLoops(Module& module) {
            for (auto& function : module.functions) {
                if (!function.defined || function.blocks.empty()) continue;
                const auto loops = FindNaturalLoops(function);
                if (loops.empty()) continue;
                Blocks blocks;
                for (auto& block : function.blocks) blocks.emplace(block.id, &block);
                std::set<BlockId> obsolete;
                for (const auto& loop : loops) {
                    auto candidate = MatchLoop(module, function, loop, blocks);
                    if (!candidate || !CanClone(function, loop, blocks, *candidate)) continue;
                    ReplaceLoop(function, *blocks.at(loop.header), *candidate);
                    obsolete.insert(candidate->body->id);
                    obsolete.insert(loop.latches.front());
                }
                if (obsolete.empty()) continue;
                std::erase_if(function.blocks, [&](const Block& block) { return obsolete.contains(block.id); });
                BuildCFG(function);
            }
        }
    }
}
