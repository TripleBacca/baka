// Standalone unit tests for src/codegen/peephole.cc. Programs are written as
// assembly text, parsed into MFunctions, optimized and compared as text.
// Build: g++ -std=c++23 -I src custom_tests/codegen/peephole_tests.cc src/codegen/peephole.cc src/codegen/mips.cc
#include "codegen/internal.hh"
#include <iostream>
#include <sstream>

using namespace baka::codegen;

namespace {
    std::string Trim(std::string s) {
        const auto b = s.find_first_not_of(" \t");
        if (b == std::string::npos) {
            return "";
        }
        return s.substr(b, s.find_last_not_of(" \t") - b + 1);
    }

    Reg ParseReg(const std::string& name) {
        for (unsigned r = 0; r < reg::HiLo; ++r) {
            if (reg::Name(static_cast<Reg>(r)) == name) {
                return static_cast<Reg>(r);
            }
        }
        std::cerr << "bad register " << name << "\n";
        std::exit(2);
    }

    MOp ParseOp(const std::string& mnemonic) {
        for (unsigned o = static_cast<unsigned>(MOp::Addu); o <= static_cast<unsigned>(MOp::Movt); ++o) {
            if (mnemonic == Info(static_cast<MOp>(o)).mnemonic) {
                return static_cast<MOp>(o);
            }
        }
        std::cerr << "bad mnemonic " << mnemonic << "\n";
        std::exit(2);
    }

    // Test conventions: jalr reads $t9 and $a0/$a1 and clobbers every caller-saved
    // register; jr reads $ra and $v0.
    MInst Parse(const std::string& line) {
        MInst in;
        // `Name:` is a label; `Name:[$r1,$r2]` also hints the registers live there.
        if (const auto colon = line.find(':'); colon != std::string::npos) {
            in.op = MOp::Label;
            in.operands = {MOperand::L(line.substr(0, colon))};
            if (colon + 1 < line.size()) {
                std::stringstream hint(line.substr(colon + 2, line.size() - colon - 3));
                std::string r;
                while (std::getline(hint, r, ',')) {
                    in.implicitUses.push_back(ParseReg(Trim(r)));
                }
            }
            return in;
        }
        const auto sp = line.find(' ');
        in.op = ParseOp(line.substr(0, sp));
        if (sp != std::string::npos) {
            std::stringstream rest(line.substr(sp + 1));
            std::string tok;
            while (std::getline(rest, tok, ',')) {
                tok = Trim(tok);
                if (tok[0] == '$') {
                    in.operands.push_back(MOperand::R(ParseReg(tok)));
                } else if (tok.find('(') != std::string::npos) {
                    const auto l = tok.find('(');
                    in.operands.push_back(MOperand::M(ParseReg(tok.substr(l + 1, tok.size() - l - 2)), std::stoll(tok.substr(0, l))));
                } else if (std::isdigit(static_cast<unsigned char>(tok[0])) || tok[0] == '-') {
                    in.operands.push_back(MOperand::I(std::stoll(tok)));
                } else if (Info(in.op).branch) {
                    in.operands.push_back(MOperand::L(tok));
                } else {
                    in.operands.push_back(MOperand::S(tok));
                }
            }
        }
        if (in.op == MOp::Jr) {
            in.implicitUses = {reg::V0};
        } else if (in.op == MOp::Jalr) {
            in.implicitUses = {reg::A0, reg::A1};
            for (Reg r = 0; r < reg::HiLo; ++r) {
                if (r != reg::Zero && r != reg::Sp && !reg::IsCalleeSaved(r) && !(r >= reg::F(24))) {
                    in.implicitDefs.push_back(r);
                }
            }
        }
        return in;
    }

    std::string Print(const MInst& in) {
        if (in.op == MOp::Label) {
            return in.operands[0].name + ":";
        }
        std::string s = Info(in.op).mnemonic;
        for (size_t i = 0; i < in.operands.size(); ++i) {
            const auto& o = in.operands[i];
            s += i ? ", " : " ";
            switch (o.kind) {
            case MOperand::Kind::Register: s += reg::Name(o.reg); break;
            case MOperand::Kind::Immediate: s += std::to_string(o.imm); break;
            case MOperand::Kind::Memory: s += std::to_string(o.imm) + "(" + reg::Name(o.reg) + ")"; break;
            default: s += o.name; break;
            }
        }
        return s;
    }

