#include "internal.hh"
#include <algorithm>
#include <bitset>
#include <limits>
#include <map>
#include <optional>
#include <set>

namespace baka {
    namespace codegen {
        namespace {
            // Every transformation is local to a straight-line segment: it never looks past a
            // Label or past a branch/jr (with its delay slot). Delay slots hold `nop` until
            // the final fill pass, so the "branch + exactly one slot instruction" shape that
            // instruction selection guarantees is preserved by every earlier rule.
            using Code = std::vector<MInst>;
            using Bits = std::bitset<reg::Count>;
            constexpr size_t npos = std::numeric_limits<size_t>::max();

            using Kind = MOperand::Kind;

            bool Has(const std::vector<Reg>& regs, Reg r) {
                return std::ranges::find(regs, r) != regs.end();
            }

            bool EndsBlock(const MInst& in) {
                return Info(in.op).branch || Info(in.op).ret;
            }

            size_t NextReal(const Code& c, size_t i) {
                for (size_t j = i + 1; j < c.size(); ++j) {
                    if (c[j].op != MOp::Comment) {
                        return j;
                    }
                }
                return npos;
            }

            size_t PrevReal(const Code& c, size_t i) {
                for (size_t j = i; j-- > 0;) {
                    if (c[j].op != MOp::Comment) {
                        return j;
                    }
                }
                return npos;
            }

            const MOperand* MemOperand(const MInst& in) {
                for (const auto& o : in.operands) {
                    if (o.kind == Kind::Memory) {
                        return &o;
                    }
                }
                return nullptr;
            }

            int Width(MOp op) {
                switch (op) {
                case MOp::Lb: case MOp::Lbu: case MOp::Sb: return 1;
                case MOp::Lh: case MOp::Lhu: case MOp::Sh: return 2;
                case MOp::Lw: case MOp::Lwu: case MOp::Lwc1: case MOp::Sw: case MOp::Swc1: return 4;
                default: return 8;
                }
            }

            // The load that reads back exactly the value a store wrote from its register
            // (sw/lw is excluded: lw sign-extends, the register may not be).
            std::optional<MOp> ReloadOf(MOp store) {
                switch (store) {
                case MOp::Sd: return MOp::Ld;
                case MOp::Sdc1: return MOp::Ldc1;
                case MOp::Swc1: return MOp::Lwc1;
                default: return std::nullopt;
                }
            }

            MOp CopyFor(MOp load) {
                return load == MOp::Ldc1 ? MOp::MovD : load == MOp::Lwc1 ? MOp::MovS : MOp::Move;
            }

            MInst Make(MOp op, std::vector<MOperand> operands) {
                MInst in;
                in.op = op;
                in.operands = std::move(operands);
                return in;
            }

            // Registers live at function exit: the return-value registers named by jr, the
            // stack/link registers and everything the callee must preserve.
            Bits ReturnLive(const MInst& jr) {
                Bits live;
                for (Reg r : Uses(jr)) {
                    live.set(r);
                }
                for (Reg r = 0; r < reg::HiLo; ++r) {
                    if (reg::IsCalleeSaved(r)) {
                        live.set(r);
                    }
                }
                live.set(reg::Sp);
                live.set(reg::Ra);
                return live;
            }

