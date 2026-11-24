#include "ir.hh"
#include <algorithm>
#include <map>
#include <set>

namespace baka {
    namespace ir {
        namespace {
            // A callee is inlined when it never needs more than this many values in
            // registers at once (its peak register pressure).
            constexpr size_t InlineRegisterLimit = 6;
            // Inlining into a function that itself was just expanded may expose new
            // small call sites; a few rounds catch nested helpers without unbounded growth.
            constexpr size_t InlineRounds = 3;

            // A liveness name: a temporary, or a register-resident object tagged in the
            // high word (the same scheme the MIPS selector uses for its descriptors).
            using Name = uint64_t;
            constexpr Name ObjectTag = uint64_t{1} << 32;

            // Scalar locals and parameters whose address is only ever used directly by
            // a whole-value load or store. These are the objects the backend keeps in
            // registers; everything else lives in stack memory and costs no register.
            std::set<uint32_t> RegisterObjects(const Module& module, const Function& function) {
                std::set<uint32_t> result;
                for (const auto& object : function.objects) {
                    auto kind = module.type(object.type).kind;
                    if (kind == TypeKind::Integer || kind == TypeKind::Floating || kind == TypeKind::Pointer) {
                        result.insert(object.id);
                    }
                }
                for (const auto& block : function.blocks) {
                    for (const auto& instruction : block.instructions) {
                        auto check = [&](const Operand& operand, bool directAddress) {
                            if (operand.kind == OperandKind::Object && (!directAddress || operand.integer)) {
                                result.erase(operand.id);
                            }
                        };
                        bool memory = instruction.opcode == Opcode::Load || instruction.opcode == Opcode::Store;
                        check(instruction.left, memory);
                        check(instruction.right, false);
                    }
                }
                return result;
            }

            struct Effects {
                std::vector<Name> uses, defs;
            };

            Effects NamesOf(const Instruction& instruction, const std::set<uint32_t>& registers) {
                Effects effects;
                auto use = [&](const Operand& operand) {
                    if (operand.kind == OperandKind::Temporary) {
                        effects.uses.push_back(operand.id);
                    }
                };
                if (instruction.opcode == Opcode::Load && instruction.left.kind == OperandKind::Object &&
                    registers.count(instruction.left.id)) {
                    effects.uses.push_back(ObjectTag | instruction.left.id);
                } else {
                    use(instruction.left);
                }
                use(instruction.right);
                if (DefinesTemporary(instruction)) {
                    effects.defs.push_back(instruction.result.id);
                }
                if (instruction.opcode == Opcode::Store && instruction.left.kind == OperandKind::Object &&
                    registers.count(instruction.left.id)) {
                    effects.defs.push_back(ObjectTag | instruction.left.id);
                }
                return effects;
            }

            // The most values the function ever needs in registers at the same time: walk
            // each block backward from its live-out set, counting at every instruction the
            // values live after it plus whatever it defines. Requires current CFG edges.
            size_t PeakLive(const Module& module, const Function& function) {
                const auto registers = RegisterObjects(module, function);
                LiveSets uses, defs;
                for (const auto& block : function.blocks) {
                    std::set<Name> exposed, written;
                    for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
                        auto effects = NamesOf(*it, registers);
                        for (auto name : effects.defs) {
                            exposed.erase(name);
                            written.insert(name);
                        }
                        exposed.insert(effects.uses.begin(), effects.uses.end());
                    }
                    uses[block.id] = std::move(exposed);
                    defs[block.id] = std::move(written);
                }
                const auto out = LiveOut(function, uses, defs);
                size_t peak = 0;
                for (const auto& block : function.blocks) {
                    auto found = out.find(block.id);
                    std::set<Name> live = found == out.end() ? std::set<Name>{} : found->second;
                    peak = std::max(peak, live.size());
                    for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
                        auto effects = NamesOf(*it, registers);
                        auto busy = live;
                        busy.insert(effects.defs.begin(), effects.defs.end());
                        peak = std::max(peak, busy.size());
                        for (auto name : effects.defs) live.erase(name);
                        live.insert(effects.uses.begin(), effects.uses.end());
                        peak = std::max(peak, live.size());
                    }
                }
                return peak;
            }

