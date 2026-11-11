#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace baka {
    namespace codegen {
        // Machine IR for MIPS64r2, n64 ABI, little-endian. One MInst is exactly
        // one printed assembly line. Instruction selection creates it, the peephole
        // optimizer rewrites it in place, and the printer writes it out. Assembly is
        // emitted under `.set noreorder`: every branch, jump and call is followed by
        // an explicit delay-slot instruction (initially `nop`).

        // Registers are dense numbers: 0-31 are GPRs, 32-63 are FPRs, and two
        // pseudo-registers model implicit HI/LO and floating condition code 0, so
        // def/use analysis treats them like any other register.
        using Reg = uint8_t;

        namespace reg {
            inline constexpr Reg Zero = 0, At = 1, V0 = 2, V1 = 3, A0 = 4, A1 = 5, A2 = 6, A3 = 7, A4 = 8, A5 = 9,
                                 A6 = 10, A7 = 11, T0 = 12, T1 = 13, T2 = 14, T3 = 15, S0 = 16, S1 = 17, S2 = 18,
                                 S3 = 19, S4 = 20, S5 = 21, S6 = 22, S7 = 23, T8 = 24, T9 = 25, K0 = 26, K1 = 27,
                                 Gp = 28, Sp = 29, Fp = 30, Ra = 31;
            inline constexpr Reg FirstFloat = 32;
            inline constexpr Reg HiLo = 64, Fcc = 65, None = 255;
            inline constexpr Reg Count = 66;

            constexpr Reg F(unsigned n) {
                return static_cast<Reg>(FirstFloat + n);
            }

            constexpr bool IsFloat(Reg r) {
                return r >= FirstFloat && r < HiLo;
            }

            constexpr bool IsGeneral(Reg r) {
                return r < FirstFloat;
            }

            // n64: $s0-$s7, $fp, $gp and $f24-$f31 are preserved across calls.
            constexpr bool IsCalleeSaved(Reg r) {
                return (r >= S0 && r <= S7) || r == Fp || r == Gp || (r >= F(24) && r <= F(31));
            }

            std::string Name(Reg r);
        }

        // Every mnemonic used by the backend. Operand order matches assembly order.
        enum class MOp : uint8_t {
            // Pseudo lines.
            Label,   // operands: label
            Comment, // operands: none; text in MInst::comment

            // Integer register-register: rd, rs, rt.
            Addu, Daddu, Subu, Dsubu, And, Or, Xor, Nor, Slt, Sltu,
            Sllv, Srlv, Srav, Dsllv, Dsrlv, Dsrav, Mul, Movn, Movz,

            // Integer register-immediate: rt, rs, imm.
            Addiu, Daddiu, Andi, Ori, Xori, Slti, Sltiu,
            Sll, Srl, Sra, Dsll, Dsrl, Dsra, Dsll32, Dsrl32, Dsra32,

            // rd, rs (two-register forms).
            Move, Seb, Seh,
            // rt, rs, pos, size.
            Dext,
            // rt, imm (lui); rd, imm (li: any 64-bit constant, printed as `dli`).
            Lui, Li,
            // rd, symbol(+imm): load a 64-bit symbol address (printed as `dla`).
            La,

            // HI/LO multiply and divide: rs, rt (defines HiLo). Division prints the
            // explicit `$zero` destination form so the assembler adds no checks.
            Mult, Multu, Dmult, Dmultu, Div, Divu, Ddiv, Ddivu,
            // rd (reads HiLo).
            Mflo, Mfhi,

            // Loads: rt, imm(base). Stores: rt, imm(base).
            Lb, Lbu, Lh, Lhu, Lw, Lwu, Ld, Lwc1, Ldc1,
            Sb, Sh, Sw, Sd, Swc1, Sdc1,

            // Control transfer; each is followed by one delay-slot instruction.
            Beq, Bne,             // rs, rt, label
            Blez, Bgtz, Bltz, Bgez, // rs, label
            B,                    // label
            Bc1t, Bc1f,           // label (reads Fcc)
            Jr,                   // rs (function return through $ra)
            Jalr,                 // rs (call; the callee address is in $t9)
            Nop,

            // Floating point. fd, fs, ft / fd, fs.
            AddS, AddD, SubS, SubD, MulS, MulD, DivS, DivD, NegS, NegD, MovS, MovD,
            CvtDS, CvtSD, CvtDL, CvtSL, CvtDW, CvtSW, TruncLD, TruncLS, TruncWD, TruncWS,
            // fs, ft (defines Fcc).
            CeqS, CeqD, CltS, CltD, CleS, CleD,
            // GPR <-> FPR moves: rt, fs. Mtc1/Dmtc1 define the FPR operand.
            Mfc1, Dmfc1, Mtc1, Dmtc1,
            // rd, rs (conditional on Fcc; reads rd too).
            Movf, Movt,
        };

        struct MOperand {
            enum class Kind : uint8_t { None, Register, Immediate, Memory, Label, Symbol };

            Kind kind = Kind::None;
            Reg reg = reg::None; // Register, or Memory base
            int64_t imm = 0;     // Immediate, Memory offset, or Symbol addend
            std::string name;    // Label or Symbol

            static MOperand R(Reg r) {
                return {Kind::Register, r, 0, {}};
            }

            static MOperand I(int64_t value) {
                return {Kind::Immediate, reg::None, value, {}};
            }

            static MOperand M(Reg base, int64_t offset) {
                return {Kind::Memory, base, offset, {}};
            }

            static MOperand L(std::string label) {
                return {Kind::Label, reg::None, 0, std::move(label)};
            }

            static MOperand S(std::string symbol, int64_t addend = 0) {
                return {Kind::Symbol, reg::None, addend, std::move(symbol)};
            }

            bool operator==(const MOperand&) const = default;
        };

        struct MInst {
            MOp op = MOp::Nop;
            std::vector<MOperand> operands;
            // Calls and returns name the registers they implicitly read/write so
            // liveness stays exact: argument registers for jalr, $v0/$f0 for jr,
            // and every caller-saved register as clobbered by jalr. On a Label,
            // non-empty implicitUses is the exact set of registers live there; an
            // empty set means unknown (everything live).
            std::vector<Reg> implicitUses, implicitDefs;
            std::string comment;

            bool operator==(const MInst&) const = default;
        };

        // Static per-opcode facts used by every pass that analyses MInsts.
        struct OpInfo {
            const char* mnemonic;
            bool definesFirst = false;  // operand 0 is written
            bool definesSecond = false; // operand 1 is written (mtc1/dmtc1)
            bool readsFirst = false;    // conditional moves read their destination
            bool load = false, store = false;
            bool branch = false;  // conditional or unconditional local branch
            bool call = false, ret = false;
            bool hasDelaySlot = false;
        };

        const OpInfo& Info(MOp op);
        // Registers read / written, including memory bases and implicit registers.
        std::vector<Reg> Uses(const MInst& inst);
        std::vector<Reg> Defs(const MInst& inst);
        // Labels referenced by a branch (empty otherwise).
        const std::string* BranchTarget(const MInst& inst);

        struct MFunction {
            std::string symbol;
            bool global = true;
            std::vector<MInst> code; // complete body, including prologue/epilogue
        };

        // Floating literals cannot be immediates. Selection interns each distinct
        // bit pattern once in read-only data and loads it by label.
        struct LiteralPool {
            std::map<std::pair<unsigned, uint64_t>, std::string> labels; // (bits, pattern) -> label

            std::string intern(unsigned bits, uint64_t pattern) {
                auto [it, inserted] = labels.try_emplace({bits, pattern});
                if (inserted) {
                    it->second = ".Lfp." + std::to_string(labels.size() - 1);
                }
                return it->second;
            }
        };
    }
}
