#include "constant_folding.hh"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace baka::ir {
    namespace {
        uint64_t Mask(const Type& type) {
            return type.bits == 64 ? ~uint64_t{0} : (uint64_t{1} << type.bits) - 1;
        }

        int64_t Signed(uint64_t value, const Type& type) {
            value &= Mask(type);
            if (type.bits < 64 && (value & (uint64_t{1} << (type.bits - 1)))) {
                value |= ~Mask(type);
            }
            return std::bit_cast<int64_t>(value);
        }

        std::optional<Operand> Fold(const Module& module, const Instruction& in) {
            if (!DefinesTemporary(in)) {
                return {};
            }
            const auto& result = module.type(in.result.type);
            const auto& source = module.type(in.left.type);
            auto a = in.left, b = in.right;
            auto integer = [&](uint64_t value) -> std::optional<Operand> {
                if (result.kind != TypeKind::Integer) {
                    return {};
                }
                return Operand{OperandKind::Integer, in.result.type, 0, value & Mask(result)};
            };
            if (in.opcode == Opcode::Cast && a.kind == OperandKind::Integer &&
                (source.kind == TypeKind::Integer || source.kind == TypeKind::Pointer)) {
                auto value = source.kind == TypeKind::Integer && source.isSigned
                                 ? static_cast<uint64_t>(Signed(a.integer, source))
                                 : a.integer;
                if (result.kind == TypeKind::Pointer) {
                    return Operand{OperandKind::Integer, in.result.type, 0, value};
                }
                if (source.kind == TypeKind::Pointer && result.kind == TypeKind::Integer) {
                    return integer(result.bits == 1 ? value != 0 : value);
                }
            }
            if (in.opcode == Opcode::Cast) {
                if (a.kind == OperandKind::Integer && source.kind == TypeKind::Integer &&
                    result.kind == TypeKind::Integer) {
                    auto value = source.isSigned ? static_cast<uint64_t>(Signed(a.integer, source)) : a.integer;
                    // bool is represented as u1; conversion is truth testing, not truncation.
                    return integer(result.bits == 1 ? value != 0 : value);
                }
                if (a.kind == OperandKind::Floating && result.kind == TypeKind::Integer && result.bits == 1) {
                    return integer(a.floating != 0);
                }
                if (result.kind == TypeKind::Floating &&
                    (a.kind == OperandKind::Floating ||
                     (a.kind == OperandKind::Integer && source.kind == TypeKind::Integer))) {
                    long double value = a.kind == OperandKind::Floating ? a.floating
                                        : source.isSigned ? static_cast<long double>(Signed(a.integer, source))
                                                          : static_cast<long double>(a.integer);
                    value = result.bits == 32 ? static_cast<long double>(static_cast<float>(value))
                                              : static_cast<long double>(static_cast<double>(value));
                    if (std::isfinite(value)) {
                        return Operand{OperandKind::Floating, in.result.type, 0, 0, value};
                    }
                }
                // Out-of-range float->integer and pointer relocations remain runtime casts.
                if (a.kind == OperandKind::Floating && result.kind == TypeKind::Integer && std::isfinite(a.floating)) {
                    auto value = std::trunc(a.floating);
                    auto minimum = result.isSigned ? -std::ldexp(1.L, result.bits - 1) : 0.L;
                    auto maximum = std::ldexp(1.L, result.isSigned ? result.bits - 1 : result.bits);
                    if (value >= minimum && value < maximum) {
                        return integer(result.isSigned ? static_cast<uint64_t>(static_cast<int64_t>(value))
                                                       : static_cast<uint64_t>(value));
                    }
                }
                return {};
            }
            if (a.kind == OperandKind::Floating) {
                auto x = a.floating, y = b.floating;
                if (in.opcode == Opcode::LogicalNot) {
                    return integer(x == 0);
                }
                long double number;
                if (in.opcode == Opcode::Negate) {
                    number = -x;
                } else {
                    if (b.kind != OperandKind::Floating) {
                        return {};
                    }
                    switch (in.opcode) {
                    case Opcode::Equal:
                        return integer(x == y);
                    case Opcode::NotEqual:
                        return integer(x != y);
                    case Opcode::Less:
                        return integer(x < y);
                    case Opcode::Greater:
                        return integer(x > y);
                    case Opcode::LessEqual:
                        return integer(x <= y);
                    case Opcode::GreaterEqual:
                        return integer(x >= y);
                    default:
                        // Execute constant FP arithmetic at the declared width,
                        // rather than long-double arithmetic followed by rounding.
                        auto compute = [&](auto left, auto right) -> long double {
                            switch (in.opcode) {
                            case Opcode::Add:
                                return left + right;
                            case Opcode::Subtract:
                                return left - right;
                            case Opcode::Multiply:
                                return left * right;
                            case Opcode::Divide:
                                return right != 0 ? left / right : std::numeric_limits<long double>::quiet_NaN();
                            default:
                                return std::numeric_limits<long double>::quiet_NaN();
                            }
                        };
                        number = source.bits == 32 ? compute(static_cast<float>(x), static_cast<float>(y))
                                                   : compute(static_cast<double>(x), static_cast<double>(y));
                        break;
                    }
                }
                if (result.kind == TypeKind::Floating && std::isfinite(number)) {
                    return Operand{OperandKind::Floating, in.result.type, 0, 0, number};
                }
                return {};
            }
            if (a.kind != OperandKind::Integer || source.kind != TypeKind::Integer) {
                return {};
            }
            auto x = a.integer & Mask(source);
            if (in.opcode == Opcode::Negate) {
                return integer(uint64_t{0} - x);
            }
            if (in.opcode == Opcode::Complement) {
                return integer(~x);
            }
            if (in.opcode == Opcode::LogicalNot) {
                return integer(x == 0);
            }
            if (b.kind != OperandKind::Integer || module.type(b.type).kind != TypeKind::Integer) {
                return {};
            }
            auto y = b.integer & Mask(module.type(b.type));
            auto sx = Signed(x, source), sy = Signed(y, module.type(b.type));
            switch (in.opcode) {
            case Opcode::Add:
                return integer(x + y);
            case Opcode::Subtract:
                return integer(x - y);
            case Opcode::Multiply:
                return integer(x * y);
            case Opcode::Divide:
            case Opcode::Remainder:
                if (!y) {
                    return {};
                }
                if (source.isSigned) {
                    // INT_MIN/-1 wraps for division; the remainder is zero.
                    if (sx == std::numeric_limits<int64_t>::min() && sy == -1) {
                        return integer(in.opcode == Opcode::Divide ? x : 0);
                    }
                    return integer(static_cast<uint64_t>(in.opcode == Opcode::Divide ? sx / sy : sx % sy));
                }
                return integer(in.opcode == Opcode::Divide ? x / y : x % y);
            case Opcode::ShiftLeft:
                if (y < source.bits) {
                    return integer(x << y);
                }
                return {};
            case Opcode::ShiftRight:
                if (y >= source.bits) {
                    return {};
                }
                if (!source.isSigned || sx >= 0 || y == 0) {
                    return integer(x >> y);
                }
                return integer((x >> y) | (~uint64_t{0} << (source.bits - y)));
            case Opcode::BitAnd:
                return integer(x & y);
            case Opcode::BitOr:
                return integer(x | y);
            case Opcode::BitXor:
                return integer(x ^ y);
            case Opcode::Equal:
                return integer(x == y);
            case Opcode::NotEqual:
                return integer(x != y);
            case Opcode::Less:
                return integer(source.isSigned ? sx < sy : x < y);
            case Opcode::Greater:
                return integer(source.isSigned ? sx > sy : x > y);
            case Opcode::LessEqual:
                return integer(source.isSigned ? sx <= sy : x <= y);
            case Opcode::GreaterEqual:
                return integer(source.isSigned ? sx >= sy : x >= y);
            default:
                return {};
            }
        }

        bool Removable(Opcode opcode) {
            // Preserve potentially faulting loads/divisions and all memory/call/control effects.
            return opcode == Opcode::Copy || opcode == Opcode::Cast || opcode == Opcode::AddressOffset ||
                   (opcode >= Opcode::Add && opcode <= Opcode::LogicalNot && opcode != Opcode::Divide &&
                    opcode != Opcode::Remainder);
        }
    }

    std::optional<Operand> FoldConstant(const Module& module, const Instruction& instruction) {
        return Fold(module, instruction);
    }

    void ConstantFold(Module& module) {
        for (auto& function : module.functions) {
            for (auto& block : function.blocks) {
                std::map<uint32_t, Operand> constants;
                for (auto& in : block.instructions) {
                    auto substitute = [&](Operand& operand) {
                        if (operand.kind == OperandKind::Temporary && constants.count(operand.id)) {
                            operand = constants.at(operand.id);
                        }
                    };
                    substitute(in.left);
                    substitute(in.right);
                    if (in.opcode == Opcode::Store) {
                        substitute(in.result);
                    }
                    if (DefinesTemporary(in)) {
                        constants.erase(in.result.id);
                        if (auto value = FoldConstant(module, in)) {
                            in.opcode = Opcode::Copy;
                            in.left = *value;
                            in.right = {};
                        }
                        if (in.opcode == Opcode::Copy &&
                            (in.left.kind == OperandKind::Integer || in.left.kind == OperandKind::Floating)) {
                            constants[in.result.id] = in.left;
                        }
                    }
                    if (in.opcode == Opcode::Branch && in.left.kind == OperandKind::Integer) {
                        auto destination = in.left.integer ? in.right : in.result;
                        in = {Opcode::Jump, {}, destination, {}, in.source};
                    }
                }
            }
        }
    }

    void CopyPropagate(Module& module) {
        for (auto& function : module.functions) {
            for (auto& block : function.blocks) {
                std::map<uint32_t, Operand> copies;
                for (auto& in : block.instructions) {
                    auto substitute = [&](Operand& operand) {
                        if (operand.kind == OperandKind::Temporary && copies.count(operand.id)) {
                            operand = copies.at(operand.id);
                        }
                    };
                    substitute(in.left);
                    substitute(in.right);
                    if (in.opcode == Opcode::Store) {
                        substitute(in.result);
                    }
                    if (!DefinesTemporary(in)) {
                        continue;
                    }
                    const auto id = in.result.id;
                    for (auto it = copies.begin(); it != copies.end();) {
                        if (it->first == id || (it->second.kind == OperandKind::Temporary && it->second.id == id)) {
                            it = copies.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    if (in.opcode == Opcode::Copy && !(in.left.kind == OperandKind::Temporary && in.left.id == id)) {
                        copies[id] = in.left;
                    }
                }
            }
        }
    }

    void EliminateDeadCode(Module& module) {
        for (auto& function : module.functions) {
            if (!function.defined || function.blocks.empty()) {
                continue;
            }
            BuildCFG(function);
            std::set<BlockId> reachable;
            std::vector<BlockId> pending{function.blocks.front().id};
            while (!pending.empty()) {
                auto id = pending.back();
                pending.pop_back();
                if (!reachable.insert(id).second) {
                    continue;
                }
                for (const auto& block : function.blocks) {
                    if (block.id == id) {
                        for (auto successor : block.successors) {
                            pending.push_back(successor);
                        }
                    }
                }
            }
            std::erase_if(function.blocks, [&](const Block& block) { return !reachable.count(block.id); });
            BuildCFG(function);
            std::map<BlockId, std::set<uint32_t>> liveIn, liveOut;
            bool changed = true;
            while (changed) {
                changed = false;
                for (auto it = function.blocks.rbegin(); it != function.blocks.rend(); ++it) {
                    std::set<uint32_t> out;
                    for (auto successor : it->successors) {
                        out.insert(liveIn[successor].begin(), liveIn[successor].end());
                    }
                    auto in = out;
                    for (auto instruction = it->instructions.rbegin(); instruction != it->instructions.rend();
                         ++instruction) {
                        if (DefinesTemporary(*instruction)) {
                            in.erase(instruction->result.id);
                        }
                        for (auto operand : Uses(*instruction)) {
                            if (operand.kind == OperandKind::Temporary) {
                                in.insert(operand.id);
                            }
                        }
                    }
                    if (liveIn[it->id] != in || liveOut[it->id] != out) {
                        changed = true;
                    }
                    liveIn[it->id] = std::move(in);
                    liveOut[it->id] = std::move(out);
                }
            }
            // Repeat local deletion to remove newly dead chains, using interblock
            // liveness each time through Optimize. Stores are never guessed dead.
            for (auto& block : function.blocks) {
                auto live = liveOut[block.id];
                std::vector<Instruction> kept;
                for (auto it = block.instructions.rbegin(); it != block.instructions.rend(); ++it) {
                    if (DefinesTemporary(*it) && !live.count(it->result.id) && Removable(it->opcode)) {
                        continue;
                    }
                    if (DefinesTemporary(*it)) {
                        live.erase(it->result.id);
                    }
                    for (auto operand : Uses(*it)) {
                        if (operand.kind == OperandKind::Temporary) {
                            live.insert(operand.id);
                        }
                    }
                    kept.push_back(*it);
                }
                std::reverse(kept.begin(), kept.end());
                block.instructions = std::move(kept);
            }
        }
    }

    void Optimize(Module& module) {
        ConstantFold(module);
        CopyPropagate(module);
        EliminateDeadCode(module);
        for (auto& function : module.functions) {
            BuildCFG(function);
        }
    }
}