            bool Calls(const Function& function, uint32_t callee) {
                for (const auto& block : function.blocks)
                    for (const auto& instruction : block.instructions)
                        if (instruction.opcode == Opcode::Call && instruction.left.kind == OperandKind::Function &&
                            instruction.left.id == callee)
                            return true;
                return false;
            }

            // Defined, fixed-arity, not directly recursive, not the module initializer,
            // no static-destructor registration, and small enough register pressure.
            bool Inlinable(const Module& module, const Function& function) {
                if (!function.defined || function.blocks.empty() || function.id == module.initializer ||
                    module.type(function.signature).variadic || Calls(function, function.id)) {
                    return false;
                }
                for (const auto& block : function.blocks)
                    for (const auto& instruction : block.instructions)
                        if (instruction.opcode == Opcode::RegisterDestructor) return false;
                return PeakLive(module, function) <= InlineRegisterLimit;
            }

            uint32_t NextObjectId(const Module& module) {
                uint32_t next = 0;
                for (const auto& global : module.globals) next = std::max(next, global.id);
                for (const auto& function : module.functions) {
                    next = std::max(next, function.id);
                    for (const auto& object : function.objects) next = std::max(next, object.id);
                }
                return next + 1;
            }

            class Inliner {
            public:
                Inliner(Module& module, Function& caller, uint32_t& nextObject)
                    : M(module), Caller(caller), NextObject(nextObject) {
                    for (const auto& block : caller.blocks) NextBlock = std::max(NextBlock, block.id + 1);
                }

                // Replaces the call at caller.blocks[blockIndex].instructions[callIndex] (with
                // its Param run) by a copy of `callee`. The block is split: the head stores
                // each argument into the copied parameter object and jumps to the copied
                // entry; every copied return writes the call's result and jumps to a new
                // continuation block holding the instructions that followed the call.
                void expand(size_t blockIndex, size_t callIndex, const Function& callee) {
                    auto& block = Caller.blocks[blockIndex];
                    const auto call = block.instructions[callIndex];
                    const size_t first = callIndex - callee.parameters.size();
                    std::vector<Operand> arguments;
                    for (size_t k = first; k < callIndex; ++k) arguments.push_back(block.instructions[k].left);
                    std::vector<Instruction> tail(block.instructions.begin() + callIndex + 1, block.instructions.end());
                    block.instructions.resize(first);

                    Objects.clear();
                    Temporaries.clear();
                    Blocks.clear();
                    for (const auto& object : callee.objects) {
                        auto copy = object;
                        copy.id = NextObject++;
                        Objects[object.id] = copy.id;
                        Caller.objects.push_back(std::move(copy));
                    }
                    for (uint32_t t = 1; t < callee.temporaries.size(); ++t) {
                        Temporaries[t] = static_cast<uint32_t>(Caller.temporaries.size());
                        Caller.temporaries.push_back(callee.temporaries[t]);
                    }
                    const BlockId continuation = NextBlock++;
                    for (const auto& source : callee.blocks) Blocks[source.id] = NextBlock++;

                    for (size_t k = 0; k < arguments.size(); ++k) {
                        const auto& parameter = *std::find_if(
                            callee.objects.begin(), callee.objects.end(),
                            [&](const Object& object) { return object.id == callee.parameters[k]; });
                        Operand address{OperandKind::Object, M.pointer(parameter.type), Objects.at(parameter.id)};
                        block.instructions.push_back(Instruction::store(address, arguments[k], call.source));
                    }
                    block.instructions.push_back(
                        {Opcode::Jump, {}, label(Blocks.at(callee.blocks.front().id)), {}, call.source});

                    std::vector<Block> body;
                    for (const auto& source : callee.blocks) {
                        Block copy;
                        copy.id = Blocks.at(source.id);
                        for (auto instruction : source.instructions) {
                            remap(instruction.result);
                            remap(instruction.left);
                            remap(instruction.right);
                            if (instruction.opcode == Opcode::Return) {
                                if (call.result && instruction.left) {
                                    copy.instructions.push_back(
                                        {Opcode::Copy, call.result, instruction.left, {}, instruction.source});
                                }
                                instruction = {Opcode::Jump, {}, label(continuation), {}, instruction.source};
                            }
                            copy.instructions.push_back(std::move(instruction));
                        }
                        body.push_back(std::move(copy));
                    }
                    Block rest;
                    rest.id = continuation;
                    rest.instructions = std::move(tail);
                    body.push_back(std::move(rest));
                    Caller.blocks.insert(Caller.blocks.begin() + static_cast<std::ptrdiff_t>(blockIndex) + 1,
                                         std::make_move_iterator(body.begin()), std::make_move_iterator(body.end()));
                }