            // liveAfter[i]: registers that may still be read after instruction i. A Label with
            // non-empty implicitUses is live-in exactly that set; an unhinted label, the end of
            // code, or an unresolvable branch target means everything is live. After a delay slot:
            // jr gives the return set, `b L` gives live(L), a conditional branch live(L) plus the
            // fallthrough. $sp is always live.
            std::vector<Bits> LiveAfter(const Code& c) {
                const size_t n = c.size();
                Bits all;
                all.set();
                std::map<std::string, Bits> hints;
                for (const auto& in : c) {
                    if (in.op == MOp::Label && !in.implicitUses.empty()) {
                        Bits h;
                        for (Reg r : in.implicitUses) {
                            h.set(r);
                        }
                        h.set(reg::Sp);
                        hints[in.operands[0].name] = h;
                    }
                }
                std::vector<Bits> after(n);
                std::vector<size_t> owner(n, npos);
                size_t prev = npos;
                for (size_t i = 0; i < n; ++i) {
                    if (c[i].op == MOp::Comment) {
                        continue;
                    }
                    if (prev != npos && Info(c[prev].op).hasDelaySlot) {
                        owner[i] = prev;
                    }
                    prev = i;
                }
                Bits cur = all;
                size_t next = npos;
                for (size_t i = n; i-- > 0;) {
                    if (c[i].op == MOp::Comment) {
                        after[i] = cur;
                        continue;
                    }
                    Bits out = next == npos ? all : cur;
                    if (owner[i] != npos) {
                        const auto& br = c[owner[i]];
                        if (Info(br.op).ret) {
                            out = ReturnLive(br);
                        } else if (Info(br.op).branch) {
                            const std::string* target = BranchTarget(br);
                            const auto it = target ? hints.find(*target) : hints.end();
                            const Bits taken = it == hints.end() ? all : it->second;
                            out = br.op == MOp::B ? taken : (taken | out);
                        }
                    }
                    after[i] = out;
                    Bits in = out;
                    if (c[i].op == MOp::Label) {
                        const auto it = hints.find(c[i].operands[0].name);
                        in = it == hints.end() ? all : it->second;
                    } else {
                        for (Reg d : Defs(c[i])) {
                            in.reset(d);
                        }
                        for (Reg u : Uses(c[i])) {
                            in.set(u);
                        }
                        in.set(reg::Sp);
                    }
                    cur = in;
                    next = i;
                }
                return after;
            }

            // First instruction after i in the segment that reads r, provided r (and guard,
            // if given) is not redefined before it.
            size_t FirstUse(const Code& c, size_t i, Reg r, Reg guard = reg::None) {
                for (size_t j = i + 1; j < c.size(); ++j) {
                    const auto& in = c[j];
                    if (in.op == MOp::Label) {
                        return npos;
                    }
                    if (in.op == MOp::Comment) {
                        continue;
                    }
                    if (Has(Uses(in), r)) {
                        return j;
                    }
                    const auto defs = Defs(in);
                    if (Has(defs, r) || (guard != reg::None && Has(defs, guard)) || EndsBlock(in)) {
                        return npos;
                    }
                }
                return npos;
            }

            bool Fits16(int64_t v) {
                return v >= -32768 && v <= 32767;
            }

            int Log2(int64_t k) {
                if (k <= 0 || (k & (k - 1)) != 0) {
                    return -1;
                }
                int n = 0;
                while ((int64_t{1} << n) != k) {
                    ++n;
                }
                return n;
            }

            // Rule 1: a reload of a $sp slot whose value is still in a register.
            bool ForwardLoads(Code& c) {
                struct Fact {
                    int64_t off;
                    int width;
                    MOp load;
                    Reg holder;
                };
                bool changed = false;
                std::vector<Fact> facts;
                for (size_t i = 0; i < c.size();) {
                    const auto& info = Info(c[i].op);
                    if (c[i].op == MOp::Label || EndsBlock(c[i])) {
                        facts.clear();
                        ++i;
                        continue;
                    }
                    if (c[i].op == MOp::Comment) {
                        ++i;
                        continue;
                    }
                    const MOp op = c[i].op;
                    const MOperand* mem = MemOperand(c[i]);
                    const bool spMem = mem && mem->reg == reg::Sp;
                    const int64_t off = mem ? mem->imm : 0;
                    const Reg first = !c[i].operands.empty() ? c[i].operands[0].reg : reg::None;
                    if (info.load && spMem) {
                        auto it = std::ranges::find_if(facts, [&](const Fact& f) { return f.off == off && f.load == op; });
                        if (it != facts.end()) {
                            if (it->holder == first) {
                                c.erase(c.begin() + i);
                                changed = true;
                                continue;
                            }
                            std::string comment = std::move(c[i].comment);
                            c[i] = Make(CopyFor(op), {MOperand::R(first), MOperand::R(it->holder)});
                            c[i].comment = std::move(comment);
                            changed = true;
                        }
                    }
                    const auto defs = Defs(c[i]);
                    for (Reg d : defs) {
                        std::erase_if(facts, [&](const Fact& f) { return f.holder == d; });
                    }
                    if (Has(defs, reg::Sp) || info.call) {
                        facts.clear();
                    }
                    if (info.store) {
                        if (spMem) {
                            const int w = Width(op);
                            std::erase_if(facts, [&](const Fact& f) { return f.off < off + w && off < f.off + f.width; });
                            if (auto ld = ReloadOf(op)) {
                                facts.push_back({off, w, *ld, first});
                            }
                        } else {
                            facts.clear();
                        }
                    } else if (info.load && spMem) {
                        facts.push_back({off, Width(op), op, first});
                    }
                    ++i;
                }
                return changed;
            }

