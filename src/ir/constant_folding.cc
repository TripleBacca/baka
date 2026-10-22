#include "constant_folding.hh"
#include "base/numeric.hh"
#include "base/target.hh"

namespace baka {
    namespace ir {
        namespace {
            bool IsLiteral(Operand value) {
                return value.kind == OperandKind::Integer || value.kind == OperandKind::Floating;
            }

            numeric::Type NumericType(const Type& type) {
                return {type.kind == TypeKind::Pointer ? target::PointerBits : type.bits, type.isSigned,
                        type.kind == TypeKind::Floating};
            }
        }

        std::optional<Operand> FoldConstant(const Module& module, const Instruction& in) {
            if (!DefinesTemporary(in)) {
                return {};
            }
            if (!IsLiteral(in.left)) {
                return {};
            }
            const auto source = NumericType(module.type(in.left.type));
            const auto result = NumericType(module.type(in.result.type));
            const numeric::Value a{in.left.integer, in.left.floating}, b{in.right.integer, in.right.floating};
            std::optional<numeric::Value> value;
            if (in.opcode == Opcode::Cast) {
                value = numeric::convert(a, source, result);
            } else {
                numeric::Op op;
                switch (in.opcode) {
                case Opcode::Add:
                    op = numeric::Op::Add;
                    break;
                case Opcode::Subtract:
                    op = numeric::Op::Subtract;
                    break;
                case Opcode::Multiply:
                    op = numeric::Op::Multiply;
                    break;
                case Opcode::Divide:
                    op = numeric::Op::Divide;
                    break;
                case Opcode::Remainder:
                    op = numeric::Op::Remainder;
                    break;
                case Opcode::ShiftLeft:
                    op = numeric::Op::ShiftLeft;
                    break;
                case Opcode::ShiftRight:
                    op = numeric::Op::ShiftRight;
                    break;
                case Opcode::BitAnd:
                    op = numeric::Op::BitAnd;
                    break;
                case Opcode::BitOr:
                    op = numeric::Op::BitOr;
                    break;
                case Opcode::BitXor:
                    op = numeric::Op::BitXor;
                    break;
                case Opcode::Equal:
                    op = numeric::Op::Equal;
                    break;
                case Opcode::NotEqual:
                    op = numeric::Op::NotEqual;
                    break;
                case Opcode::Less:
                    op = numeric::Op::Less;
                    break;
                case Opcode::Greater:
                    op = numeric::Op::Greater;
                    break;
                case Opcode::LessEqual:
                    op = numeric::Op::LessEqual;
                    break;
                case Opcode::GreaterEqual:
                    op = numeric::Op::GreaterEqual;
                    break;
                case Opcode::Negate:
                    op = numeric::Op::Negate;
                    break;
                case Opcode::Complement:
                    op = numeric::Op::Complement;
                    break;
                case Opcode::LogicalNot:
                    op = numeric::Op::LogicalNot;
                    break;
                default:
                    return {};
                }
                const bool unary =
                    in.opcode == Opcode::Negate || in.opcode == Opcode::Complement || in.opcode == Opcode::LogicalNot;
                if (!unary && !IsLiteral(in.right)) {
                    return {};
                }
                value = numeric::evaluate(op, a, source, b,
                                          unary ? numeric::Type{} : NumericType(module.type(in.right.type)), result);
            }
            if (!value) {
                return {};
            }
            return Operand{module.type(in.result.type).kind == TypeKind::Floating ? OperandKind::Floating
                                                                                  : OperandKind::Integer,
                           in.result.type, 0, value->bits, value->floating};
        }
    }
}