    std::vector<std::string> Lines(const std::string& text) {
        std::vector<std::string> out;
        std::stringstream ss(text);
        std::string line;
        while (std::getline(ss, line)) {
            if (!(line = Trim(line)).empty()) {
                out.push_back(line);
            }
        }
        return out;
    }

    // Every delay-slot instruction is followed by exactly one non-label, non-delay-slot instruction.
    bool SlotsValid(const std::vector<MInst>& code) {
        for (size_t i = 0; i < code.size(); ++i) {
            if (Info(code[i].op).hasDelaySlot &&
                (i + 1 >= code.size() || code[i + 1].op == MOp::Label || Info(code[i + 1].op).hasDelaySlot)) {
                return false;
            }
        }
        return true;
    }

    int failures = 0;

    // `tail` keeps $v0 live and cannot be filled (dli), so rules are seen unaffected by slot filling.
    const std::string Tail = "dli $v0, 7\njr $ra\nnop\n";

    void Check(const std::string& name, const std::string& input, const std::string& expected, bool tail = true) {
        MFunction f;
        f.symbol = "t";
        for (const auto& l : Lines(input + "\n" + (tail ? Tail : ""))) {
            f.code.push_back(Parse(l));
        }
        Peephole(f);
        std::vector<std::string> got;
        for (const auto& in : f.code) {
            got.push_back(Print(in));
        }
        const auto want = Lines(expected + "\n" + (tail ? Tail : ""));
        const bool ok = got == want && SlotsValid(f.code);
        std::cout << (ok ? "PASS " : "FAIL ") << name << "\n";
        if (!ok) {
            ++failures;
            std::cout << "  got:\n";
            for (const auto& l : got) {
                std::cout << "    " << l << "\n";
            }
            std::cout << "  want:\n";
            for (const auto& l : want) {
                std::cout << "    " << l << "\n";
            }
        }
    }

    void Same(const std::string& name, const std::string& input, bool tail = true) {
        Check(name, input, input, tail);
    }
}