            // Rule 2: a $sp store overwritten by a covering store with no possible read between.
            bool DeadStores(Code& c) {
                bool changed = false;
                for (size_t i = 0; i < c.size();) {
                    const MOperand* mem = MemOperand(c[i]);
                    bool removed = false;
                    if (Info(c[i].op).store && mem && mem->reg == reg::Sp) {
                        const int64_t off = mem->imm;
                        const int w = Width(c[i].op);
                        for (size_t j = i + 1; j < c.size() && !removed; ++j) {
                            const auto& in = c[j];
                            const auto& info = Info(in.op);
                            if (in.op == MOp::Comment) {
                                continue;
                            }
                            if (in.op == MOp::Label || EndsBlock(in) || info.call || Has(Defs(in), reg::Sp)) {
                                break;
                            }
                            const MOperand* m = MemOperand(in);
                            if (info.load && (!m || m->reg != reg::Sp || (m->imm < off + w && off < m->imm + Width(in.op)))) {
                                break;
                            }
                            if (info.store && m && m->reg == reg::Sp && m->imm <= off && m->imm + Width(in.op) >= off + w) {
                                removed = true;
                            }
                        }
                    }
                    if (removed) {
                        c.erase(c.begin() + i);
                        changed = true;
                    } else {
                        ++i;
                    }
                }
                return changed;
            }

            // Rule 3: side-effect-free instruction whose results are all dead. Nop (delay
            // slot filler) defines nothing and is therefore never removed here.
            bool DeadCode(Code& c) {
                bool changed = false;
                auto live = LiveAfter(c);
                for (size_t i = c.size(); i-- > 0;) {
                    const auto& info = Info(c[i].op);
                    if (c[i].op == MOp::Label || c[i].op == MOp::Comment || info.store || info.branch || info.call || info.ret) {
                        continue;
                    }
                    const auto defs = Defs(c[i]);
                    if (defs.empty() || std::ranges::any_of(defs, [&](Reg d) { return live[i][d]; })) {
                        continue;
                    }
                    c.erase(c.begin() + i);
                    changed = true;
                    live = LiveAfter(c);
                }
                return changed;
            }

            // Rule 2b: `I: op b, ...; move a, b` with b dead afterwards makes I define a directly.
            bool RetargetCopies(Code& c) {
                bool changed = false;
                auto live = LiveAfter(c);
                for (size_t i = 0; i < c.size();) {
                    const auto& info = Info(c[i].op);
                    const size_t j = NextReal(c, i);
                    bool done = false;
                    if (c[i].op != MOp::Label && c[i].op != MOp::Comment && c[i].op != MOp::Nop && info.definesFirst &&
                        !info.readsFirst && !info.store && !info.branch && !info.call && !info.ret && j != npos &&
                        (c[j].op == MOp::Move || c[j].op == MOp::MovD || c[j].op == MOp::MovS)) {
                        const auto defs = Defs(c[i]);
                        const Reg b = c[i].operands[0].reg, a = c[j].operands[0].reg;
                        if (defs.size() == 1 && defs[0] == b && c[j].operands[1].reg == b && a != b &&
                            a != reg::Zero && a != reg::Sp && reg::IsFloat(a) == reg::IsFloat(b) && !live[j][b]) {
                            c[i].operands[0].reg = a;
                            c.erase(c.begin() + j);
                            changed = done = true;
                            live = LiveAfter(c);
                        }
                    }
                    if (!done) {
                        ++i;
                    }
                }
                return changed;
            }

