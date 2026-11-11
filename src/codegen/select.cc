#include "internal.hh"
#include <algorithm>
#include <bit>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace baka {
    namespace codegen {
        namespace {
            using ir::Opcode;
            using ir::OperandKind;
            using ir::TypeKind;

            // A descriptor name: a temporary, or a promoted scalar object (tagged in
            // the high word). Promoted objects play the slides' "user variables".
            using Name = uint64_t;
            constexpr Name ObjectTag = uint64_t{1} << 32;
            constexpr size_t Never = std::numeric_limits<size_t>::max();

            enum class Class { Int, Float };

            // Selection scratch: never allocated, valid only inside one instruction.
            constexpr Reg ScratchA = reg::V0, ScratchB = reg::V1, Address = reg::T9;
            const Reg FloatScratchA = reg::F(0), FloatScratchB = reg::F(1);

            const std::vector<Reg> IntCallerSaved{reg::T0, reg::T1, reg::T2, reg::T3, reg::T8};
            const std::vector<Reg> IntCalleeSaved{reg::S0, reg::S1, reg::S2, reg::S3,
                                                  reg::S4, reg::S5, reg::S6, reg::S7};

            std::vector<Reg> FloatRange(unsigned from, unsigned to) {
                std::vector<Reg> result;
                for (auto n = from; n <= to; ++n) result.push_back(reg::F(n));
                return result;
            }

            const std::vector<Reg> FloatCallerSaved = [] {
                auto low = FloatRange(2, 11), high = FloatRange(20, 23);
                low.insert(low.end(), high.begin(), high.end());
                return low;
            }();
            const std::vector<Reg> FloatCalleeSaved = FloatRange(24, 31);

            // Everything a call may overwrite under n64.
            const std::vector<Reg> CallClobbers = [] {
                std::vector<Reg> result{reg::At, reg::V0, reg::V1, reg::A0, reg::A1, reg::A2, reg::A3, reg::A4,
                                        reg::A5, reg::A6, reg::A7, reg::T0, reg::T1, reg::T2, reg::T3, reg::T8,
                                        reg::T9, reg::Ra, reg::HiLo, reg::Fcc};
                for (unsigned n = 0; n < 24; ++n) result.push_back(reg::F(n));
                return result;
            }();

            // Registers that may hold a value at an IR block label: the frame and
            // return address, plus callee-saved registers the epilogue restores.
            const std::vector<Reg> LiveAtBlockStart = [] {
                std::vector<Reg> result{reg::Sp, reg::Ra, reg::Gp, reg::Fp};
                for (Reg r = reg::S0; r <= reg::S7; ++r) result.push_back(r);
                for (unsigned n = 24; n < 32; ++n) result.push_back(reg::F(n));
                return result;
            }();

            bool FitsSigned16(int64_t value) {
                return value >= -32768 && value <= 32767;
            }

            int64_t AlignUp(int64_t value, int64_t alignment) {
                return (value + alignment - 1) / alignment * alignment;
            }

            class Selector {
                const ir::Module& Module;
                const ir::Function& Function;
                LiteralPool& Pool;
                MFunction Result;
                std::vector<MInst> Body;
                uint32_t LocalLabels = 0;

                // Frame: [0, Outgoing) outgoing stack arguments, then objects and
                // spill homes (Cursor grows), then saved registers at the top.
                int64_t Outgoing = 0, Cursor = 0, SretSlot = -1;
                std::map<uint32_t, int64_t> ObjectSlots, TempHomes;
                std::set<Reg> SavedRegisters;
                bool MakesCall = false;

                std::set<uint32_t> Promoted;     // non-escaping scalar objects
                // Global liveness (analysis only): names live on exit from each block.
                // Only these are written back when a block ends.
                ir::LiveSets BlockLiveOut;
                const std::set<uint64_t>* LiveOutHere = nullptr;

                // Per-block next-use information (slides, lecture 33): Next[i] maps
                // each name live after instruction i to its next use.
                std::vector<std::map<Name, size_t>> Next;
                std::vector<size_t> Calls;
                size_t BlockEnd = 0;

                // Register and address descriptors (slides, lectures 33-34).
                std::vector<std::vector<Name>> Holds = std::vector<std::vector<Name>>(reg::Count);

                struct Where {
                    std::vector<Reg> regs;
                    bool memory = false;
                };

                std::map<Name, Where> Locations;

            public:
                Selector(const ir::Module& module, const ir::Function& function, LiteralPool& pool)
                    : Module(module), Function(function), Pool(pool) {}

                MFunction run() {
                    Result.symbol = FunctionSymbol(Function);
                    Result.global = !Function.internal;
                    analyze();
                    layoutObjects();
                    for (const auto& block : Function.blocks) {
                        emitBlock(block);
                    }
                    finish();
                    return std::move(Result);
                }

            private:
                // ---- types ------------------------------------------------------

                const ir::Type& type(ir::TypeId id) const {
                    return Module.type(id);
                }

                bool isAggregate(ir::TypeId id) const {
                    const auto kind = type(id).kind;
                    return kind == TypeKind::Record || kind == TypeKind::Array;
                }

                bool isFloat(ir::TypeId id) const {
                    return type(id).kind == TypeKind::Floating;
                }

                Class classOf(ir::TypeId id) const {
                    return isFloat(id) ? Class::Float : Class::Int;
                }

                unsigned bits(ir::TypeId id) const {
                    const auto& t = type(id);
                    return t.kind == TypeKind::Integer || t.kind == TypeKind::Floating ? t.bits : 64;
                }

                bool isSigned(ir::TypeId id) const {
                    const auto& t = type(id);
                    return t.kind == TypeKind::Integer && t.isSigned;
                }

                bool isUnsigned32(ir::TypeId id) const {
                    const auto& t = type(id);
                    return t.kind == TypeKind::Integer && !t.isSigned && t.bits == 32;
                }

                const ir::Object& object(uint32_t id) const {
                    auto* found = Module.object(Function, id);
                    if (!found) throw Unsupported("unknown object " + std::to_string(id));
                    return *found;
                }

                ir::TypeId nameType(Name name) const {
                    return name & ObjectTag ? object(static_cast<uint32_t>(name)).type
                                            : Function.temporaries.at(static_cast<uint32_t>(name));
                }

                MOp loadOp(ir::TypeId id) const {
                    const auto width = bits(id);
                    if (isFloat(id)) return width == 32 ? MOp::Lwc1 : MOp::Ldc1;
                    const bool sign = isSigned(id);
                    if (width <= 8) return sign ? MOp::Lb : MOp::Lbu;
                    if (width <= 16) return sign ? MOp::Lh : MOp::Lhu;
                    if (width <= 32) return sign ? MOp::Lw : MOp::Lwu;
                    return MOp::Ld;
                }

                MOp storeOp(ir::TypeId id) const {
                    const auto width = bits(id);
                    if (isFloat(id)) return width == 32 ? MOp::Swc1 : MOp::Sdc1;
                    if (width <= 8) return MOp::Sb;
                    if (width <= 16) return MOp::Sh;
                    if (width <= 32) return MOp::Sw;
                    return MOp::Sd;
                }

                // Integer literals carry their low bits; registers hold every integer
                // extended to 64 bits by its own signedness (README: canonical form).
                int64_t canonical(const ir::Operand& operand) const {
                    const auto width = bits(operand.type);
                    if (width >= 64) return static_cast<int64_t>(operand.integer);
                    const uint64_t mask = (uint64_t{1} << width) - 1, value = operand.integer & mask;
                    if (isSigned(operand.type) && (value >> (width - 1)) & 1) {
                        return static_cast<int64_t>(value | ~mask);
                    }
                    return static_cast<int64_t>(value);
                }

                uint64_t floatBits(const ir::Operand& operand) const {
                    if (bits(operand.type) == 32) {
                        return std::bit_cast<uint32_t>(static_cast<float>(operand.floating));
                    }
                    return std::bit_cast<uint64_t>(static_cast<double>(operand.floating));
                }

                // ---- emission ---------------------------------------------------

                void emit(MOp op, std::vector<MOperand> operands = {}, std::string comment = {}) {
                    Body.push_back({op, std::move(operands), {}, {}, std::move(comment)});
                }

                void emitDelayed(MOp op, std::vector<MOperand> operands) {
                    emit(op, std::move(operands));
                    emit(MOp::Nop);
                }

                static MOperand R(Reg r) {
                    return MOperand::R(r);
                }

                static MOperand I(int64_t value) {
                    return MOperand::I(value);
                }

                std::string blockLabel(ir::BlockId id) const {
                    return ".LB" + std::to_string(Function.id) + "." + std::to_string(id);
                }

                std::string returnLabel() const {
                    return ".LR" + std::to_string(Function.id);
                }

                std::string freshLabel() {
                    return ".LL" + std::to_string(Function.id) + "." + std::to_string(LocalLabels++);
                }

                // Memory operands must use 16-bit signed displacements; larger frames
                // or record copies form the address in `scratch` first.
                MOperand fit(MOperand memory, Reg scratch, int64_t extent = 8) {
                    if (FitsSigned16(memory.imm) && FitsSigned16(memory.imm + extent)) return memory;
                    emit(MOp::Li, {R(scratch), I(memory.imm)});
                    emit(MOp::Daddu, {R(scratch), R(scratch), R(memory.reg)});
                    return MOperand::M(scratch, 0);
                }

                MOperand slot(int64_t offset, Reg scratch = Address, int64_t extent = 8) {
                    return fit(MOperand::M(reg::Sp, offset), scratch, extent);
                }

                void move(Reg to, Reg from, ir::TypeId id) {
                    if (to == from) return;
                    if (reg::IsFloat(to) && reg::IsFloat(from)) {
                        emit(bits(id) == 32 ? MOp::MovS : MOp::MovD, {R(to), R(from)});
                    } else if (reg::IsFloat(from)) {
                        emit(bits(id) == 32 ? MOp::Mfc1 : MOp::Dmfc1, {R(to), R(from)});
                    } else if (reg::IsFloat(to)) {
                        emit(bits(id) == 32 ? MOp::Mtc1 : MOp::Dmtc1, {R(from), R(to)});
                    } else {
                        emit(MOp::Move, {R(to), R(from)});
                    }
                }

                // Re-establishes the canonical form after an operation that may leave
                // bits outside the type's width.
                void normalize(Reg r, ir::TypeId id) {
                    const auto width = bits(id);
                    if (width >= 64 || isFloat(id)) return;
                    if (isSigned(id)) {
                        if (width == 8) emit(MOp::Seb, {R(r), R(r)});
                        else if (width == 16) emit(MOp::Seh, {R(r), R(r)});
                        else emit(MOp::Sll, {R(r), R(r), I(0)});
                    } else if (width == 32) {
                        emit(MOp::Dext, {R(r), R(r), I(0), I(32)});
                    } else {
                        emit(MOp::Andi, {R(r), R(r), I(static_cast<int64_t>((uint64_t{1} << width) - 1))});
                    }
                }

                // ---- analysis ---------------------------------------------------

                void analyze() {
                    std::set<uint32_t> escaped;
                    for (const auto& block : Function.blocks) {
                        size_t params = 0;
                        for (const auto& inst : block.instructions) {
                            // An object stays promotable only while every reference is a
                            // whole-value load/store of its own type at offset zero.
                            auto direct = [&](const ir::Operand& address, const ir::Operand& value) {
                                return address.kind == OperandKind::Object && address.integer == 0 &&
                                       value.type == object(address.id).type;
                            };
                            const bool directLoad = inst.opcode == Opcode::Load && direct(inst.left, inst.result);
                            const bool directStore = inst.opcode == Opcode::Store && direct(inst.left, inst.right);
                            for (const auto* operand : {&inst.left, &inst.right}) {
                                if (operand->kind != OperandKind::Object) continue;
                                if (operand == &inst.left && (directLoad || directStore)) continue;
                                escaped.insert(operand->id);
                            }
                            if (inst.opcode == Opcode::Param) {
                                ++params;
                            } else if (inst.opcode == Opcode::Call || inst.opcode == Opcode::RegisterDestructor) {
                                auto slots = static_cast<int64_t>(params) + 3;
                                Outgoing = std::max(Outgoing, (slots - 8) * 8);
                                params = 0;
                            }
                        }
                    }
                    for (const auto& item : Function.objects) {
                        const auto kind = type(item.type).kind;
                        if (!escaped.count(item.id) && kind != TypeKind::Record && kind != TypeKind::Array &&
                            kind != TypeKind::Void && kind != TypeKind::Function) {
                            Promoted.insert(item.id);
                        }
                    }
                    Outgoing = AlignUp(Outgoing, 16);
                    Cursor = Outgoing;
                    computeLiveness();
                }

                // Upward-exposed reads and whole-value writes per block, over temporaries
                // and promoted variables, solved globally by ir::LiveOut. This replaces the
                // slides' simplification that every variable is live at block exit.
                void computeLiveness() {
                    ir::LiveSets reads, writes;
                    for (const auto& block : Function.blocks) {
                        auto& read = reads[block.id];
                        auto& written = writes[block.id];
                        for (const auto& inst : block.instructions) {
                            for (auto name : uses(inst)) {
                                if (!written.count(name)) read.insert(name);
                            }
                            if (auto name = definition(inst)) written.insert(*name);
                        }
                    }
                    BlockLiveOut = ir::LiveOut(Function, reads, writes);
                }

                int64_t allocate(int64_t size, int64_t alignment) {
                    Cursor = AlignUp(Cursor, std::max<int64_t>(alignment, 1));
                    const auto offset = Cursor;
                    Cursor += std::max<int64_t>(size, 1);
                    return offset;
                }

                void layoutObjects() {
                    if (isAggregate(type(Function.signature).element)) SretSlot = allocate(8, 8);
                    for (const auto& item : Function.objects) {
                        const auto& t = type(item.type);
                        ObjectSlots[item.id] = allocate(static_cast<int64_t>(t.size),
                                                        static_cast<int64_t>(std::min<uint64_t>(t.alignment, 16)));
                    }
                }

                // Record temporaries are memory values; every other temporary gets an
                // 8-byte home only when it is first spilled or written back.
                int64_t tempHome(uint32_t id) {
                    auto found = TempHomes.find(id);
                    if (found != TempHomes.end()) return found->second;
                    const auto& t = type(Function.temporaries.at(id));
                    const auto offset = isAggregate(Function.temporaries.at(id))
                                            ? allocate(static_cast<int64_t>(t.size),
                                                       static_cast<int64_t>(std::max<uint64_t>(t.alignment, 8)))
                                            : allocate(8, 8);
                    return TempHomes[id] = offset;
                }

                int64_t home(Name name) {
                    return name & ObjectTag ? ObjectSlots.at(static_cast<uint32_t>(name))
                                            : tempHome(static_cast<uint32_t>(name));
                }

                bool liveOut(Name name) const {
                    return LiveOutHere->count(name) > 0;
                }

                std::optional<Name> nameOf(const ir::Operand& operand) const {
                    if (operand.kind == OperandKind::Temporary && !isAggregate(operand.type)) return operand.id;
                    return std::nullopt;
                }

                bool promotedAddress(const ir::Operand& operand) const {
                    return operand.kind == OperandKind::Object && Promoted.count(operand.id);
                }

                std::vector<Name> uses(const ir::Instruction& inst) const {
                    std::vector<Name> result;
                    if (inst.opcode == Opcode::Load && promotedAddress(inst.left)) {
                        result.push_back(ObjectTag | inst.left.id);
                        return result;
                    }
                    for (const auto* operand : {&inst.left, &inst.right}) {
                        if (auto name = nameOf(*operand)) result.push_back(*name);
                    }
                    return result;
                }

                std::optional<Name> definition(const ir::Instruction& inst) const {
                    if (inst.opcode == Opcode::Store && promotedAddress(inst.left)) return ObjectTag | inst.left.id;
                    if (ir::DefinesTemporary(inst)) return nameOf(inst.result);
                    return std::nullopt;
                }

                // Backward next-use scan (lecture 33): record the facts after i, then
                // kill the definition, then make the operands live with next use i.
                void computeNextUse(const ir::Block& block) {
                    const auto& code = block.instructions;
                    BlockEnd = code.size();
                    Next.assign(code.size(), {});
                    Calls.clear();
                    std::map<Name, size_t> current;
                    for (size_t i = code.size(); i-- > 0;) {
                        Next[i] = current;
                        if (auto name = definition(code[i])) current.erase(*name);
                        for (auto name : uses(code[i])) current[name] = i;
                        if (code[i].opcode == Opcode::Call || code[i].opcode == Opcode::RegisterDestructor) {
                            Calls.push_back(i);
                        }
                    }
                    std::reverse(Calls.begin(), Calls.end());
                }

                size_t nextUse(size_t i, Name name) const {
                    auto found = Next[i].find(name);
                    if (found != Next[i].end()) return found->second;
                    return liveOut(name) ? BlockEnd : Never;
                }

                bool live(size_t i, Name name) const {
                    return nextUse(i, name) != Never;
                }

                bool liveAcrossCall(size_t i, Name name) const {
                    const auto use = nextUse(i, name);
                    if (use == Never) return false;
                    auto call = std::upper_bound(Calls.begin(), Calls.end(), i);
                    return call != Calls.end() && *call < use;
                }

                // ---- descriptors ------------------------------------------------

                void forget(Reg r) {
                    for (auto name : Holds[r]) {
                        auto& regs = Locations[name].regs;
                        std::erase(regs, r);
                    }
                    Holds[r].clear();
                }

                void detach(Name name) {
                    auto found = Locations.find(name);
                    if (found == Locations.end()) return;
                    for (auto r : found->second.regs) std::erase(Holds[r], name);
                    found->second.regs.clear();
                }

                // `name` now lives only in r (a new definition, or a copy sharing r).
                void bind(Reg r, Name name) {
                    detach(name);
                    Holds[r].push_back(name);
                    Locations[name] = {{r}, false};
                }

                void loaded(Reg r, Name name) {
                    Holds[r].push_back(name);
                    auto& where = Locations[name];
                    where.regs.push_back(r);
                    where.memory = true;
                }

                void drop(Name name) {
                    detach(name);
                    Locations.erase(name);
                }

                std::optional<Reg> registerOf(Name name) const {
                    auto found = Locations.find(name);
                    if (found == Locations.end() || found->second.regs.empty()) return std::nullopt;
                    return found->second.regs.front();
                }

                void storeHome(Name name, Reg r) {
                    const auto id = nameType(name);
                    const MOp op = name & ObjectTag ? storeOp(id) : (isFloat(id) ? MOp::Sdc1 : MOp::Sd);
                    emit(op, {R(r), slot(home(name))});
                }

                void loadHome(Name name, Reg r) {
                    const auto id = nameType(name);
                    if (reg::IsGeneral(r) && isFloat(id)) {
                        // Raw bits for a variadic float argument; a promoted f32 object
                        // has 4 bytes in memory, a temporary home always 8.
                        emit(name & ObjectTag && bits(id) == 32 ? MOp::Lwu : MOp::Ld, {R(r), slot(home(name))});
                        return;
                    }
                    const MOp op = name & ObjectTag ? loadOp(id) : (isFloat(id) ? MOp::Ldc1 : MOp::Ld);
                    emit(op, {R(r), slot(home(name))});
                }

                // A register may be overwritten without a store when every name in it
                // is dead after i, already in memory, or also held in another register.
                size_t spillCost(size_t i, Reg r) const {
                    size_t cost = 0;
                    for (auto name : Holds[r]) {
                        const auto& where = Locations.at(name);
                        cost += live(i, name) && !where.memory && where.regs.size() == 1;
                    }
                    return cost;
                }

                void spill(size_t i, Reg r) {
                    for (auto name : Holds[r]) {
                        auto& where = Locations[name];
                        if (live(i, name) && !where.memory && where.regs.size() == 1) {
                            storeHome(name, r);
                            where.memory = true;
                        }
                    }
                    forget(r);
                }

                void used(Reg r) {
                    if (reg::IsCalleeSaved(r)) SavedRegisters.insert(r);
                }

                // getReg (lecture 34): an empty register, else the cheapest victim,
                // breaking ties by the farthest next use. Values live across a call
                // prefer callee-saved registers so the call does not force a spill.
                Reg getReg(size_t i, Class cls, const std::vector<Reg>& pinned, bool preferCallee) {
                    const auto& caller = cls == Class::Int ? IntCallerSaved : FloatCallerSaved;
                    const auto& callee = cls == Class::Int ? IntCalleeSaved : FloatCalleeSaved;
                    std::vector<Reg> order = preferCallee ? callee : caller;
                    const auto& rest = preferCallee ? caller : callee;
                    order.insert(order.end(), rest.begin(), rest.end());
                    auto isPinned = [&](Reg r) { return std::find(pinned.begin(), pinned.end(), r) != pinned.end(); };
                    for (auto r : order) {
                        if (!isPinned(r) && Holds[r].empty()) {
                            used(r);
                            return r;
                        }
                    }
                    Reg best = reg::None;
                    size_t bestCost = Never, bestNext = 0;
                    for (auto r : order) {
                        if (isPinned(r)) continue;
                        const auto cost = spillCost(i, r);
                        size_t soonest = Never;
                        for (auto name : Holds[r]) {
                            if (live(i, name)) soonest = std::min(soonest, nextUse(i, name));
                        }
                        if (cost < bestCost || (cost == bestCost && soonest > bestNext)) {
                            best = r;
                            bestCost = cost;
                            bestNext = soonest;
                        }
                    }
                    if (best == reg::None) throw Unsupported("register allocation failed");
                    spill(i, best);
                    used(best);
                    return best;
                }

                // Register for the result x of instruction i: one holding only x, else
                // an operand register that may be overwritten, else getReg.
                Reg target(size_t i, Name x, const std::vector<std::pair<std::optional<Name>, Reg>>& operands,
                           std::vector<Reg>& pinned) {
                    const auto cls = classOf(nameType(x));
                    auto fitsClass = [&](Reg r) { return (cls == Class::Float) == reg::IsFloat(r); };
                    if (auto r = registerOf(x); r && Holds[*r].size() == 1 && fitsClass(*r)) return *r;
                    for (const auto& [name, r] : operands) {
                        if (name && fitsClass(r) && registerOf(*name) && spillCost(i, r) == 0 &&
                            !liveAcrossCall(i, x) == !reg::IsCalleeSaved(r)) {
                            return r;
                        }
                    }
                    for (const auto& [name, r] : operands) {
                        if (name && fitsClass(r) && registerOf(*name) && spillCost(i, r) == 0) return r;
                    }
                    auto r = getReg(i, cls, pinned, liveAcrossCall(i, x));
                    pinned.push_back(r);
                    return r;
                }

                void define(Reg r, Name x) {
                    forget(r);
                    bind(r, x);
                }

                void freeDead(size_t i) {
                    std::vector<Name> dead;
                    for (const auto& [name, where] : Locations) {
                        if (!live(i, name)) dead.push_back(name);
                    }
                    for (auto name : dead) drop(name);
                }

                // Stores every live-out value whose only copy is in a register, so the
                // next block starts with empty descriptors (all values in memory).
                void flush() {
                    for (auto& [name, where] : Locations) {
                        if (liveOut(name) && !where.memory && !where.regs.empty()) {
                            storeHome(name, where.regs.front());
                            where.memory = true;
                        }
                    }
                }

                void resetDescriptors() {
                    for (auto& names : Holds) names.clear();
                    Locations.clear();
                }

                // ---- operands ---------------------------------------------------

                // Pins registers already holding the instruction's operands so loading
                // one operand never evicts another.
                void pinResident(const ir::Instruction& inst, std::vector<Reg>& pinned) const {
                    for (const auto* operand : {&inst.left, &inst.right}) {
                        if (auto name = nameOf(*operand)) {
                            if (auto r = registerOf(*name)) pinned.push_back(*r);
                        }
                    }
                }

                Reg ensure(size_t i, Name name, std::vector<Reg>& pinned) {
                    if (auto r = registerOf(name)) {
                        pinned.push_back(*r);
                        return *r;
                    }
                    auto r = getReg(i, classOf(nameType(name)), pinned, liveAcrossCall(i, name));
                    loadHome(name, r);
                    loaded(r, name);
                    pinned.push_back(r);
                    return r;
                }

                MOperand addressOfObject(const ir::Operand& operand, Reg scratch) {
                    const auto& item = object(operand.id);
                    const auto addend = static_cast<int64_t>(operand.integer);
                    if (item.global) {
                        emit(MOp::La, {R(scratch), MOperand::S(ObjectSymbol(item), addend)});
                        return MOperand::M(scratch, 0);
                    }
                    return MOperand::M(reg::Sp, ObjectSlots.at(operand.id) + addend);
                }

                void materializeAddress(const MOperand& memory, Reg into) {
                    if (memory.imm == 0) {
                        if (memory.reg != into) emit(MOp::Move, {R(into), R(memory.reg)});
                    } else if (FitsSigned16(memory.imm)) {
                        emit(MOp::Daddiu, {R(into), R(memory.reg), I(memory.imm)});
                    } else {
                        emit(MOp::Li, {R(into), I(memory.imm)});
                        emit(MOp::Daddu, {R(into), R(into), R(memory.reg)});
                    }
                }

                void loadFloatLiteral(const ir::Operand& operand, Reg into) {
                    const auto pattern = floatBits(operand);
                    const bool single = bits(operand.type) == 32;
                    if (pattern == 0) {
                        emit(single ? MOp::Mtc1 : MOp::Dmtc1, {R(reg::Zero), R(into)});
                        return;
                    }
                    emit(MOp::La, {R(Address), MOperand::S(Pool.intern(single ? 32 : 64, pattern))});
                    emit(single ? MOp::Lwc1 : MOp::Ldc1, {R(into), MOperand::M(Address, 0)});
                }

                // Value of any scalar operand in a register. Names get tracked pool
                // registers; literals and addresses use the given scratch.
                Reg use(size_t i, const ir::Operand& operand, Reg scratch, std::vector<Reg>& pinned) {
                    switch (operand.kind) {
                    case OperandKind::Temporary:
                        if (isAggregate(operand.type)) {
                            emit(MOp::Daddiu, {R(scratch), R(reg::Sp), I(tempHome(operand.id))});
                            return scratch;
                        }
                        return ensure(i, operand.id, pinned);
                    case OperandKind::Integer: {
                        const auto value = canonical(operand);
                        if (value == 0) return reg::Zero;
                        emit(MOp::Li, {R(scratch), I(value)});
                        return scratch;
                    }
                    case OperandKind::Floating:
                        loadFloatLiteral(operand, scratch);
                        return scratch;
                    case OperandKind::Object:
                        materializeAddress(addressOfObject(operand, scratch), scratch);
                        return scratch;
                    case OperandKind::Function:
                        emit(MOp::La, {R(scratch), MOperand::S(functionSymbol(operand.id))});
                        return scratch;
                    default:
                        throw Unsupported("operand kind cannot be a value");
                    }
                }

                std::string functionSymbol(uint32_t id) const {
                    auto* callee = Module.functionById(id);
                    if (!callee) throw Unsupported("unknown function " + std::to_string(id));
                    return FunctionSymbol(*callee);
                }

                // Memory operand for an address operand. Non-promoted objects are
                // addressed directly; pointers come from (tracked) registers.
                MOperand address(size_t i, const ir::Operand& operand, Reg scratch, std::vector<Reg>& pinned) {
                    if (operand.kind == OperandKind::Object) return addressOfObject(operand, scratch);
                    return MOperand::M(use(i, operand, scratch, pinned), 0);
                }

                // Moves a value directly into a fixed register (call staging/return).
                // Integer registers receive float values as raw bits (n64 varargs).
                void materializeInto(size_t i, const ir::Operand& operand, Reg into) {
                    if (operand.kind == OperandKind::Temporary && isAggregate(operand.type)) {
                        materializeAddress(MOperand::M(reg::Sp, tempHome(operand.id)), into);
                        return;
                    }
                    if (auto name = nameOf(operand)) {
                        if (auto r = registerOf(*name)) {
                            move(into, *r, operand.type);
                        } else {
                            loadHome(*name, into);
                        }
                        return;
                    }
                    if (operand.kind == OperandKind::Floating) {
                        if (reg::IsGeneral(into)) {
                            emit(MOp::Li, {R(into), I(static_cast<int64_t>(floatBits(operand)))});
                        } else {
                            loadFloatLiteral(operand, into);
                        }
                        return;
                    }
                    std::vector<Reg> pinned;
                    auto r = use(i, operand, into, pinned);
                    if (r != into) move(into, r, operand.type);
                }

                // ---- records ----------------------------------------------------

                // Copies `size` bytes between two memory operands with chunks allowed by
                // the alignment. Large bodies use a loop over $a0-$a2 (free between
                // calls); the prologue disallows loops because $a* hold arguments.
                void copyBytes(MOperand to, MOperand from, uint64_t size, uint64_t alignment, bool loops = true) {
                    const auto chunk = static_cast<int64_t>(std::min<uint64_t>(std::bit_floor(std::max<uint64_t>(alignment, 1)), 8));
                    auto ops = [](int64_t width) -> std::pair<MOp, MOp> {
                        switch (width) {
                        case 8: return {MOp::Ld, MOp::Sd};
                        case 4: return {MOp::Lw, MOp::Sw};
                        case 2: return {MOp::Lh, MOp::Sh};
                        default: return {MOp::Lb, MOp::Sb};
                        }
                    };
                    if (loops && size > 256 && size % chunk == 0) {
                        materializeAddress(from, reg::A0);
                        materializeAddress(to, reg::A1);
                        emit(MOp::Li, {R(reg::A2), I(static_cast<int64_t>(size))});
                        emit(MOp::Daddu, {R(reg::A2), R(reg::A2), R(reg::A0)});
                        const auto loop = freshLabel();
                        const auto [load, store] = ops(chunk);
                        emit(MOp::Label, {MOperand::L(loop)});
                        emit(load, {R(ScratchA), MOperand::M(reg::A0, 0)});
                        emit(store, {R(ScratchA), MOperand::M(reg::A1, 0)});
                        emit(MOp::Daddiu, {R(reg::A0), R(reg::A0), I(chunk)});
                        emit(MOp::Daddiu, {R(reg::A1), R(reg::A1), I(chunk)});
                        emitDelayed(MOp::Bne, {R(reg::A0), R(reg::A2), MOperand::L(loop)});
                        emit(MOp::Label, {MOperand::L(freshLabel())});
                        return;
                    }
                    // Out-of-range displacements need an address register distinct
                    // from both bases and from the data register ($v0).
                    const auto extent = static_cast<int64_t>(size);
                    auto spare = [&](Reg avoid) {
                        for (Reg r : {ScratchB, Address, reg::A3}) {
                            if (r != avoid && r != from.reg && r != to.reg) return r;
                        }
                        return reg::A2;
                    };
                    from = fit(from, spare(reg::None), extent);
                    to = fit(to, spare(from.reg), extent);
                    for (int64_t offset = 0; offset < extent;) {
                        auto width = chunk;
                        while (offset + width > extent) width /= 2;
                        const auto [load, store] = ops(width);
                        emit(load, {R(ScratchA), MOperand::M(from.reg, from.imm + offset)});
                        emit(store, {R(ScratchA), MOperand::M(to.reg, to.imm + offset)});
                        offset += width;
                    }
                }

                uint64_t sizeOf(ir::TypeId id) const {
                    return type(id).size;
                }

                uint64_t alignOf(ir::TypeId id) const {
                    return type(id).alignment;
                }

                // ---- blocks -----------------------------------------------------

                void emitBlock(const ir::Block& block) {
                    LiveOutHere = &BlockLiveOut[block.id];
                    computeNextUse(block);
                    resetDescriptors();
                    // Descriptors are flushed at every block boundary, so only the
                    // frame/return state is live here; the peephole pass relies on it.
                    Body.push_back({MOp::Label, {MOperand::L(blockLabel(block.id))}, LiveAtBlockStart, {}, {}});
                    const auto& code = block.instructions;
                    const bool fused = fusedCompare(block);
                    std::vector<ir::Operand> arguments;
                    for (size_t i = 0; i < code.size(); ++i) {
                        const auto& inst = code[i];
                        if (fused && i + 3 == code.size()) {
                            continue; // emitted with the branch
                        }
                        switch (inst.opcode) {
                        case Opcode::Param:
                            arguments.push_back(inst.left);
                            continue; // consumed by the call; still a use for next-use
                        case Opcode::Call:
                            emitCall(i, inst, arguments);
                            arguments.clear();
                            break;
                        case Opcode::RegisterDestructor:
                            emitRegisterDestructor(i, inst);
                            break;
                        case Opcode::Jump:
                            flush();
                            emitDelayed(MOp::B, {MOperand::L(blockLabel(inst.left.id))});
                            break;
                        case Opcode::JumpIf:
                            emitConditional(i, block, fused);
                            return;
                        case Opcode::Return:
                            emitReturn(i, inst);
                            break;
                        default:
                            emitInstruction(i, inst);
                            break;
                        }
                        freeDead(i);
                    }
                }

                bool isCompare(Opcode op) const {
                    return op >= Opcode::Equal && op <= Opcode::GreaterEqual;
                }

                // `t = a < b; if_goto t, L; goto M` with t used nowhere else becomes one
                // compare-and-branch.
                bool fusedCompare(const ir::Block& block) const {
                    const auto& code = block.instructions;
                    if (code.size() < 3) return false;
                    const auto& compare = code[code.size() - 3];
                    const auto& branch = code[code.size() - 2];
                    if (branch.opcode != Opcode::JumpIf || !isCompare(compare.opcode) ||
                        branch.left.kind != OperandKind::Temporary || compare.result.id != branch.left.id) {
                        return false;
                    }
                    const Name name = compare.result.id;
                    return !liveOut(name) && nextUse(code.size() - 3, name) == code.size() - 2 &&
                           !live(code.size() - 2, name);
                }

                void emitConditional(size_t i, const ir::Block& block, bool fused) {
                    const auto& code = block.instructions;
                    const auto& branch = code[i];
                    const auto taken = MOperand::L(blockLabel(branch.right.id));
                    const auto other = MOperand::L(blockLabel(code[i + 1].left.id));
                    std::vector<Reg> pinned;
                    if (fused) {
                        const auto& compare = code[i - 1];
                        pinResident(compare, pinned);
                        emitCompareBranch(i - 1, compare, taken, pinned);
                    } else if (branch.left.kind == OperandKind::Integer) {
                        flush();
                        emitDelayed(MOp::B, {branch.left.integer ? taken : other});
                        return;
                    } else {
                        auto r = use(i, branch.left, ScratchA, pinned);
                        if (reg::IsFloat(r)) throw Unsupported("floating branch condition");
                        flush();
                        emitDelayed(MOp::Bne, {R(r), R(reg::Zero), taken});
                    }
                    emitDelayed(MOp::B, {other});
                }

                void emitCompareBranch(size_t i, const ir::Instruction& compare, const MOperand& taken,
                                       std::vector<Reg>& pinned) {
                    const auto operandType = compare.left.type;
                    if (isFloat(operandType)) {
                        auto a = use(i, compare.left, FloatScratchA, pinned);
                        auto b = use(i, compare.right, FloatScratchB, pinned);
                        flush();
                        const bool single = bits(operandType) == 32;
                        auto [op, swap, onTrue] = floatCompare(compare.opcode, single);
                        emit(op, {R(swap ? b : a), R(swap ? a : b)});
                        emitDelayed(onTrue ? MOp::Bc1t : MOp::Bc1f, {taken});
                        return;
                    }
                    auto a = use(i, compare.left, ScratchA, pinned);
                    auto b = use(i, compare.right, ScratchB, pinned);
                    flush();
                    const bool sign = isSigned(operandType);
                    const auto zero = R(reg::Zero);
                    switch (compare.opcode) {
                    case Opcode::Equal:
                        emitDelayed(MOp::Beq, {R(a), R(b), taken});
                        return;
                    case Opcode::NotEqual:
                        emitDelayed(MOp::Bne, {R(a), R(b), taken});
                        return;
                    default:
                        break;
                    }
                    // Signed comparisons against zero have dedicated branches.
                    if (sign && b == reg::Zero) {
                        static const std::map<Opcode, MOp> againstZero{{Opcode::Less, MOp::Bltz},
                                                                       {Opcode::Greater, MOp::Bgtz},
                                                                       {Opcode::LessEqual, MOp::Blez},
                                                                       {Opcode::GreaterEqual, MOp::Bgez}};
                        emitDelayed(againstZero.at(compare.opcode), {R(a), taken});
                        return;
                    }
                    const auto slt = sign ? MOp::Slt : MOp::Sltu;
                    switch (compare.opcode) {
                    case Opcode::Less:
                        emit(slt, {R(ScratchA), R(a), R(b)});
                        emitDelayed(MOp::Bne, {R(ScratchA), zero, taken});
                        break;
                    case Opcode::Greater:
                        emit(slt, {R(ScratchA), R(b), R(a)});
                        emitDelayed(MOp::Bne, {R(ScratchA), zero, taken});
                        break;
                    case Opcode::LessEqual:
                        emit(slt, {R(ScratchA), R(b), R(a)});
                        emitDelayed(MOp::Beq, {R(ScratchA), zero, taken});
                        break;
                    default:
                        emit(slt, {R(ScratchA), R(a), R(b)});
                        emitDelayed(MOp::Beq, {R(ScratchA), zero, taken});
                        break;
                    }
                }

                // (compare op, swap operands, condition true when cc set). NaN makes
                // every ordered comparison false and only `!=` true.
                std::tuple<MOp, bool, bool> floatCompare(Opcode op, bool single) const {
                    const auto eq = single ? MOp::CeqS : MOp::CeqD, lt = single ? MOp::CltS : MOp::CltD,
                               le = single ? MOp::CleS : MOp::CleD;
                    switch (op) {
                    case Opcode::Equal: return {eq, false, true};
                    case Opcode::NotEqual: return {eq, false, false};
                    case Opcode::Less: return {lt, false, true};
                    case Opcode::Greater: return {lt, true, true};
                    case Opcode::LessEqual: return {le, false, true};
                    default: return {le, true, true};
                    }
                }

                // ---- calls ------------------------------------------------------

                // Caller-saved registers die at a call: values still needed afterwards
                // are stored to their homes first (unless a copy survives elsewhere).
                void saveAcrossCall(size_t i) {
                    for (const auto* pool : {&IntCallerSaved, &FloatCallerSaved}) {
                        for (auto r : *pool) {
                            for (auto name : Holds[r]) {
                                auto& where = Locations[name];
                                const bool survives =
                                    where.memory || std::any_of(where.regs.begin(), where.regs.end(),
                                                                [](Reg other) { return reg::IsCalleeSaved(other); });
                                if (live(i, name) && !survives) {
                                    storeHome(name, r);
                                    where.memory = true;
                                }
                            }
                        }
                    }
                }

                void finishCall(std::vector<Reg> arguments) {
                    arguments.push_back(Address);
                    Body.push_back({MOp::Jalr, {R(Address)}, std::move(arguments), CallClobbers, {}});
                    emit(MOp::Nop);
                    for (const auto* pool : {&IntCallerSaved, &FloatCallerSaved}) {
                        for (auto r : *pool) forget(r);
                    }
                    MakesCall = true;
                }

                void emitCall(size_t i, const ir::Instruction& inst, const std::vector<ir::Operand>& arguments) {
                    const auto& pointer = type(inst.left.type);
                    const auto& signature = type(pointer.kind == TypeKind::Pointer ? pointer.element : inst.left.type);
                    if (signature.kind != TypeKind::Function) throw Unsupported("call through a non-function type");
                    const bool sret = isAggregate(signature.element);
                    saveAcrossCall(i);
                    std::vector<Reg> staged;
                    size_t slotIndex = 0;
                    if (sret) {
                        if (!inst.result) throw Unsupported("record call without a result");
                        materializeAddress(MOperand::M(reg::Sp, tempHome(inst.result.id)), reg::A0);
                        staged.push_back(reg::A0);
                        slotIndex = 1;
                    }
                    for (size_t j = 0; j < arguments.size(); ++j, ++slotIndex) {
                        const auto& argument = arguments[j];
                        const bool fixed = j < signature.parameters.size();
                        const bool inFloat = fixed && isFloat(argument.type) && !isAggregate(argument.type);
                        if (slotIndex < 8) {
                            const Reg into = inFloat ? reg::F(12 + static_cast<unsigned>(slotIndex))
                                                     : static_cast<Reg>(reg::A0 + slotIndex);
                            materializeInto(i, argument, into);
                            if (isUnsigned32(argument.type)) emit(MOp::Sll, {R(into), R(into), I(0)});
                            staged.push_back(into);
                        } else {
                            const Reg via = inFloat ? FloatScratchA : ScratchA;
                            materializeInto(i, argument, via);
                            if (isUnsigned32(argument.type)) emit(MOp::Sll, {R(via), R(via), I(0)});
                            const auto offset = static_cast<int64_t>((slotIndex - 8) * 8);
                            emit(inFloat ? (bits(argument.type) == 32 ? MOp::Swc1 : MOp::Sdc1) : MOp::Sd,
                                 {R(via), slot(offset)});
                        }
                    }
                    if (inst.left.kind == OperandKind::Function) {
                        emit(MOp::La, {R(Address), MOperand::S(functionSymbol(inst.left.id))});
                    } else {
                        materializeInto(i, inst.left, Address);
                    }
                    finishCall(staged);
                    if (!inst.result || sret) return;
                    const auto name = *nameOf(inst.result);
                    std::vector<Reg> pinned;
                    auto r = getReg(i, classOf(inst.result.type), pinned, liveAcrossCall(i, name));
                    if (isFloat(inst.result.type)) {
                        move(r, reg::F(0), inst.result.type);
                    } else if (isUnsigned32(inst.result.type)) {
                        emit(MOp::Dext, {R(r), R(reg::V0), I(0), I(32)});
                    } else {
                        move(r, reg::V0, inst.result.type);
                    }
                    define(r, name);
                }

                // register_dtor f, p  ==>  __cxa_atexit(f, p, &__dso_handle); glibc runs
                // these after main returns in reverse registration order.
                void emitRegisterDestructor(size_t i, const ir::Instruction& inst) {
                    saveAcrossCall(i);
                    materializeInto(i, inst.left, reg::A0);
                    materializeInto(i, inst.right, reg::A1);
                    emit(MOp::La, {R(reg::A2), MOperand::S("__dso_handle")});
                    emit(MOp::La, {R(Address), MOperand::S("__cxa_atexit")});
                    finishCall({reg::A0, reg::A1, reg::A2});
                }

                void emitReturn(size_t i, const ir::Instruction& inst) {
                    if (inst.left) {
                        if (isAggregate(inst.left.type)) {
                            emit(MOp::Ld, {R(ScratchB), slot(SretSlot)});
                            copyBytes(MOperand::M(ScratchB, 0), MOperand::M(reg::Sp, tempHome(inst.left.id)),
                                      sizeOf(inst.left.type), alignOf(inst.left.type));
                            emit(MOp::Ld, {R(reg::V0), slot(SretSlot)});
                        } else {
                            const Reg into = isFloat(inst.left.type) ? reg::F(0) : reg::V0;
                            materializeInto(i, inst.left, into);
                            if (isUnsigned32(inst.left.type)) emit(MOp::Sll, {R(into), R(into), I(0)});
                        }
                    }
                    emitDelayed(MOp::B, {MOperand::L(returnLabel())});
                }

                // ---- ordinary instructions ---------------------------------------

                void emitInstruction(size_t i, const ir::Instruction& inst) {
                    switch (inst.opcode) {
                    case Opcode::Copy:
                        emitCopy(i, inst);
                        return;
                    case Opcode::Load:
                        emitLoad(i, inst);
                        return;
                    case Opcode::Store:
                        emitStore(i, inst);
                        return;
                    case Opcode::Cast:
                        emitCast(i, inst);
                        return;
                    case Opcode::ZeroMemory:
                        emitZero(i, inst);
                        return;
                    case Opcode::AddressOffset:
                        emitBinary(i, inst, Opcode::Add);
                        return;
                    default:
                        if (inst.opcode >= Opcode::Add && inst.opcode <= Opcode::GreaterEqual) {
                            emitBinary(i, inst, inst.opcode);
                            return;
                        }
                        if (inst.opcode >= Opcode::Negate && inst.opcode <= Opcode::LogicalNot) {
                            emitUnary(i, inst);
                            return;
                        }
                        throw Unsupported("unexpected instruction");
                    }
                }

                // x = y emits no code when y is a name: x joins y's register (slides).
                void shareName(size_t i, Name x, Name value) {
                    std::vector<Reg> pinned;
                    auto r = ensure(i, value, pinned);
                    detach(x);
                    Holds[r].push_back(x);
                    Locations[x] = {{r}, false};
                }

                void share(size_t i, Name x, const ir::Operand& value) {
                    if (auto name = nameOf(value)) {
                        shareName(i, x, *name);
                        return;
                    }
                    std::vector<Reg> pinned;
                    const auto cls = classOf(nameType(x));
                    auto r = getReg(i, cls, pinned, liveAcrossCall(i, x));
                    materializeInto(i, value, r);
                    define(r, x);
                }

                void emitCopy(size_t i, const ir::Instruction& inst) {
                    if (isAggregate(inst.result.type)) {
                        if (inst.left.kind != OperandKind::Temporary) throw Unsupported("record copy source");
                        copyBytes(MOperand::M(reg::Sp, tempHome(inst.result.id)),
                                  MOperand::M(reg::Sp, tempHome(inst.left.id)), sizeOf(inst.result.type),
                                  alignOf(inst.result.type));
                        return;
                    }
                    share(i, inst.result.id, inst.left);
                }

                void emitLoad(size_t i, const ir::Instruction& inst) {
                    if (promotedAddress(inst.left)) {
                        shareName(i, inst.result.id, ObjectTag | inst.left.id);
                        return;
                    }
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    if (isAggregate(inst.result.type)) {
                        auto from = address(i, inst.left, Address, pinned);
                        copyBytes(MOperand::M(reg::Sp, tempHome(inst.result.id)), from, sizeOf(inst.result.type),
                                  alignOf(inst.result.type));
                        return;
                    }
                    const Name x = inst.result.id;
                    Reg r;
                    MOperand from;
                    if (inst.left.kind == OperandKind::Object) {
                        // Choose (and possibly spill for) the result first: a spill in a
                        // large frame uses $t9, which then holds the global's address.
                        r = target(i, x, {}, pinned);
                        from = fit(addressOfObject(inst.left, Address), Address);
                    } else {
                        from = address(i, inst.left, Address, pinned);
                        r = target(i, x, {{nameOf(inst.left), from.reg}}, pinned);
                    }
                    emit(loadOp(inst.result.type), {R(r), from});
                    define(r, x);
                }

                void emitStore(size_t i, const ir::Instruction& inst) {
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    if (promotedAddress(inst.left)) {
                        const Name variable = ObjectTag | inst.left.id;
                        if (auto name = nameOf(inst.right)) {
                            auto r = ensure(i, *name, pinned);
                            bind(r, variable);
                        } else {
                            // A literal or address goes straight to the slot; nothing is cached.
                            auto r = use(i, inst.right, isFloat(inst.right.type) ? FloatScratchA : ScratchA, pinned);
                            drop(variable);
                            emit(storeOp(inst.right.type), {R(r), slot(ObjectSlots.at(inst.left.id))});
                            Locations[variable] = {{}, true};
                        }
                        return;
                    }
                    if (isAggregate(inst.right.type)) {
                        auto to = address(i, inst.left, ScratchB, pinned);
                        copyBytes(to, MOperand::M(reg::Sp, tempHome(inst.right.id)), sizeOf(inst.right.type),
                                  alignOf(inst.right.type));
                        return;
                    }
                    // Value first: a float literal uses $t9 to reach the literal pool.
                    Reg value = reg::Zero;
                    const bool zeroBits = (inst.right.kind == OperandKind::Integer && canonical(inst.right) == 0) ||
                                          (inst.right.kind == OperandKind::Floating && floatBits(inst.right) == 0);
                    if (!zeroBits) {
                        value = use(i, inst.right, isFloat(inst.right.type) ? FloatScratchA : ScratchA, pinned);
                    }
                    auto to = fit(address(i, inst.left, Address, pinned), Address);
                    MOp op = storeOp(inst.right.type);
                    if (value == reg::Zero && isFloat(inst.right.type)) op = bits(inst.right.type) == 32 ? MOp::Sw : MOp::Sd;
                    emit(op, {R(value), to});
                }

                void emitZero(size_t i, const ir::Instruction& inst) {
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    auto to = address(i, inst.left, Address, pinned);
                    uint64_t alignment = 1;
                    if (const auto& pointer = type(inst.left.type); pointer.kind == TypeKind::Pointer && pointer.element) {
                        alignment = std::max<uint64_t>(type(pointer.element).alignment, 1);
                    }
                    if (inst.left.kind == OperandKind::Object && !object(inst.left.id).global) {
                        alignment = std::max<uint64_t>(alignment, std::min<uint64_t>(alignOf(object(inst.left.id).type), 16));
                    }
                    if (inst.right.kind == OperandKind::Integer) {
                        const auto count = static_cast<int64_t>(inst.right.integer);
                        int64_t chunk = static_cast<int64_t>(std::min<uint64_t>(std::bit_floor(alignment), 8));
                        while (count % chunk) chunk /= 2;
                        const MOp store = chunk == 8 ? MOp::Sd : chunk == 4 ? MOp::Sw : chunk == 2 ? MOp::Sh : MOp::Sb;
                        if (count <= 128) {
                            to = fit(to, Address, count);
                            for (int64_t offset = 0; offset < count; offset += chunk) {
                                emit(store, {R(reg::Zero), MOperand::M(to.reg, to.imm + offset)});
                            }
                            return;
                        }
                        materializeAddress(to, reg::A0);
                        emit(MOp::Li, {R(reg::A1), I(count)});
                        emit(MOp::Daddu, {R(reg::A1), R(reg::A1), R(reg::A0)});
                        const auto loop = freshLabel();
                        emit(MOp::Label, {MOperand::L(loop)});
                        emit(store, {R(reg::Zero), MOperand::M(reg::A0, 0)});
                        emit(MOp::Daddiu, {R(reg::A0), R(reg::A0), I(chunk)});
                        emitDelayed(MOp::Bne, {R(reg::A0), R(reg::A1), MOperand::L(loop)});
                        emit(MOp::Label, {MOperand::L(freshLabel())});
                        return;
                    }
                    materializeAddress(to, reg::A0);
                    auto count = use(i, inst.right, ScratchA, pinned);
                    emit(MOp::Daddu, {R(reg::A1), R(reg::A0), R(count)});
                    const auto loop = freshLabel(), done = freshLabel();
                    emitDelayed(MOp::Beq, {R(reg::A0), R(reg::A1), MOperand::L(done)});
                    emit(MOp::Label, {MOperand::L(loop)});
                    emit(MOp::Sb, {R(reg::Zero), MOperand::M(reg::A0, 0)});
                    emit(MOp::Daddiu, {R(reg::A0), R(reg::A0), I(1)});
                    emitDelayed(MOp::Bne, {R(reg::A0), R(reg::A1), MOperand::L(loop)});
                    emit(MOp::Label, {MOperand::L(done)});
                }

                // ---- arithmetic -------------------------------------------------

                bool is32Signed(ir::TypeId id) const {
                    return bits(id) == 32 && isSigned(id);
                }

                // Immediate form of x = y op k, if one exists for this type.
                bool emitImmediate(Opcode op, ir::TypeId id, Reg x, Reg y, int64_t k) {
                    const bool narrow = is32Signed(id), wide = bits(id) >= 64;
                    switch (op) {
                    case Opcode::Add:
                    case Opcode::Subtract: {
                        const auto value = op == Opcode::Add ? k : -k;
                        if (!FitsSigned16(value)) return false;
                        emit(narrow ? MOp::Addiu : MOp::Daddiu, {R(x), R(y), I(value)});
                        if (!narrow && !wide) normalize(x, id);
                        return true;
                    }
                    case Opcode::BitAnd:
                    case Opcode::BitOr:
                    case Opcode::BitXor: {
                        if (k < 0 || k > 0xffff) return false;
                        const auto mop = op == Opcode::BitAnd ? MOp::Andi : op == Opcode::BitOr ? MOp::Ori : MOp::Xori;
                        emit(mop, {R(x), R(y), I(k)});
                        return true;
                    }
                    case Opcode::ShiftLeft:
                    case Opcode::ShiftRight: {
                        const auto amount = k & (wide ? 63 : 31);
                        const bool left = op == Opcode::ShiftLeft;
                        if (narrow) {
                            emit(left ? MOp::Sll : MOp::Sra, {R(x), R(y), I(amount)});
                            return true;
                        }
                        const bool high = (k & 63) >= 32;
                        MOp mop = left ? (high ? MOp::Dsll32 : MOp::Dsll)
                                       : isSigned(id) ? (high ? MOp::Dsra32 : MOp::Dsra) : (high ? MOp::Dsrl32 : MOp::Dsrl);
                        emit(mop, {R(x), R(y), I((k & 63) % 32)});
                        if (left && !wide) normalize(x, id);
                        return true;
                    }
                    default:
                        return false;
                    }
                }

                void emitArithmetic(Opcode op, ir::TypeId id, Reg x, Reg y, Reg z) {
                    const bool narrow = is32Signed(id), wide = bits(id) >= 64, sign = isSigned(id);
                    auto finish = [&] {
                        if (!narrow && !wide) normalize(x, id);
                    };
                    switch (op) {
                    case Opcode::Add:
                        emit(narrow ? MOp::Addu : MOp::Daddu, {R(x), R(y), R(z)});
                        return finish();
                    case Opcode::Subtract:
                        emit(narrow ? MOp::Subu : MOp::Dsubu, {R(x), R(y), R(z)});
                        return finish();
                    case Opcode::Multiply:
                        if (narrow) {
                            emit(MOp::Mul, {R(x), R(y), R(z)});
                            return;
                        }
                        emit(MOp::Dmultu, {R(y), R(z)});
                        emit(MOp::Mflo, {R(x)});
                        return finish();
                    case Opcode::Divide:
                    case Opcode::Remainder: {
                        // 32-bit div needs sign-extended inputs: true for i32 only.
                        const auto mop = narrow ? MOp::Div : sign ? MOp::Ddiv : MOp::Ddivu;
                        emit(mop, {R(y), R(z)});
                        emit(op == Opcode::Divide ? MOp::Mflo : MOp::Mfhi, {R(x)});
                        return finish();
                    }
                    case Opcode::ShiftLeft:
                        emit(narrow ? MOp::Sllv : MOp::Dsllv, {R(x), R(y), R(z)});
                        return finish();
                    case Opcode::ShiftRight:
                        emit(narrow ? MOp::Srav : sign ? MOp::Dsrav : MOp::Dsrlv, {R(x), R(y), R(z)});
                        return;
                    case Opcode::BitAnd:
                        emit(MOp::And, {R(x), R(y), R(z)});
                        return;
                    case Opcode::BitOr:
                        emit(MOp::Or, {R(x), R(y), R(z)});
                        return;
                    case Opcode::BitXor:
                        emit(MOp::Xor, {R(x), R(y), R(z)});
                        return;
                    default:
                        throw Unsupported("unexpected arithmetic");
                    }
                }

                void emitFloatArithmetic(Opcode op, ir::TypeId id, Reg x, Reg y, Reg z) {
                    const bool single = bits(id) == 32;
                    static const std::map<Opcode, std::pair<MOp, MOp>> ops{
                        {Opcode::Add, {MOp::AddS, MOp::AddD}}, {Opcode::Subtract, {MOp::SubS, MOp::SubD}},
                        {Opcode::Multiply, {MOp::MulS, MOp::MulD}}, {Opcode::Divide, {MOp::DivS, MOp::DivD}}};
                    auto found = ops.find(op);
                    if (found == ops.end()) throw Unsupported("unexpected floating operation");
                    emit(single ? found->second.first : found->second.second, {R(x), R(y), R(z)});
                }

                void emitIntegerCompare(Opcode op, ir::TypeId operandType, Reg x, Reg a, Reg b) {
                    const auto slt = isSigned(operandType) ? MOp::Slt : MOp::Sltu;
                    switch (op) {
                    case Opcode::Equal:
                        emit(MOp::Xor, {R(x), R(a), R(b)});
                        emit(MOp::Sltiu, {R(x), R(x), I(1)});
                        return;
                    case Opcode::NotEqual:
                        emit(MOp::Xor, {R(x), R(a), R(b)});
                        emit(MOp::Sltu, {R(x), R(reg::Zero), R(x)});
                        return;
                    case Opcode::Less:
                        emit(slt, {R(x), R(a), R(b)});
                        return;
                    case Opcode::Greater:
                        emit(slt, {R(x), R(b), R(a)});
                        return;
                    case Opcode::LessEqual:
                        emit(slt, {R(x), R(b), R(a)});
                        emit(MOp::Xori, {R(x), R(x), I(1)});
                        return;
                    default:
                        emit(slt, {R(x), R(a), R(b)});
                        emit(MOp::Xori, {R(x), R(x), I(1)});
                        return;
                    }
                }

                void emitBinary(size_t i, const ir::Instruction& inst, Opcode op) {
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    const auto operandType = inst.left.type;
                    const bool floating = isFloat(operandType);
                    const Name x = inst.result.id;
                    auto left = inst.left, right = inst.right;
                    const bool commutative = op == Opcode::Add || op == Opcode::Multiply || op == Opcode::BitAnd ||
                                             op == Opcode::BitOr || op == Opcode::BitXor;
                    if (commutative && left.kind == OperandKind::Integer && right.kind != OperandKind::Integer) {
                        std::swap(left, right);
                    }
                    const auto scratchA = floating ? FloatScratchA : ScratchA;
                    const auto scratchB = floating ? FloatScratchB : ScratchB;
                    Reg a = use(i, left, scratchA, pinned);
                    if (!floating && !isCompare(op) && right.kind == OperandKind::Integer) {
                        auto r = target(i, x, {{nameOf(left), a}}, pinned);
                        if (emitImmediate(op, inst.result.type, r, a, canonical(right))) {
                            define(r, x);
                            return;
                        }
                        Reg b = use(i, right, scratchB, pinned);
                        emitArithmetic(op, inst.result.type, r, a, b);
                        define(r, x);
                        return;
                    }
                    if (!floating && isCompare(op) && right.kind == OperandKind::Integer &&
                        (op == Opcode::Less || op == Opcode::GreaterEqual) && FitsSigned16(canonical(right))) {
                        auto r = target(i, x, {{nameOf(left), a}}, pinned);
                        emit(isSigned(operandType) ? MOp::Slti : MOp::Sltiu, {R(r), R(a), I(canonical(right))});
                        if (op == Opcode::GreaterEqual) emit(MOp::Xori, {R(r), R(r), I(1)});
                        define(r, x);
                        return;
                    }
                    Reg b = use(i, right, scratchB, pinned);
                    auto r = target(i, x, {{nameOf(left), a}, {nameOf(right), b}}, pinned);
                    if (isCompare(op)) {
                        if (floating) {
                            auto [mop, swap, onTrue] = floatCompare(op, bits(operandType) == 32);
                            emit(mop, {R(swap ? b : a), R(swap ? a : b)});
                            emit(MOp::Li, {R(r), I(1)});
                            emit(onTrue ? MOp::Movf : MOp::Movt, {R(r), R(reg::Zero)});
                        } else {
                            emitIntegerCompare(op, operandType, r, a, b);
                        }
                    } else if (floating) {
                        emitFloatArithmetic(op, inst.result.type, r, a, b);
                    } else {
                        emitArithmetic(op, inst.result.type, r, a, b);
                    }
                    define(r, x);
                }

                void emitUnary(size_t i, const ir::Instruction& inst) {
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    const auto id = inst.result.type;
                    const Name x = inst.result.id;
                    const bool floating = isFloat(inst.left.type);
                    Reg a = use(i, inst.left, floating ? FloatScratchA : ScratchA, pinned);
                    if (inst.opcode == Opcode::LogicalNot && floating) {
                        // !f is f == 0.0; the result is an integer register.
                        const bool single = bits(inst.left.type) == 32;
                        emit(single ? MOp::Mtc1 : MOp::Dmtc1, {R(reg::Zero), R(FloatScratchB)});
                        auto r = target(i, x, {}, pinned);
                        emit(single ? MOp::CeqS : MOp::CeqD, {R(a), R(FloatScratchB)});
                        emit(MOp::Li, {R(r), I(1)});
                        emit(MOp::Movf, {R(r), R(reg::Zero)});
                        define(r, x);
                        return;
                    }
                    auto r = target(i, x, {{nameOf(inst.left), a}}, pinned);
                    switch (inst.opcode) {
                    case Opcode::Negate:
                        if (floating) {
                            emit(bits(id) == 32 ? MOp::NegS : MOp::NegD, {R(r), R(a)});
                        } else {
                            emit(is32Signed(id) ? MOp::Subu : MOp::Dsubu, {R(r), R(reg::Zero), R(a)});
                            if (!is32Signed(id)) normalize(r, id);
                        }
                        break;
                    case Opcode::Complement:
                        emit(MOp::Nor, {R(r), R(a), R(reg::Zero)});
                        if (!isSigned(id)) normalize(r, id);
                        break;
                    default:
                        emit(MOp::Sltiu, {R(r), R(a), I(1)});
                        break;
                    }
                    define(r, x);
                }

                // ---- conversions ------------------------------------------------

                // Whether a canonical `from` value is already canonical as `to`.
                bool representationPreserved(ir::TypeId from, ir::TypeId to) const {
                    const auto& source = type(from);
                    const auto& destination = type(to);
                    auto integral = [](const ir::Type& t) {
                        return t.kind == TypeKind::Integer || t.kind == TypeKind::Pointer ||
                               t.kind == TypeKind::Function;
                    };
                    if (!integral(source) || !integral(destination)) return false;
                    if (destination.kind == TypeKind::Integer && destination.bits == 1) {
                        return source.kind == TypeKind::Integer && source.bits == 1;
                    }
                    const auto fromBits = bits(from), toBits = bits(to);
                    if (toBits >= 64) return true;
                    if (fromBits < toBits) return isSigned(from) == isSigned(to) || !isSigned(from);
                    return fromBits == toBits && isSigned(from) == isSigned(to);
                }

                void emitCast(size_t i, const ir::Instruction& inst) {
                    const auto from = inst.left.type, to = inst.result.type;
                    const Name x = inst.result.id;
                    if (isAggregate(from) || isAggregate(to)) throw Unsupported("record conversion");
                    if (representationPreserved(from, to) ||
                        (isFloat(from) && isFloat(to) && bits(from) == bits(to))) {
                        share(i, x, inst.left);
                        return;
                    }
                    std::vector<Reg> pinned;
                    pinResident(inst, pinned);
                    const bool floatSource = isFloat(from), floatTarget = isFloat(to);
                    Reg a = use(i, inst.left, floatSource ? FloatScratchA : ScratchA, pinned);
                    auto r = target(i, x, {{nameOf(inst.left), a}}, pinned);
                    const bool toBool = type(to).kind == TypeKind::Integer && type(to).bits == 1;
                    if (floatSource && floatTarget) {
                        emit(bits(to) == 64 ? MOp::CvtDS : MOp::CvtSD, {R(r), R(a)});
                    } else if (floatSource && toBool) {
                        const bool single = bits(from) == 32;
                        emit(single ? MOp::Mtc1 : MOp::Dmtc1, {R(reg::Zero), R(FloatScratchB)});
                        emit(single ? MOp::CeqS : MOp::CeqD, {R(a), R(FloatScratchB)});
                        emit(MOp::Li, {R(r), I(1)});
                        emit(MOp::Movt, {R(r), R(reg::Zero)});
                    } else if (floatSource) {
                        floatToInteger(a, r, from, to);
                    } else if (floatTarget) {
                        integerToFloat(a, r, from, to);
                    } else if (toBool) {
                        emit(MOp::Sltu, {R(r), R(reg::Zero), R(a)});
                    } else {
                        if (r != a) emit(MOp::Move, {R(r), R(a)});
                        normalize(r, to);
                    }
                    define(r, x);
                }

                // Signed values (and unsigned below 64 bits, which are non-negative when
                // zero-extended) convert from a 64-bit integer. A u64 with its top bit
                // set is halved with its low bit kept for correct rounding, then doubled.
                void integerToFloat(Reg a, Reg r, ir::TypeId from, ir::TypeId to) {
                    const bool single = bits(to) == 32;
                    const auto convert = single ? MOp::CvtSL : MOp::CvtDL;
                    if (isSigned(from) || bits(from) < 64) {
                        emit(MOp::Dmtc1, {R(a), R(r)});
                        emit(convert, {R(r), R(r)});
                        return;
                    }
                    const auto large = freshLabel(), done = freshLabel();
                    emit(MOp::Dmtc1, {R(a), R(r)});
                    emitDelayed(MOp::Bltz, {R(a), MOperand::L(large)});
                    emit(convert, {R(r), R(r)});
                    emitDelayed(MOp::B, {MOperand::L(done)});
                    emit(MOp::Label, {MOperand::L(large)});
                    emit(MOp::Andi, {R(ScratchB), R(a), I(1)});
                    emit(MOp::Dsrl, {R(ScratchA), R(a), I(1)});
                    emit(MOp::Or, {R(ScratchA), R(ScratchA), R(ScratchB)});
                    emit(MOp::Dmtc1, {R(ScratchA), R(r)});
                    emit(convert, {R(r), R(r)});
                    emit(single ? MOp::AddS : MOp::AddD, {R(r), R(r), R(r)});
                    emit(MOp::Label, {MOperand::L(done)});
                }

                // trunc.l gives the signed 64-bit result. A u64 destination at or above
                // 2^63 converts value - 2^63 and sets the top bit again.
                void floatToInteger(Reg a, Reg r, ir::TypeId from, ir::TypeId to) {
                    const bool single = bits(from) == 32;
                    const auto truncate = single ? MOp::TruncLS : MOp::TruncLD;
                    if (isSigned(to) || bits(to) < 64) {
                        emit(truncate, {R(FloatScratchB), R(a)});
                        emit(MOp::Dmfc1, {R(r), R(FloatScratchB)});
                        normalize(r, to);
                        return;
                    }
                    const auto pattern = single ? uint64_t{std::bit_cast<uint32_t>(9223372036854775808.0f)}
                                                : std::bit_cast<uint64_t>(9223372036854775808.0);
                    const auto label = Pool.intern(single ? 32 : 64, pattern);
                    const auto large = freshLabel(), done = freshLabel();
                    emit(MOp::La, {R(Address), MOperand::S(label)});
                    emit(single ? MOp::Lwc1 : MOp::Ldc1, {R(FloatScratchB), MOperand::M(Address, 0)});
                    emit(single ? MOp::CleS : MOp::CleD, {R(FloatScratchB), R(a)});
                    emitDelayed(MOp::Bc1t, {MOperand::L(large)});
                    emit(truncate, {R(FloatScratchB), R(a)});
                    emit(MOp::Dmfc1, {R(r), R(FloatScratchB)});
                    emitDelayed(MOp::B, {MOperand::L(done)});
                    emit(MOp::Label, {MOperand::L(large)});
                    emit(single ? MOp::SubS : MOp::SubD, {R(FloatScratchB), R(a), R(FloatScratchB)});
                    emit(truncate, {R(FloatScratchB), R(FloatScratchB)});
                    emit(MOp::Dmfc1, {R(r), R(FloatScratchB)});
                    emit(MOp::Li, {R(ScratchA), I(1)});
                    emit(MOp::Dsll32, {R(ScratchA), R(ScratchA), I(31)});
                    emit(MOp::Or, {R(r), R(r), R(ScratchA)});
                    emit(MOp::Label, {MOperand::L(done)});
                }

                // ---- prologue and epilogue ----------------------------------------

                void finish() {
                    std::vector<Reg> saved(SavedRegisters.begin(), SavedRegisters.end());
                    if (MakesCall) saved.push_back(reg::Ra);
                    const auto saveBase = AlignUp(Cursor, 8);
                    const auto frame = AlignUp(saveBase + 8 * static_cast<int64_t>(saved.size()), 16);
                    std::vector<MInst> prologue;
                    std::swap(prologue, Body);
                    adjustStack(-frame);
                    for (size_t k = 0; k < saved.size(); ++k) {
                        emit(reg::IsFloat(saved[k]) ? MOp::Sdc1 : MOp::Sd,
                             {R(saved[k]), slot(saveBase + 8 * static_cast<int64_t>(k))});
                    }
                    storeParameters(frame);
                    std::swap(prologue, Body);
                    std::vector<MInst> code = std::move(prologue);
                    code.insert(code.end(), Body.begin(), Body.end());
                    Body.clear();
                    auto liveAtReturn = LiveAtBlockStart;
                    liveAtReturn.insert(liveAtReturn.end(), {reg::V0, reg::F(0)});
                    Body.push_back({MOp::Label, {MOperand::L(returnLabel())}, liveAtReturn, {}, {}});
                    for (size_t k = 0; k < saved.size(); ++k) {
                        emit(reg::IsFloat(saved[k]) ? MOp::Ldc1 : MOp::Ld,
                             {R(saved[k]), slot(saveBase + 8 * static_cast<int64_t>(k))});
                    }
                    adjustStack(frame);
                    std::vector<Reg> returned{reg::Ra, reg::Sp, reg::V0, reg::F(0)};
                    for (auto r : saved) returned.push_back(r);
                    Body.push_back({MOp::Jr, {R(reg::Ra)}, returned, {}, {}});
                    emit(MOp::Nop);
                    code.insert(code.end(), Body.begin(), Body.end());
                    Result.code = std::move(code);
                }

                void adjustStack(int64_t delta) {
                    if (delta == 0) return;
                    if (FitsSigned16(delta)) {
                        emit(MOp::Daddiu, {R(reg::Sp), R(reg::Sp), I(delta)});
                    } else {
                        emit(MOp::Li, {R(Address), I(delta)});
                        emit(MOp::Daddu, {R(reg::Sp), R(reg::Sp), R(Address)});
                    }
                }

                // Incoming arguments are stored to their parameter objects; a record
                // argument is a pointer to the caller's snapshot and is copied.
                void storeParameters(int64_t frame) {
                    const auto& signature = type(Function.signature);
                    size_t slotIndex = 0;
                    if (SretSlot >= 0) {
                        emit(MOp::Sd, {R(reg::A0), slot(SretSlot)});
                        slotIndex = 1;
                    }
                    for (size_t j = 0; j < Function.parameters.size(); ++j, ++slotIndex) {
                        const auto& parameter = object(Function.parameters[j]);
                        const auto id = parameter.type;
                        const auto destination = ObjectSlots.at(parameter.id);
                        const bool aggregate = isAggregate(id);
                        const bool inFloat = !aggregate && isFloat(id) && j < signature.parameters.size();
                        Reg source;
                        if (slotIndex < 8) {
                            source = inFloat ? reg::F(12 + static_cast<unsigned>(slotIndex))
                                             : static_cast<Reg>(reg::A0 + slotIndex);
                        } else {
                            source = inFloat ? FloatScratchA : ScratchB;
                            const auto incoming = frame + static_cast<int64_t>((slotIndex - 8) * 8);
                            emit(inFloat ? (bits(id) == 32 ? MOp::Lwc1 : MOp::Ldc1) : MOp::Ld,
                                 {R(source), slot(incoming)});
                        }
                        if (aggregate) {
                            copyBytes(MOperand::M(reg::Sp, destination), MOperand::M(source, 0), sizeOf(id),
                                      alignOf(id), false);
                        } else {
                            emit(storeOp(id), {R(source), slot(destination)});
                        }
                    }
                }
            };
        }

        MFunction SelectFunction(const ir::Module& module, const ir::Function& function, LiteralPool& pool) {
            return Selector(module, function, pool).run();
        }
    }
}
