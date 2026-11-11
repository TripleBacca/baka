#include "mips.hh"
#include <array>

namespace baka {
    namespace codegen {
        namespace {
            // Field order: mnemonic, definesFirst, definesSecond, readsFirst, load,
            // store, branch, call, ret, hasDelaySlot.
            constexpr OpInfo Def(const char* name) {
                return {name, true};
            }

            constexpr OpInfo Plain(const char* name) {
                return {name};
            }

            constexpr OpInfo Load(const char* name) {
                return {name, true, false, false, true};
            }

            constexpr OpInfo Store(const char* name) {
                return {name, false, false, false, false, true};
            }

            constexpr OpInfo Branch(const char* name) {
                return {name, false, false, false, false, false, true, false, false, true};
            }

            const OpInfo& Lookup(MOp op) {
                static const std::array<OpInfo, static_cast<size_t>(MOp::Movt) + 1> table = [] {
                    std::array<OpInfo, static_cast<size_t>(MOp::Movt) + 1> t{};
                    auto set = [&](MOp op, OpInfo info) { t[static_cast<size_t>(op)] = info; };
                    set(MOp::Label, Plain(""));
                    set(MOp::Comment, Plain(""));
                    set(MOp::Addu, Def("addu"));
                    set(MOp::Daddu, Def("daddu"));
                    set(MOp::Subu, Def("subu"));
                    set(MOp::Dsubu, Def("dsubu"));
                    set(MOp::And, Def("and"));
                    set(MOp::Or, Def("or"));
                    set(MOp::Xor, Def("xor"));
                    set(MOp::Nor, Def("nor"));
                    set(MOp::Slt, Def("slt"));
                    set(MOp::Sltu, Def("sltu"));
                    set(MOp::Sllv, Def("sllv"));
                    set(MOp::Srlv, Def("srlv"));
                    set(MOp::Srav, Def("srav"));
                    set(MOp::Dsllv, Def("dsllv"));
                    set(MOp::Dsrlv, Def("dsrlv"));
                    set(MOp::Dsrav, Def("dsrav"));
                    set(MOp::Mul, Def("mul"));
                    set(MOp::Movn, {"movn", true, false, true});
                    set(MOp::Movz, {"movz", true, false, true});
                    set(MOp::Addiu, Def("addiu"));
                    set(MOp::Daddiu, Def("daddiu"));
                    set(MOp::Andi, Def("andi"));
                    set(MOp::Ori, Def("ori"));
                    set(MOp::Xori, Def("xori"));
                    set(MOp::Slti, Def("slti"));
                    set(MOp::Sltiu, Def("sltiu"));
                    set(MOp::Sll, Def("sll"));
                    set(MOp::Srl, Def("srl"));
                    set(MOp::Sra, Def("sra"));
                    set(MOp::Dsll, Def("dsll"));
                    set(MOp::Dsrl, Def("dsrl"));
                    set(MOp::Dsra, Def("dsra"));
                    set(MOp::Dsll32, Def("dsll32"));
                    set(MOp::Dsrl32, Def("dsrl32"));
                    set(MOp::Dsra32, Def("dsra32"));
                    set(MOp::Move, Def("move"));
                    set(MOp::Seb, Def("seb"));
                    set(MOp::Seh, Def("seh"));
                    set(MOp::Dext, Def("dext"));
                    set(MOp::Lui, Def("lui"));
                    set(MOp::Li, Def("dli"));
                    set(MOp::La, Def("dla"));
                    set(MOp::Mult, Plain("mult"));
                    set(MOp::Multu, Plain("multu"));
                    set(MOp::Dmult, Plain("dmult"));
                    set(MOp::Dmultu, Plain("dmultu"));
                    set(MOp::Div, Plain("div"));
                    set(MOp::Divu, Plain("divu"));
                    set(MOp::Ddiv, Plain("ddiv"));
                    set(MOp::Ddivu, Plain("ddivu"));
                    set(MOp::Mflo, Def("mflo"));
                    set(MOp::Mfhi, Def("mfhi"));
                    set(MOp::Lb, Load("lb"));
                    set(MOp::Lbu, Load("lbu"));
                    set(MOp::Lh, Load("lh"));
                    set(MOp::Lhu, Load("lhu"));
                    set(MOp::Lw, Load("lw"));
                    set(MOp::Lwu, Load("lwu"));
                    set(MOp::Ld, Load("ld"));
                    set(MOp::Lwc1, Load("lwc1"));
                    set(MOp::Ldc1, Load("ldc1"));
                    set(MOp::Sb, Store("sb"));
                    set(MOp::Sh, Store("sh"));
                    set(MOp::Sw, Store("sw"));
                    set(MOp::Sd, Store("sd"));
                    set(MOp::Swc1, Store("swc1"));
                    set(MOp::Sdc1, Store("sdc1"));
                    set(MOp::Beq, Branch("beq"));
                    set(MOp::Bne, Branch("bne"));
                    set(MOp::Blez, Branch("blez"));
                    set(MOp::Bgtz, Branch("bgtz"));
                    set(MOp::Bltz, Branch("bltz"));
                    set(MOp::Bgez, Branch("bgez"));
                    set(MOp::B, Branch("b"));
                    set(MOp::Bc1t, Branch("bc1t"));
                    set(MOp::Bc1f, Branch("bc1f"));
                    set(MOp::Jr, {"jr", false, false, false, false, false, false, false, true, true});
                    set(MOp::Jalr, {"jalr", false, false, false, false, false, false, true, false, true});
                    set(MOp::Nop, Plain("nop"));
                    set(MOp::AddS, Def("add.s"));
                    set(MOp::AddD, Def("add.d"));
                    set(MOp::SubS, Def("sub.s"));
                    set(MOp::SubD, Def("sub.d"));
                    set(MOp::MulS, Def("mul.s"));
                    set(MOp::MulD, Def("mul.d"));
                    set(MOp::DivS, Def("div.s"));
                    set(MOp::DivD, Def("div.d"));
                    set(MOp::NegS, Def("neg.s"));
                    set(MOp::NegD, Def("neg.d"));
                    set(MOp::MovS, Def("mov.s"));
                    set(MOp::MovD, Def("mov.d"));
                    set(MOp::CvtDS, Def("cvt.d.s"));
                    set(MOp::CvtSD, Def("cvt.s.d"));
                    set(MOp::CvtDL, Def("cvt.d.l"));
                    set(MOp::CvtSL, Def("cvt.s.l"));
                    set(MOp::CvtDW, Def("cvt.d.w"));
                    set(MOp::CvtSW, Def("cvt.s.w"));
                    set(MOp::TruncLD, Def("trunc.l.d"));
                    set(MOp::TruncLS, Def("trunc.l.s"));
                    set(MOp::TruncWD, Def("trunc.w.d"));
                    set(MOp::TruncWS, Def("trunc.w.s"));
                    set(MOp::CeqS, Plain("c.eq.s"));
                    set(MOp::CeqD, Plain("c.eq.d"));
                    set(MOp::CltS, Plain("c.lt.s"));
                    set(MOp::CltD, Plain("c.lt.d"));
                    set(MOp::CleS, Plain("c.le.s"));
                    set(MOp::CleD, Plain("c.le.d"));
                    set(MOp::Mfc1, Def("mfc1"));
                    set(MOp::Dmfc1, Def("dmfc1"));
                    set(MOp::Mtc1, {"mtc1", false, true});
                    set(MOp::Dmtc1, {"dmtc1", false, true});
                    set(MOp::Movf, {"movf", true, false, true});
                    set(MOp::Movt, {"movt", true, false, true});
                    return t;
                }();
                return table[static_cast<size_t>(op)];
            }

