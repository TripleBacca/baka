#include "expressions.hh"
#include "memory/custom_arenas.hh"

namespace baka {
    namespace semantic {
        namespace {
            bool Assignment(types::ASTBinaryOp op) {
                return op >= types::OP_ASSIGN && op <= types::OP_RSHIFT_ASSIGN;
            }

            bool Comparison(types::ASTBinaryOp op) {
                return op >= types::OP_EQ && op <= types::OP_GE;
            }

            types::ASTBinaryOp Underlying(types::ASTBinaryOp op) {
                switch (op) {
                case types::OP_ADD_ASSIGN:
                    return types::OP_ADD;
                case types::OP_SUB_ASSIGN:
                    return types::OP_SUB;
                case types::OP_MUL_ASSIGN:
                    return types::OP_MUL;
                case types::OP_DIV_ASSIGN:
                    return types::OP_DIV;
                case types::OP_MOD_ASSIGN:
                    return types::OP_MOD;
                case types::OP_AND_ASSIGN:
                    return types::OP_BIT_AND;
                case types::OP_OR_ASSIGN:
                    return types::OP_BIT_OR;
                case types::OP_XOR_ASSIGN:
                    return types::OP_BIT_XOR;
                case types::OP_LSHIFT_ASSIGN:
                    return types::OP_LSHIFT;
                case types::OP_RSHIFT_ASSIGN:
                    return types::OP_RSHIFT;
                default:
                    return op;
                }
            }
        }

        types::ExpressionNode* ExpressionChecker::materializeBinary(types::BinaryExpressionNode* binary) {
            types::ExpressionNode* expression = binary;
            const auto boolean = TypeSystem.builtin(Builtin::Bool);
            auto op = binary->getBinaryOp();
            auto* left = binary->getLeft();
            auto* right = binary->getRight();
            TypeId lt{left->semanticInfo().Type}, rt{right->semanticInfo().Type};
            lt = decay(lt);
            rt = decay(rt);
            if (Assignment(op)) {
                auto* location = materializeImpl(left, false);
                if (op == types::OP_ASSIGN) {
                    binary->rewriteExpressions([&](auto* child) {
                        return child == left ? location : materializeImpl(child, true, TypeSystem.unConst(lt));
                    });
                } else {
                    // A saved-address placeholder avoids duplicating a[i++] or
                    // *get_pointer() when spelling the promoted read and store.
                    auto* load = base::ASTNodeArena::getInstance().Alloc<types::CompoundTargetNode>();
                    load->semanticInfo() = location->semanticInfo();
                    auto targetSpan = location->getSourceSpan();
                    if (targetSpan.Valid) {
                        load->setSourceSpan(targetSpan.Line, targetSpan.Col);
                    }
                    auto underlying = Underlying(op);
                    bool shift = underlying == types::OP_LSHIFT || underlying == types::OP_RSHIFT;
                    TypeId computation;
                    if (isPointer(lt)) {
                        computation = lt;
                    } else if (shift) {
                        computation = promote(lt);
                    } else {
                        computation = usualArithmetic(lt, rt, expression);
                    }
                    auto* lhs = convertValue(load, computation);
                    auto* rhs = materializeImpl(right, true, isPointer(lt) || shift ? promote(rt) : computation);
                    auto* operation =
                        base::ASTNodeArena::getInstance().Alloc<types::BinaryExpressionNode>(underlying, lhs, rhs);
                    operation->semanticInfo() = expression->semanticInfo();
                    operation->semanticInfo().Type = computation.value;
                    operation->semanticInfo().LValue = false;
                    auto span = expression->getSourceSpan();
                    if (span.Valid) {
                        operation->setSourceSpan(span.Line, span.Col);
                    }
                    auto* assigned = convertValue(operation, TypeSystem.unConst(lt));
                    auto* replacement =
                        base::ASTNodeArena::getInstance().Alloc<types::CompoundAssignmentNode>(location, assigned);
                    replacement->semanticInfo() = expression->semanticInfo();
                    if (span.Valid) {
                        replacement->setSourceSpan(span.Line, span.Col);
                    }
                    expression = replacement;
                }
            } else {
                TypeId leftTarget, rightTarget;
                if (op == types::OP_LOGICAL_AND || op == types::OP_LOGICAL_OR) {
                    leftTarget = rightTarget = boolean;
                } else if (op == types::OP_LSHIFT || op == types::OP_RSHIFT) {
                    leftTarget = promote(lt);
                    rightTarget = promote(rt);
                } else if (isArithmetic(lt) && isArithmetic(rt) && op != types::OP_COMMA) {
                    leftTarget = rightTarget = usualArithmetic(lt, rt, expression);
                } else if (Comparison(op)) {
                    if (isPointer(lt) && !isPointer(rt)) {
                        rightTarget = lt;
                    } else if (isPointer(rt) && !isPointer(lt)) {
                        leftTarget = rt;
                    } else if (isPointer(lt) && isPointer(rt)) {
                        auto lp = TypeSystem.get(lt).element, rp = TypeSystem.get(rt).element;
                        auto element = TypeSystem.isVoid(lp) ? lp : rp;
                        element = TypeSystem.constant(element, TypeSystem.isConst(lp) || TypeSystem.isConst(rp));
                        leftTarget = rightTarget = TypeSystem.pointer(element);
                    }
                } else if (isPointer(lt) && isInteger(rt)) {
                    rightTarget = promote(rt);
                } else if (isPointer(rt) && isInteger(lt)) {
                    leftTarget = promote(lt);
                }
                binary->rewriteExpressions([&](auto* child) {
                    return materializeImpl(child, true, child == left ? leftTarget : rightTarget);
                });
            }
            return expression;
        }

    }
}