            // Rule 4: instructions that compute their own destination's value.
            bool IsNoop(const MInst& in) {
                const auto& o = in.operands;
                switch (in.op) {
                case MOp::Move: case MOp::MovD: case MOp::MovS:
                    return o[0].reg == o[1].reg;
                case MOp::Daddiu:
                    return o[0].reg == o[1].reg && o[2].imm == 0;
                case MOp::Or: case MOp::Daddu:
                    return (o[0].reg == o[1].reg && o[2].reg == reg::Zero) || (o[0].reg == o[2].reg && o[1].reg == reg::Zero);
                default:
                    return false;
                }
            }

            bool RemoveNoops(Code& c) {
                return std::erase_if(c, IsNoop) > 0;
            }

            bool Substitute(MInst& in, Reg a, Reg b) {
                const auto& info = Info(in.op);
                bool changed = false;
                for (size_t k = 0; k < in.operands.size(); ++k) {
                    auto& o = in.operands[k];
                    const bool written = (k == 0 && info.definesFirst) || (k == 1 && info.definesSecond);
                    if (((o.kind == Kind::Register && !written) || o.kind == Kind::Memory) && o.reg == a) {
                        o.reg = b;
                        changed = true;
                    }
                }
                return changed;
            }

            // Rule 5: uses of `a` after `move a, b` read `b` while neither is redefined.
            bool CopyPropagate(Code& c) {
                bool changed = false;
                for (size_t i = 0; i < c.size(); ++i) {
                    if (c[i].op != MOp::Move && c[i].op != MOp::MovD) {
                        continue;
                    }
                    const Reg a = c[i].operands[0].reg, b = c[i].operands[1].reg;
                    if (a == b || a == reg::Zero || b == reg::Zero || a == reg::Sp || b == reg::Sp) {
                        continue;
                    }
                    for (size_t j = i + 1; j < c.size(); ++j) {
                        auto& in = c[j];
                        if (in.op == MOp::Label) {
                            break;
                        }
                        if (in.op == MOp::Comment) {
                            continue;
                        }
                        if (in.op != MOp::Jr && in.op != MOp::Jalr) {
                            changed |= Substitute(in, a, b);
                        }
                        const auto defs = Defs(in);
                        if (Has(defs, a) || Has(defs, b) || EndsBlock(in)) {
                            break;
                        }
                    }
                }
                return changed;
            }

            // Rule 6: `daddiu t, b, k; op r, j(t)` becomes `op r, k+j(b)`; the daddiu then dies.
            bool FoldAddresses(Code& c) {
                bool changed = false;
                auto live = LiveAfter(c);
                for (size_t i = 0; i < c.size(); ++i) {
                    if (c[i].op != MOp::Daddiu) {
                        continue;
                    }
                    const Reg t = c[i].operands[0].reg, b = c[i].operands[1].reg;
                    const int64_t k = c[i].operands[2].imm;
                    if (t == b || t == reg::Zero || t == reg::Sp) {
                        continue;
                    }
                    const size_t j = FirstUse(c, i, t, b);
                    if (j == npos) {
                        continue;
                    }
                    auto& u = c[j];
                    const auto& info = Info(u.op);
                    if (!(info.load || info.store) || u.operands.size() != 2 || u.operands[1].kind != Kind::Memory ||
                        u.operands[1].reg != t || (info.store && u.operands[0].reg == t)) {
                        continue;
                    }
                    if (live[j][t] && !Has(Defs(u), t)) {
                        continue;
                    }
                    if (!Fits16(k + u.operands[1].imm)) {
                        continue;
                    }
                    u.operands[1].imm += k;
                    u.operands[1].reg = b;
                    changed = true;
                    live = LiveAfter(c);
                }
                return changed;
            }

