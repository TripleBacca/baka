#include "lowering.hh"
#include <algorithm>
#include <bit>
#include <limits>

namespace baka {
    namespace ir {
        Operand Lowerer::binaryValue(const types::BinaryExpressionNode* binaryNode) {
            const auto resultType = type(binaryNode);
            auto opcode = binaryNode->getBinaryOp();
            if (opcode == types::OP_ASSIGN) {
                auto location = address(binaryNode->getLeft());
                auto result = value(binaryNode->getRight());
                store(location, result);
                return result;
            }
            auto left = value(binaryNode->getLeft());
            if (opcode == types::OP_LOGICAL_AND || opcode == types::OP_LOGICAL_OR) {
                auto rightBlock = newBlock(), shortBlock = newBlock(), join = newBlock();
                auto result = temporary(resultType);
                bool conjunction = opcode == types::OP_LOGICAL_AND;
                branch(left, conjunction ? rightBlock : shortBlock, conjunction ? shortBlock : rightBlock);
                select(shortBlock);
                emit(Opcode::Copy, result, integer(resultType, conjunction ? 0 : 1));
                jump(join);
                select(rightBlock);
                auto right = value(binaryNode->getRight());
                emit(Opcode::Copy, result, cast(right, resultType));
                jump(join);
                select(join);
                return result;
            }
            auto right = value(binaryNode->getRight());
            if (opcode == types::OP_COMMA) {
                return right;
            }
            return binary(opcode, left, right, resultType);
        }

        Operand Lowerer::unaryValue(const types::FactorNode* unary) {
            const auto resultType = type(unary);
            using U = types::ASTUnaryOp;
            auto opcode = unary->getUnaryOp();
            if (opcode == U::OP_ADDR_OF) {
                return cast(address(unary->getOperand()), resultType);
            }
            if (opcode == U::OP_DEREF) {
                auto pointer = value(unary->getOperand());
                return Result.type(resultType).kind == TypeKind::Function ? pointer : load(pointer, resultType);
            }
            if (opcode == U::OP_INC || opcode == U::OP_DEC) {
                return increment(unary->getOperand(), opcode == U::OP_DEC, false);
            }
            auto operand = value(unary->getOperand());
            if (opcode == U::OP_UNARY_PLUS || opcode == U::OP_NOOP) {
                return operand;
            }
            if (opcode == U::OP_UNARY_MINUS) {
                return emitValue(Opcode::Negate, resultType, operand);
            }
            if (opcode == U::OP_TILDE) {
                return emitValue(Opcode::Complement, resultType, operand);
            }
            if (opcode == U::OP_LOGICAL_NOT) {
                return emitValue(Opcode::LogicalNot, resultType, operand);
            }
            fail(unary, "unsupported unary operator in IR");
        }

        Operand Lowerer::compoundAssignment(const types::CompoundAssignmentNode* compound, bool returnAddress) {
            auto target = address(compound->getTarget());
            State.SavedTargets.push_back(target);
            auto result = value(compound->getValue());
            State.SavedTargets.pop_back();
            store(target, result);
            return returnAddress ? target : result;
        }

        Operand Lowerer::binary(types::ASTBinaryOp opcode, Operand left, Operand right, TypeId result) {
            auto leftType = Result.type(left.type), rightType = Result.type(right.type);
            if ((opcode == types::OP_ADD || opcode == types::OP_SUB) &&
                (leftType.kind == TypeKind::Pointer || rightType.kind == TypeKind::Pointer)) {
                if (leftType.kind == TypeKind::Pointer && rightType.kind == TypeKind::Pointer) {
                    auto lhs = cast(left, integerType()), rhs = cast(right, integerType());
                    auto bytes = emitValue(Opcode::Subtract, integerType(), lhs, rhs);
                    auto size = Result.type(leftType.element).size;
                    if (!size) {
                        fail(nullptr, "pointer difference has an unsized pointee");
                    }
                    return cast(emitValue(Opcode::Divide, integerType(), bytes, integer(integerType(), size)), result);
                }
                auto pointer = leftType.kind == TypeKind::Pointer ? left : right;
                auto index = leftType.kind == TypeKind::Pointer ? right : left;
                auto element = Result.type(Result.type(pointer.type).element);
                if (!element.size) {
                    fail(nullptr, "pointer arithmetic has an unsized pointee");
                }
                auto bytes = emitValue(Opcode::Multiply, integerType(), cast(index, integerType()),
                                       integer(integerType(), element.size));
                if (opcode == types::OP_SUB) {
                    bytes = emitValue(Opcode::Negate, integerType(), bytes);
                }
                return emitValue(Opcode::AddressOffset, result, pointer, bytes);
            }
            Opcode operation;
            switch (opcode) {
            case types::OP_ADD:
                operation = Opcode::Add;
                break;
            case types::OP_SUB:
                operation = Opcode::Subtract;
                break;
            case types::OP_MUL:
                operation = Opcode::Multiply;
                break;
            case types::OP_DIV:
                operation = Opcode::Divide;
                break;
            case types::OP_MOD:
                operation = Opcode::Remainder;
                break;
            case types::OP_LSHIFT:
                operation = Opcode::ShiftLeft;
                break;
            case types::OP_RSHIFT:
                operation = Opcode::ShiftRight;
                break;
            case types::OP_BIT_AND:
                operation = Opcode::BitAnd;
                break;
            case types::OP_BIT_OR:
                operation = Opcode::BitOr;
                break;
            case types::OP_BIT_XOR:
                operation = Opcode::BitXor;
                break;
            case types::OP_EQ:
                operation = Opcode::Equal;
                break;
            case types::OP_NEQ:
                operation = Opcode::NotEqual;
                break;
            case types::OP_LT:
                operation = Opcode::Less;
                break;
            case types::OP_GT:
                operation = Opcode::Greater;
                break;
            case types::OP_LE:
                operation = Opcode::LessEqual;
                break;
            case types::OP_GE:
                operation = Opcode::GreaterEqual;
                break;

            default:
                fail(nullptr, "unmaterialized or unsupported binary operator in IR");
            }
            return emitValue(operation, result, left, right);
        }

        Operand Lowerer::increment(const types::ExpressionNode* target, bool decrement, bool postfix,
                                   bool returnAddress) {
            auto location = address(target);
            auto targetType = type(target);
            auto old = load(location, targetType);
            auto info = Result.type(targetType);
            Operand next;
            if (info.kind == TypeKind::Pointer) {
                auto delta = Result.type(info.element).size;
                if (!delta) {
                    fail(target, "pointer increment has an unsized pointee");
                }
                next = emitValue(Opcode::AddressOffset, targetType, old,
                                 integer(integerType(), decrement ? uint64_t{0} - delta : delta));
            } else {
                // Use the frontend's integral-promotion rule before the writeback.
                // For bool this is essential: converting 2 to bool yields true,
                // whereas doing addition directly in a u1 temporary would wrap to 0.
                auto computation = info.kind == TypeKind::Integer && info.bits < 32
                                       ? importType(Analyzer.types().builtin(semantic::Builtin::Int))
                                       : targetType;
                auto promoted = cast(old, computation);
                auto one = info.kind == TypeKind::Floating ? Operand{OperandKind::Floating, computation, 0, 0, 1}
                                                           : integer(computation, 1);
                next =
                    cast(emitValue(decrement ? Opcode::Subtract : Opcode::Add, computation, promoted, one), targetType);
            }
            store(location, next);
            return returnAddress ? location : postfix ? old : next;
        }
    }
}