            bool DefinesHiLo(MOp op) {
                return op >= MOp::Mult && op <= MOp::Ddivu;
            }

            bool DefinesFcc(MOp op) {
                return op >= MOp::CeqS && op <= MOp::CleD;
            }

            bool ReadsFcc(MOp op) {
                return op == MOp::Bc1t || op == MOp::Bc1f || op == MOp::Movf || op == MOp::Movt;
            }
        }

        std::string reg::Name(Reg r) {
            static const char* const general[] = {
                "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$a4", "$a5", "$a6",
                "$a7", "$t0", "$t1", "$t2", "$t3", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5",
                "$s6", "$s7", "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"};
            if (IsGeneral(r)) {
                return general[r];
            }
            if (IsFloat(r)) {
                return "$f" + std::to_string(r - FirstFloat);
            }
            return r == Fcc ? "$fcc0" : "<reg>";
        }

        const OpInfo& Info(MOp op) {
            return Lookup(op);
        }

        std::vector<Reg> Defs(const MInst& inst) {
            const auto& info = Info(inst.op);
            std::vector<Reg> defs;
            if (info.definesFirst && !inst.operands.empty() && inst.operands[0].kind == MOperand::Kind::Register) {
                defs.push_back(inst.operands[0].reg);
            }
            if (info.definesSecond && inst.operands.size() > 1 &&
                inst.operands[1].kind == MOperand::Kind::Register) {
                defs.push_back(inst.operands[1].reg);
            }
            if (DefinesHiLo(inst.op)) {
                defs.push_back(reg::HiLo);
            }
            if (DefinesFcc(inst.op)) {
                defs.push_back(reg::Fcc);
            }
            defs.insert(defs.end(), inst.implicitDefs.begin(), inst.implicitDefs.end());
            std::erase(defs, reg::Zero);
            return defs;
        }

        std::vector<Reg> Uses(const MInst& inst) {
            if (inst.op == MOp::Label || inst.op == MOp::Comment) {
                return {};
            }
            const auto& info = Info(inst.op);
            std::vector<Reg> uses;
            for (size_t i = 0; i < inst.operands.size(); ++i) {
                const auto& operand = inst.operands[i];
                const bool written = (i == 0 && info.definesFirst) || (i == 1 && info.definesSecond);
                if (operand.kind == MOperand::Kind::Memory ||
                    (operand.kind == MOperand::Kind::Register && (!written || (i == 0 && info.readsFirst)))) {
                    uses.push_back(operand.reg);
                }
            }
            if (inst.op == MOp::Mflo || inst.op == MOp::Mfhi) {
                uses.push_back(reg::HiLo);
            }
            if (ReadsFcc(inst.op)) {
                uses.push_back(reg::Fcc);
            }
            uses.insert(uses.end(), inst.implicitUses.begin(), inst.implicitUses.end());
            std::erase(uses, reg::Zero);
            return uses;
        }

        const std::string* BranchTarget(const MInst& inst) {
            if (!Info(inst.op).branch) {
                return nullptr;
            }
            for (const auto& operand : inst.operands) {
                if (operand.kind == MOperand::Kind::Label) {
                    return &operand.name;
                }
            }
            return nullptr;
        }
    }
}