int main() {
    // Rule 1: redundant loads.
    Check("fwd sd->ld", "sd $t0, 8($sp)\nld $t1, 8($sp)\nsd $t1, 0($a0)", "sd $t0, 8($sp)\nsd $t0, 0($a0)");
    Check("fwd ld->ld same reg deleted", "ld $t0, 8($sp)\nld $t0, 8($sp)\nsd $t0, 0($a0)", "ld $t0, 8($sp)\nsd $t0, 0($a0)");
    Check("fwd ld->ld other reg", "ld $t0, 8($sp)\nld $t1, 8($sp)\ndaddu $t2, $t0, $t1\nsd $t2, 0($a0)",
          "ld $t0, 8($sp)\ndaddu $t2, $t0, $t0\nsd $t2, 0($a0)");
    Check("fwd keeps move when source redefined", "sd $t0, 8($sp)\nld $t1, 8($sp)\ndaddiu $t0, $t0, 1\ndaddu $t2, $t1, $t0\nsd $t2, 0($a0)",
          "sd $t0, 8($sp)\nmove $t1, $t0\ndaddiu $t0, $t0, 1\ndaddu $t2, $t1, $t0\nsd $t2, 0($a0)");
    Check("fwd lw->lw", "lw $t0, 8($sp)\nlw $t1, 8($sp)\ndaddu $t2, $t0, $t1\nsd $t2, 0($a0)",
          "lw $t0, 8($sp)\ndaddu $t2, $t0, $t0\nsd $t2, 0($a0)");
    Check("fwd sdc1->ldc1 uses mov.d", "sdc1 $f4, 8($sp)\nldc1 $f2, 8($sp)\nadd.d $f4, $f4, $f4\nadd.d $f6, $f2, $f4\nsdc1 $f6, 0($a0)",
          "sdc1 $f4, 8($sp)\nmov.d $f2, $f4\nadd.d $f4, $f4, $f4\nadd.d $f6, $f2, $f4\nsdc1 $f6, 0($a0)");
    Check("fwd swc1->lwc1 uses mov.s", "swc1 $f4, 8($sp)\nlwc1 $f2, 8($sp)\nadd.s $f4, $f4, $f4\nadd.s $f6, $f2, $f4\nswc1 $f6, 0($a0)",
          "swc1 $f4, 8($sp)\nmov.s $f2, $f4\nadd.s $f4, $f4, $f4\nadd.s $f6, $f2, $f4\nswc1 $f6, 0($a0)");
    Same("no fwd sw->lw", "sw $t0, 8($sp)\nlw $t1, 8($sp)\nsd $t1, 0($a0)");
    Same("no fwd lw->ld", "lw $t0, 8($sp)\nld $t1, 8($sp)\nsd $t1, 0($a0)\nsd $t0, 8($a0)");
    Same("no fwd gpr->fpr", "sd $t0, 8($sp)\nldc1 $f2, 8($sp)\nsdc1 $f2, 0($a0)");
    Same("no fwd across jalr", "sd $t0, 8($sp)\ndla $t9, f\njalr $t9\nnop\nld $t1, 8($sp)\nsd $t1, 0($a0)");
    Same("no fwd across non-sp store", "sd $t0, 8($sp)\nsd $t2, 0($a0)\nld $t1, 8($sp)\nsd $t1, 8($a0)");
    Same("no fwd when holder redefined", "sd $t0, 8($sp)\ndaddiu $t0, $t0, 1\nld $t1, 8($sp)\nsd $t1, 0($a0)\nsd $t0, 16($a0)");
    Same("no fwd after partial overwrite", "sd $t0, 8($sp)\nsw $t2, 12($sp)\nld $t1, 8($sp)\nsd $t1, 0($a0)");
    Same("no fwd across label", "sd $t0, 8($sp)\nLx:\nld $t1, 8($sp)\nsd $t1, 0($a0)");
    Same("no fwd after sp adjust", "sd $t0, 8($sp)\ndaddiu $sp, $sp, -16\nld $t1, 8($sp)\nsd $t1, 0($a0)");

    // Rule 2: dead stores.
    Check("dead store overwritten", "sd $t0, 8($sp)\nsd $t1, 8($sp)", "sd $t1, 8($sp)");
    Check("dead sw covered by sd", "sw $t0, 8($sp)\nsd $t1, 8($sp)", "sd $t1, 8($sp)");
    Same("store kept at block end", "sd $t0, 8($sp)");
    Same("store kept: partial overwrite", "sd $t0, 8($sp)\nsw $t1, 8($sp)");
    Same("store kept: slot loaded", "sw $t0, 8($sp)\nlw $t2, 8($sp)\nsd $t1, 8($sp)\nsd $t2, 0($a0)");
    Same("store kept: non-sp load", "sd $t0, 8($sp)\nld $t2, 0($a1)\nsd $t1, 8($sp)\nsd $t2, 0($a0)");
    Same("store kept: jalr", "sd $t0, 8($sp)\ndla $t9, f\njalr $t9\nnop\nsd $t1, 8($sp)");

    // Rules 3 and 4: dead code and no-ops.
    Check("dead li", "dli $t0, 5\ndli $t0, 6\nsd $t0, 0($a0)", "dli $t0, 6\nsd $t0, 0($a0)");
    Check("dead mult overwritten", "dmult $t0, $t1\ndmult $a2, $a3\nmflo $t2\nsd $t2, 0($a0)", "dmult $a2, $a3\nmflo $t2\nsd $t2, 0($a0)");
    Same("mult kept for mflo", "dmult $t0, $t1\nmflo $t2\nsd $t2, 0($a0)");
    Same("value live at label kept", "dli $t0, 5\nLx:\nsd $t0, 0($a0)");
    Same("callee-saved kept at return", "dli $s0, 5");
    Check("move r,r", "move $t0, $t0\nsd $t0, 0($a0)", "sd $t0, 0($a0)");
    Check("or r,r,zero", "or $t1, $t1, $zero\nsd $t1, 0($a0)", "sd $t1, 0($a0)");
    Check("daddiu r,r,0", "daddiu $t2, $t2, 0\nsd $t2, 0($a0)", "sd $t2, 0($a0)");
    Check("mov.d f,f", "mov.d $f2, $f2\nsdc1 $f2, 0($a0)", "sdc1 $f2, 0($a0)");
    Same("addiu r,r,0 kept (sign-extends)", "addiu $t2, $t2, 0\nsd $t2, 0($a0)");

    // Rule 5: copy propagation.
    Check("copy prop", "move $t0, $a1\ndaddu $t2, $t0, $t0\nsd $t2, 0($a0)\nsd $t0, 8($a0)",
          "daddu $t2, $a1, $a1\nsd $t2, 0($a0)\nsd $a1, 8($a0)");
    Same("copy prop stops at redefinition", "move $t0, $a1\ndaddiu $a1, $a1, 1\nsd $t0, 0($a0)\nsd $a1, 8($a0)");

    // Rule 6: address folding.
    Check("fold address", "daddiu $t0, $sp, 16\nld $t1, 8($t0)\nsd $t1, 0($a0)", "ld $t1, 24($sp)\nsd $t1, 0($a0)");
    Check("fold address store", "daddiu $t0, $a1, 16\nsd $a2, 8($t0)", "sd $a2, 24($a1)");
    Same("no fold: temp live", "daddiu $t0, $sp, 16\nld $t1, 8($t0)\nsd $t1, 0($a0)\nsd $t0, 8($a0)");
    Same("no fold: base redefined", "daddiu $t0, $a1, 16\ndaddiu $a1, $a1, 8\nld $t1, 0($t0)\nsd $t1, 0($a0)\nsd $a1, 8($a0)");
    Same("no fold: offset overflow", "daddiu $t0, $a1, 32767\nld $t1, 8($t0)\nsd $t1, 0($a0)");
    Same("no fold: stored value is the address", "daddiu $t0, $a1, 16\nsd $t0, 8($t0)");

    // Rule 7: immediate forms.
    Check("daddu imm", "dli $t0, 100\ndaddu $t1, $a1, $t0\nsd $t1, 0($a0)", "daddiu $t1, $a1, 100\nsd $t1, 0($a0)");
    Check("addu imm, t first", "dli $t0, -5\naddu $t1, $t0, $a1\nsd $t1, 0($a0)", "addiu $t1, $a1, -5\nsd $t1, 0($a0)");
    Check("dsubu imm", "dli $t0, 100\ndsubu $t1, $a1, $t0\nsd $t1, 0($a0)", "daddiu $t1, $a1, -100\nsd $t1, 0($a0)");
    Check("subu imm", "dli $t0, 100\nsubu $t1, $a1, $t0\nsd $t1, 0($a0)", "addiu $t1, $a1, -100\nsd $t1, 0($a0)");
    Check("and imm", "dli $t0, 255\nand $t1, $a1, $t0\nsd $t1, 0($a0)", "andi $t1, $a1, 255\nsd $t1, 0($a0)");
    Check("xor imm, t first", "dli $t0, 5\nxor $t1, $t0, $a1\nsd $t1, 0($a0)", "xori $t1, $a1, 5\nsd $t1, 0($a0)");
    Check("slt imm", "dli $t0, -1\nslt $t1, $a1, $t0\nsd $t1, 0($a0)", "slti $t1, $a1, -1\nsd $t1, 0($a0)");
    Check("sltu imm", "dli $t0, 9\nsltu $t1, $a1, $t0\nsd $t1, 0($a0)", "sltiu $t1, $a1, 9\nsd $t1, 0($a0)");
    Same("no imm: slt with t first", "dli $t0, 5\nslt $t1, $t0, $a1\nsd $t1, 0($a0)");
    Same("no imm: too large", "dli $t0, 40000\ndaddu $t1, $a1, $t0\nsd $t1, 0($a0)");
    Same("no imm: andi negative", "dli $t0, -1\nand $t1, $a1, $t0\nsd $t1, 0($a0)");
    Same("no imm: t still live", "dli $t0, 100\ndaddu $t1, $a1, $t0\nsd $t1, 0($a0)\nsd $t0, 8($a0)");
    Same("no imm: dsubu -32768", "dli $t0, -32768\ndsubu $t1, $a1, $t0\nsd $t1, 0($a0)");

    // Rule 8: strength reduction.
    Check("dmult pow2", "dli $t0, 8\ndmult $a1, $t0\nmflo $t1\nsd $t1, 0($a0)", "dsll $t1, $a1, 3\nsd $t1, 0($a0)");
    Check("dmult 2^32", "dli $t0, 4294967296\ndmult $t0, $a1\nmflo $t1\nsd $t1, 0($a0)", "dsll32 $t1, $a1, 0\nsd $t1, 0($a0)");
    Check("mult pow2", "dli $t0, 16\nmult $a1, $t0\nmflo $t1\nsd $t1, 0($a0)", "sll $t1, $a1, 4\nsd $t1, 0($a0)");
    Check("mul pow2", "dli $t0, 4\nmul $t1, $a1, $t0\nsd $t1, 0($a0)", "sll $t1, $a1, 2\nsd $t1, 0($a0)");
    Same("no strength: not pow2", "dli $t0, 6\ndmult $a1, $t0\nmflo $t1\nsd $t1, 0($a0)");
    Same("no strength: HiLo read again", "dli $t0, 8\ndmult $a1, $t0\nmflo $t1\nmfhi $t3\nsd $t1, 0($a0)\nsd $t3, 8($a0)");
    Same("no strength: constant live", "dli $t0, 8\ndmult $a1, $t0\nmflo $t1\nsd $t1, 0($a0)\nsd $t0, 8($a0)");
    Same("no strength: mul too wide for sll", "dli $t0, 4294967296\nmul $t1, $a1, $t0\nsd $t1, 0($a0)");

    // Rule 9: control flow.
    Check("jump to next label", "sd $t0, 0($a0)\nb Lx\nnop\nLx:", "sd $t0, 0($a0)\nLx:");
    Check("jump to next label through comments", "sd $t0, 0($a0)\nb Lx\nnop\nLy:\nLx:", "sd $t0, 0($a0)\nLy:\nLx:");
    Check("branch to next label", "sd $t0, 0($a0)\nbne $a1, $a2, Lx\nnop\nLx:", "sd $t0, 0($a0)\nLx:");
    Check("branch over jump", "dli $t0, 1\nbeq $a1, $a2, L1\nnop\nb L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:",
          "dli $t0, 1\nbne $a1, $a2, L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:");
    Check("branch over jump bltz", "dli $t0, 1\nbltz $a1, L1\nnop\nb L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:",
          "dli $t0, 1\nbgez $a1, L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:");
    Check("branch over jump bc1t", "dli $t0, 1\nbc1t L1\nnop\nb L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:",
          "dli $t0, 1\nbc1f L2\nnop\nL1:\nsd $t0, 0($a0)\nL2:");
    Same("no inversion: other target", "dli $t0, 1\nbeq $a1, $a2, L1\nnop\nb L2\nnop\nL3:\nsd $t0, 0($a0)\nL1:\nL2:");
    Check("unreachable after b", "dli $t0, 1\nb L3\nnop\nsd $t1, 0($a0)\ndli $t2, 2\nL2:\nsd $t0, 8($a0)\nL3:",
          "dli $t0, 1\nb L3\nnop\nL2:\nsd $t0, 8($a0)\nL3:");
    Check("unreferenced .L label removed", "sd $t0, 0($a0)\n.L5:\nsd $t1, 8($a0)", "sd $t0, 0($a0)\nsd $t1, 8($a0)");
    Check("label removal merges blocks", "sd $t0, 8($sp)\n.L5:\nsd $t1, 8($sp)", "sd $t1, 8($sp)");
    Same("referenced .L label kept", "dli $t0, 1\nbeq $a1, $a2, .L6\nnop\nsd $t0, 0($a0)\n.L6:");

    // Rule 10: delay-slot filling (no Tail so the slot is the end of the function).
    Check("fill conditional branch", "sd $t0, 0($a0)\nbne $a1, $a2, Lx\nnop\nsd $t1, 8($a0)\nLx:",
          "bne $a1, $a2, Lx\nsd $t0, 0($a0)\nsd $t1, 8($a0)\nLx:", false);
    Same("no fill: defines branch operand", "daddiu $a1, $a1, 1\nbne $a1, $a2, Lx\nnop\nsd $a1, 8($a0)\nLx:", false);
    Same("no fill: dli", "dli $t0, 1\nbne $a1, $a2, Lx\nnop\nsd $t0, 8($a0)\nLx:", false);
    Same("no fill: dla", "dla $t0, sym\nbne $a1, $a2, Lx\nnop\nsd $t0, 8($a0)\nLx:", false);
    Same("no fill: mflo", "mflo $t0\nbne $a1, $a2, Lx\nnop\nsd $t0, 8($a0)\nLx:", false);
    Same("no fill: instruction follows a label", "Ly:\nsd $t0, 0($a0)\nbne $a1, $a2, Lx\nnop\nsd $t1, 8($a0)\nLx:", false);
    Same("no fill: fcc defined for bc1t", "c.lt.d $f2, $f4\nbc1t Lx\nnop\nsdc1 $f2, 8($a0)\nLx:", false);
    Check("fill call with argument setup", "dla $t9, f\nmove $a0, $t1\njalr $t9\nnop\nsd $v0, 0($a0)",
          "dla $t9, f\njalr $t9\nmove $a0, $t1\nsd $v0, 0($a0)", false);
    Same("no fill: jalr target defined", "move $t9, $a1\njalr $t9\nnop\nsd $v0, 0($a0)", false);
    Check("fill jr with stack restore", "ld $ra, 8($sp)\ndaddiu $sp, $sp, 16\njr $ra\nnop",
          "ld $ra, 8($sp)\njr $ra\ndaddiu $sp, $sp, 16", false);
    Same("no fill: jr with $ra reload", "ld $ra, 8($sp)\njr $ra\nnop", false);
    Check("fill only closest, once", "sd $t0, 0($a0)\nsd $t1, 8($a0)\nb Lz\nnop\nLy:\nsd $t2, 0($a0)\nLz:",
          "sd $t0, 0($a0)\nb Lz\nsd $t1, 8($a0)\nLy:\nsd $t2, 0($a0)\nLz:", false);

    // Label liveness hints.
    Check("hint: dead before hinted label", "dli $t0, 5\nLx:[$a0]", "Lx:");
    Check("hint: live at hinted label", "dli $t0, 5\nLx:[$t0]", "dli $t0, 5\nLx:");
    Check("hint: branch target not needing value", "dli $t0, 5\nbne $a1, $a2, Lx\nnop\ndli $t0, 6\nsd $t0, 0($a0)\nLx:[$a0]",
          "bne $a1, $a2, Lx\nnop\ndli $t0, 6\nsd $t0, 0($a0)\nLx:");
    Check("hint: branch target needing value", "dli $t0, 5\nbne $a1, $a2, Lx\nnop\ndli $t0, 6\nsd $t0, 0($a0)\nLx:[$t0]",
          "dli $t0, 5\nbne $a1, $a2, Lx\nnop\ndli $t0, 6\nsd $t0, 0($a0)\nLx:");
    Check("hint: unconditional jump uses target set", "dli $t0, 5\nb Lx\nnop\nLy:\nsd $a1, 0($a0)\nLx:[$a0]",
          "b Lx\nnop\nLy:\nsd $a1, 0($a0)\nLx:");
    // Backward copy retargeting.
    Check("retarget load into return reg", "ld $t0, 12($sp)\nmove $v0, $t0\njr $ra\nnop", "jr $ra\nld $v0, 12($sp)", false);
    Check("retarget mflo", "mflo $t0\nmove $v0, $t0\njr $ra\nnop", "mflo $v0\njr $ra\nnop", false);
    Check("retarget fpr", "add.d $f2, $f4, $f6\nmov.d $f8, $f2\nsdc1 $f8, 0($a0)", "add.d $f2, $f4, $f6\nsdc1 $f2, 0($a0)");
    Check("no retarget: source live", "ld $t0, 12($sp)\nmove $v0, $t0\nLx:[$t0,$v0]\njr $ra\nnop",
          "ld $t0, 12($sp)\nmove $v0, $t0\nLx:\njr $ra\nnop", false);
    Check("no retarget: not adjacent", "ld $t0, 12($sp)\nsd $t1, 0($a0)\nmove $v0, $t0\njr $ra\nnop",
          "ld $t0, 12($sp)\nsd $t1, 0($a0)\njr $ra\nmove $v0, $t0", false);

    std::cout << (failures ? "FAILED: " + std::to_string(failures) + " case(s)\n" : "all passed\n");
    return failures ? 1 : 0;
}