            private:
                static Operand label(BlockId id) {
                    return {OperandKind::Block, 0, id};
                }

                void remap(Operand& operand) const {
                    if (operand.kind == OperandKind::Temporary) {
                        operand.id = Temporaries.at(operand.id);
                    } else if (operand.kind == OperandKind::Block) {
                        operand.id = Blocks.at(operand.id);
                    } else if (operand.kind == OperandKind::Object) {
                        if (auto found = Objects.find(operand.id); found != Objects.end()) {
                            operand.id = found->second; // globals keep their id
                        }
                    }
                }

                Module& M;
                Function& Caller;
                uint32_t& NextObject;
                BlockId NextBlock = 1;
                std::map<uint32_t, uint32_t> Objects, Temporaries;
                std::map<BlockId, BlockId> Blocks;
            };
        }

        // Inlines direct calls to small functions (peak register pressure at most
        // InlineRegisterLimit) into their callers. Callee bodies are snapshotted at the
        // start of each round, so a function never expands into itself mid-round, and
        // directly recursive functions are never inlined. Callees stay defined: their
        // address may still be taken (function pointers, vtables). Returns whether
        // anything changed; CFG edges are rebuilt for every modified caller.
        bool Inline(Module& module) {
            for (auto& function : module.functions) BuildCFG(function);
            bool changed = false;
            uint32_t nextObject = NextObjectId(module);
            for (size_t round = 0; round < InlineRounds; ++round) {
                std::map<uint32_t, Function> callees;
                for (const auto& function : module.functions) {
                    if (Inlinable(module, function)) callees.emplace(function.id, function);
                }
                bool roundChanged = false;
                for (auto& caller : module.functions) {
                    Inliner inliner(module, caller, nextObject);
                    bool modified = false;
                    for (size_t b = 0; b < caller.blocks.size(); ++b) {
                        for (size_t i = 0; i < caller.blocks[b].instructions.size(); ++i) {
                            const auto& instruction = caller.blocks[b].instructions[i];
                            if (instruction.opcode != Opcode::Call || instruction.left.kind != OperandKind::Function ||
                                instruction.left.id == caller.id) {
                                continue;
                            }
                            auto found = callees.find(instruction.left.id);
                            if (found == callees.end() || i < found->second.parameters.size()) {
                                continue;
                            }
                            bool arguments = true;
                            for (size_t k = i - found->second.parameters.size(); k < i; ++k) {
                                arguments = arguments && caller.blocks[b].instructions[k].opcode == Opcode::Param;
                            }
                            if (!arguments) {
                                continue;
                            }
                            // The rest of this block moves to a continuation block after
                            // the inlined body. Skip the copied body (its calls wait for the
                            // next round, so mutually calling helpers cannot expand forever)
                            // and resume the scan at the continuation.
                            inliner.expand(b, i, found->second);
                            b += found->second.blocks.size();
                            modified = true;
                            break;
                        }
                    }
                    if (modified) {
                        BuildCFG(caller);
                        roundChanged = true;
                    }
                }
                module.invalidateIndexes();
                changed = changed || roundChanged;
                if (!roundChanged) break;
            }
            return changed;
        }
    }
}