            // Rule 7 helper: rewrite u (reading constant t = k) into its immediate form.
            bool ToImmediate(MInst& u, Reg t, int64_t k) {
                if (u.operands.size() != 3 || u.operands[1].kind != Kind::Register || u.operands[2].kind != Kind::Register) {
                    return false;
                }
                const bool first = u.operands[1].reg == t, second = u.operands[2].reg == t;
                if (first == second) {
                    return false;
                }
                MOp to;
                int64_t v = k;
                bool commutative = false, unsignedImm = false;
                switch (u.op) {
                case MOp::Addu: to = MOp::Addiu; commutative = true; break;
                case MOp::Daddu: to = MOp::Daddiu; commutative = true; break;
                case MOp::And: to = MOp::Andi; commutative = unsignedImm = true; break;
                case MOp::Or: to = MOp::Ori; commutative = unsignedImm = true; break;
                case MOp::Xor: to = MOp::Xori; commutative = unsignedImm = true; break;
                case MOp::Slt: to = MOp::Slti; break;
                case MOp::Sltu: to = MOp::Sltiu; break;
                case MOp::Subu: to = MOp::Addiu; v = -k; break;
                case MOp::Dsubu: to = MOp::Daddiu; v = -k; break;
                default: return false;
                }
                if (first && !commutative) {
                    return false;
                }
                if (unsignedImm ? (v < 0 || v > 65535) : !Fits16(v)) {
                    return false;
                }
                const Reg s = first ? u.operands[2].reg : u.operands[1].reg;
                u.operands = {u.operands[0], MOperand::R(s), MOperand::I(v)};
                u.op = to;
                return true;
            }

            bool FoldImmediates(Code& c) {
                bool changed = false;
                auto live = LiveAfter(c);
                for (size_t i = 0; i < c.size(); ++i) {
                    if (c[i].op != MOp::Li || c[i].operands[0].reg == reg::Zero) {
                        continue;
                    }
                    const Reg t = c[i].operands[0].reg;
                    const size_t j = FirstUse(c, i, t);
                    if (j == npos || (live[j][t] && !Has(Defs(c[j]), t))) {
                        continue;
                    }
                    if (ToImmediate(c[j], t, c[i].operands[1].imm)) {
                        changed = true;
                        live = LiveAfter(c);
                    }
                }
                return changed;
            }

