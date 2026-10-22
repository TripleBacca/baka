#include "expressions.hh"
#include <bit>
#include <bit>

namespace baka {
    namespace semantic {
        numeric::Type ExpressionChecker::numericType(TypeId type) const {
            auto info = TypeSystem.arithmetic(type);
            return info ? numeric::Type{info->bits, info->isSigned, info->floating} : numeric::Type{};
        }

        ExprResult ExpressionChecker::convertConstant(const ExprResult& source, TypeId target) const {
            ExprResult result;
            result.type = target;
            result.error = source.error;
            if (!source.constantExpression ||
                (!source.floatingConstant && source.constant.state != ConstantState::Value)) {
                return result;
            }
            numeric::Value value{static_cast<uint64_t>(source.constant.value), source.floatingConstant.value_or(0)};
            auto converted = numeric::convert(value, numericType(source.type), numericType(target));
            if (!converted) {
                return result;
            }
            result.constantExpression = true;
            auto type = numericType(target);
            if (type.floating) {
                result.floatingConstant = converted->floating;
            } else {
                auto bits = converted->bits;
                result.constant = {ConstantState::Value,
                                   type.isSigned ? numeric::signedValue(bits, type) : std::bit_cast<int64_t>(bits)};
                result.nullPointerConstant = bits == 0;
            }
            return result;
        }

        ExprResult ExpressionChecker::negateConstant(const ExprResult& source, TypeId target) const {
            auto result = convertConstant(source, target);
            if (!result.constantExpression) {
                return result;
            }
            auto type = numericType(target);
            auto value =
                numeric::evaluate(numeric::Op::Negate,
                                  {static_cast<uint64_t>(result.constant.value), result.floatingConstant.value_or(0)},
                                  type, {}, {}, type);
            if (type.floating) {
                result.floatingConstant = value->floating;
            } else {
                result.constant.value = std::bit_cast<int64_t>(value->bits);
            }
            return convertConstant(result, target);
        }

        ExprResult ExpressionChecker::foldBinary(types::ASTBinaryOp op, const ExprResult& left, const ExprResult& right,
                                                 TypeId computation, TypeId resultType, TypeId rightType) const {
            auto a = convertConstant(left, computation);
            auto b = convertConstant(right, rightType.value ? rightType : computation);
            ExprResult result;
            result.type = resultType;
            if (!a.constantExpression || !b.constantExpression) {
                return result;
            }
            numeric::Op operation;
            switch (op) {
            case types::OP_ADD:
                operation = numeric::Op::Add;
                break;
            case types::OP_SUB:
                operation = numeric::Op::Subtract;
                break;
            case types::OP_MUL:
                operation = numeric::Op::Multiply;
                break;
            case types::OP_DIV:
                operation = numeric::Op::Divide;
                break;
            case types::OP_MOD:
                operation = numeric::Op::Remainder;
                break;
            case types::OP_LSHIFT:
                operation = numeric::Op::ShiftLeft;
                break;
            case types::OP_RSHIFT:
                operation = numeric::Op::ShiftRight;
                break;
            case types::OP_BIT_AND:
                operation = numeric::Op::BitAnd;
                break;
            case types::OP_BIT_OR:
                operation = numeric::Op::BitOr;
                break;
            case types::OP_BIT_XOR:
                operation = numeric::Op::BitXor;
                break;
            case types::OP_EQ:
                operation = numeric::Op::Equal;
                break;
            case types::OP_NEQ:
                operation = numeric::Op::NotEqual;
                break;
            case types::OP_LT:
                operation = numeric::Op::Less;
                break;
            case types::OP_GT:
                operation = numeric::Op::Greater;
                break;
            case types::OP_LE:
                operation = numeric::Op::LessEqual;
                break;
            case types::OP_GE:
                operation = numeric::Op::GreaterEqual;
                break;
            default:
                return result;
            }
            auto value = numeric::evaluate(
                operation, {static_cast<uint64_t>(a.constant.value), a.floatingConstant.value_or(0)},
                numericType(a.type), {static_cast<uint64_t>(b.constant.value), b.floatingConstant.value_or(0)},
                numericType(b.type), numericType(resultType));
            if (!value) {
                return result;
            }
            ExprResult raw;
            raw.type = resultType;
            raw.constantExpression = true;
            if (numericType(resultType).floating) {
                raw.floatingConstant = value->floating;
            } else {
                raw.constant = {ConstantState::Value, std::bit_cast<int64_t>(value->bits)};
            }
            return convertConstant(raw, resultType);
        }
    }
}