            // Rule 8: multiplication by a constant power of two becomes a shift.
            bool ReduceStrength(Code& c) {
                bool changed = false;
                auto live = LiveAfter(c);
                for (size_t i = 0; i < c.size(); ++i) {
                    if (c[i].op != MOp::Li || c[i].operands[0].reg == reg::Zero) {
                        continue;
                    }
                    const Reg t = c[i].operands[0].reg;
                    const int n = Log2(c[i].operands[1].imm);
                    const size_t j = n < 0 ? npos : FirstUse(c, i, t);
                    if (j == npos) {
                        continue;
                    }
                    auto& u = c[j];
                    if (u.operands.size() < 2 || u.operands[0].kind != Kind::Register || u.operands[1].kind != Kind::Register) {
                        continue;
                    }
                    if (u.op == MOp::Mul) {
                        if (u.operands.size() != 3 || u.operands[2].kind != Kind::Register) {
                            continue;
                        }
                        const bool a = u.operands[1].reg == t, b = u.operands[2].reg == t;
                        if (a == b || n > 31 || (live[j][t] && !Has(Defs(u), t))) {
                            continue;
                        }
                        const Reg s = a ? u.operands[2].reg : u.operands[1].reg;
                        u = Make(MOp::Sll, {u.operands[0], MOperand::R(s), MOperand::I(n)});
                        changed = true;
                        live = LiveAfter(c);
                        continue;
                    }
                    const bool wide = u.op == MOp::Dmult || u.op == MOp::Dmultu;
                    if (!(wide || u.op == MOp::Mult || u.op == MOp::Multu) || u.operands.size() != 2 || live[j][t]) {
                        continue;
                    }
                    const bool a = u.operands[0].reg == t, b = u.operands[1].reg == t;
                    const size_t m = NextReal(c, j);
                    if (a == b || m == npos || c[m].op != MOp::Mflo || live[m][reg::HiLo] || n > (wide ? 62 : 31)) {
                        continue;
                    }
                    const Reg s = a ? u.operands[1].reg : u.operands[0].reg;
                    const MOperand x = c[m].operands[0];
                    const MOp shift = !wide ? MOp::Sll : n < 32 ? MOp::Dsll : MOp::Dsll32;
                    u = Make(shift, {x, MOperand::R(s), MOperand::I(wide && n >= 32 ? n - 32 : n)});
                    c.erase(c.begin() + m);
                    changed = true;
                    live = LiveAfter(c);
                }
                return changed;
            }

            std::optional<MOp> Inverse(MOp op) {
                switch (op) {
                case MOp::Beq: return MOp::Bne;
                case MOp::Bne: return MOp::Beq;
                case MOp::Blez: return MOp::Bgtz;
                case MOp::Bgtz: return MOp::Blez;
                case MOp::Bltz: return MOp::Bgez;
                case MOp::Bgez: return MOp::Bltz;
                case MOp::Bc1t: return MOp::Bc1f;
                case MOp::Bc1f: return MOp::Bc1t;
                default: return std::nullopt;
                }
            }

            // Is `name` among the labels that start at `from` (comments skipped)?
            bool LabelFollows(const Code& c, size_t from, const std::string& name) {
                for (size_t j = from; j < c.size(); ++j) {
                    if (c[j].op == MOp::Comment) {
                        continue;
                    }
                    if (c[j].op != MOp::Label) {
                        return false;
                    }
                    if (c[j].operands[0].name == name) {
                        return true;
                    }
                }
                return false;
            }

            // Rule 9.
            bool ControlFlow(Code& c) {
                bool changed = false;
                std::set<std::string> referenced;
                for (const auto& in : c) {
                    if (in.op == MOp::Label) {
                        continue;
                    }
                    for (const auto& o : in.operands) {
                        if (o.kind == Kind::Label || o.kind == Kind::Symbol) {
                            referenced.insert(o.name);
                        }
                    }
                }
                changed |= std::erase_if(c, [&](const MInst& in) {
                    return in.op == MOp::Label && in.operands[0].name.starts_with(".L") && !referenced.contains(in.operands[0].name);
                }) > 0;

                for (size_t i = 0; i < c.size();) {
                    const size_t s = Info(c[i].op).branch ? NextReal(c, i) : npos;
                    if (s == npos || c[s].op != MOp::Nop) {
                        ++i;
                        continue;
                    }
                    const std::string* target = BranchTarget(c[i]);
                    if (target && LabelFollows(c, s + 1, *target)) {
                        c.erase(c.begin() + s);
                        c.erase(c.begin() + i);
                        changed = true;
                        continue;
                    }
                    const auto inverse = Inverse(c[i].op);
                    const size_t b = inverse && target ? NextReal(c, s) : npos;
                    const size_t s2 = b != npos && c[b].op == MOp::B ? NextReal(c, b) : npos;
                    if (s2 != npos && c[s2].op == MOp::Nop && BranchTarget(c[b]) && LabelFollows(c, s2 + 1, *target)) {
                        const std::string destination = *BranchTarget(c[b]);
                        for (auto& o : c[i].operands) {
                            if (o.kind == Kind::Label) {
                                o.name = destination;
                            }
                        }
                        c[i].op = *inverse;
                        c.erase(c.begin() + s2);
                        c.erase(c.begin() + b);
                        changed = true;
                        continue;
                    }
                    ++i;
                }

                // Code after an unconditional transfer is unreachable up to the next label.
                for (size_t i = 0; i < c.size(); ++i) {
                    if (c[i].op != MOp::B && c[i].op != MOp::Jr) {
                        continue;
                    }
                    size_t j = NextReal(c, i);
                    if (j == npos) {
                        break;
                    }
                    for (++j; j < c.size() && c[j].op != MOp::Label;) {
                        if (c[j].op == MOp::Comment) {
                            ++j;
                        } else {
                            c.erase(c.begin() + j);
                            changed = true;
                        }
                    }
                }
                return changed;
            }

            // Whether the instruction at p may be moved into the delay slot of the branch at br. Rejects
            // pseudo-ops, multi-instruction expansions (li/la), hi/lo users, control transfers, and anything
            // following a label or another delay-slot owner; also rejects defs that the branch reads early.
            bool CanFill(const Code& c, size_t p, size_t br) {
                const auto& in = c[p];
                const auto& info = Info(in.op);
                switch (in.op) {
                case MOp::Label: case MOp::Comment: case MOp::Nop: case MOp::Li: case MOp::La:
                case MOp::Mult: case MOp::Multu: case MOp::Dmult: case MOp::Dmultu:
                case MOp::Div: case MOp::Divu: case MOp::Ddiv: case MOp::Ddivu:
                case MOp::Mflo: case MOp::Mfhi:
                    return false;
                default:
                    break;
                }
                if (info.hasDelaySlot || info.branch || info.call || info.ret) {
                    return false;
                }
                const size_t q = PrevReal(c, p);
                if (q != npos && (c[q].op == MOp::Label || Info(c[q].op).hasDelaySlot)) {
                    return false;
                }
                // The branch reads its operands before the slot runs; call/return implicit
                // uses (arguments, $v0) are consumed after the slot, so they may be defined.
                const auto defs = Defs(in);
                for (const auto& o : c[br].operands) {
                    if (o.kind == Kind::Register && Has(defs, o.reg)) {
                        return false;
                    }
                }
                if (c[br].op == MOp::Bc1t || c[br].op == MOp::Bc1f) {
                    if (Has(defs, reg::Fcc)) {
                        return false;
                    }
                }
                // jalr writes $ra before its slot executes; jr reads it.
                if (c[br].op == MOp::Jalr || c[br].op == MOp::Jr) {
                    if (Has(defs, reg::Ra) || Has(Uses(in), reg::Ra)) {
                        return false;
                    }
                }
                return true;
            }

            // Rule 10, run once at the end.
            void FillDelaySlots(Code& c) {
                for (size_t i = 0; i < c.size(); ++i) {
                    if (!Info(c[i].op).hasDelaySlot) {
                        continue;
                    }
                    const size_t s = NextReal(c, i);
                    const size_t p = PrevReal(c, i);
                    if (s == npos || p == npos || c[s].op != MOp::Nop || !CanFill(c, p, i)) {
                        continue;
                    }
                    c[s] = std::move(c[p]);
                    c.erase(c.begin() + p);
                    --i;
                }
            }
        }

        void Peephole(MFunction& function) {
            auto& c = function.code;
            for (int round = 0; round < 32; ++round) {
                bool changed = false;
                changed |= ControlFlow(c);
                changed |= CopyPropagate(c);
                changed |= ForwardLoads(c);
                changed |= FoldAddresses(c);
                changed |= FoldImmediates(c);
                changed |= ReduceStrength(c);
                changed |= RemoveNoops(c);
                changed |= RetargetCopies(c);
                changed |= DeadStores(c);
                changed |= DeadCode(c);
                if (!changed) {
                    break;
                }
            }
            FillDelaySlots(c);
        }
    }
}
