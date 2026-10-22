#include "expressions.hh"

namespace baka {
    namespace semantic {
        namespace {
            bool IsComparison(types::ASTBinaryOp op) {
                switch (op) {
                case types::OP_EQ:
                case types::OP_NEQ:
                case types::OP_LT:
                case types::OP_GT:
                case types::OP_LE:
                case types::OP_GE:
                    return true;
                default:
                    return false;
                }
            }

            bool IsAssignment(types::ASTBinaryOp op) {
                switch (op) {
                case types::OP_ASSIGN:
                case types::OP_ADD_ASSIGN:
                case types::OP_SUB_ASSIGN:
                case types::OP_MUL_ASSIGN:
                case types::OP_DIV_ASSIGN:
                case types::OP_MOD_ASSIGN:
                case types::OP_AND_ASSIGN:
                case types::OP_OR_ASSIGN:
                case types::OP_XOR_ASSIGN:
                case types::OP_LSHIFT_ASSIGN:
                case types::OP_RSHIFT_ASSIGN:
                    return true;
                default:
                    return false;
                }
            }

            bool IsBitwise(types::ASTBinaryOp op) {
                return op == types::OP_BIT_AND || op == types::OP_BIT_XOR || op == types::OP_BIT_OR;
            }

            bool IsShift(types::ASTBinaryOp op) {
                return op == types::OP_LSHIFT || op == types::OP_RSHIFT;
            }

            bool IsArithmeticOperation(types::ASTBinaryOp op) {
                return op == types::OP_MUL || op == types::OP_DIV || op == types::OP_MOD || op == types::OP_ADD ||
                       op == types::OP_SUB;
            }

        }

        ExprResult ExpressionChecker::checkUnary(const types::FactorNode* node) {
            auto op = node->getUnaryOp();
            auto* operandNode = node->getOperand();
            if (!operandNode) {
                return makeError();
            }
            if (op == types::ASTUnaryOp::OP_ADDR_OF) {
                auto operand = check(operandNode, false);
                if (operand.error) {
                    return operand;
                }
                if (operand.boundInstanceFunction) {
                    SymbolTable.diagnose("member-function",
                                         "instance member function cannot form an ordinary function pointer", node);
                    return makeError();
                }
                auto kind = TypeSystem.get(TypeSystem.unConst(operand.type)).kind;
                // Address-of suppresses array-to-pointer decay: &a points to
                // the entire array, preserving its bounds and element qualifiers.
                if (kind == TypeKind::Function) {
                    return makeValue(TypeSystem.pointer(operand.type));
                }
                if (operand.category != ValueCategory::LValue) {
                    SymbolTable.diagnose("address", "address-of requires an lvalue", node);
                    return makeError();
                }
                return makeValue(TypeSystem.pointer(operand.type));
            }
            auto operand = check(operandNode, true);
            if (operand.error) {
                return operand;
            }
            switch (op) {
            case types::ASTUnaryOp::OP_ADDR_OF:
                return makeError();
            case types::ASTUnaryOp::OP_DEREF: {
                if (!isPointer(operand.type)) {
                    SymbolTable.diagnose("dereference", "dereference requires a pointer", node);
                    return makeError();
                }
                auto element = TypeSystem.get(TypeSystem.unConst(operand.type)).element;
                // Dereference designates the pointee; it is not a condition.
                // Records and arrays are valid lvalues, and function pointers
                // designate functions. Keep pointee qualifiers here; value
                // conversion, completeness and condition checks belong to use
                // sites. Void is the exception: it designates no object/function.
                if (TypeSystem.isVoid(element)) {
                    SymbolTable.diagnose("dereference", "cannot dereference a pointer to void", node);
                    return makeError();
                }
                return makeValue(element, ValueCategory::LValue);
            }
            case types::ASTUnaryOp::OP_INC:
            case types::ASTUnaryOp::OP_DEC: {
                auto raw = check(operandNode, false);
                if (raw.error) {
                    return raw;
                }
                if (!isModifiableLValue(raw)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a modifiable lvalue", node);
                    return makeError();
                }
                if (!isScalar(raw.type)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a scalar", node);
                    return makeError();
                }
                if (isPointer(raw.type) && !isCompleteObject(TypeSystem.get(TypeSystem.unConst(raw.type)).element)) {
                    SymbolTable.diagnose("mutation", "pointer increment requires a complete object pointee", node);
                    return makeError();
                }
                raw.category = ValueCategory::LValue;
                raw.constantExpression = false;
                return raw;
            }
            case types::ASTUnaryOp::OP_LOGICAL_NOT: {
                if (!checkScalarCondition(operand, node)) {
                    return makeError();
                }
                auto truth = convertConstant(operand, TypeSystem.builtin(Builtin::Bool));
                return truth.constantExpression ? makeBool(!truth.constant.value) : makeValue(truth.type);
            }
            case types::ASTUnaryOp::OP_TILDE: {
                if (!isInteger(operand.type)) {
                    SymbolTable.diagnose("operator", "bitwise complement requires an integer", node);
                    return makeError();
                }
                auto type = promote(operand.type);
                auto converted = convertConstant(operand, type);
                return converted.constantExpression ? makeConstant(type, ~converted.constant.value) : makeValue(type);
            }
            case types::ASTUnaryOp::OP_UNARY_PLUS:
            case types::ASTUnaryOp::OP_UNARY_MINUS: {
                if (!isArithmetic(operand.type)) {
                    SymbolTable.diagnose("operator", "unary sign requires an arithmetic operand", node);
                    return makeError();
                }
                auto type = promote(operand.type);
                if (op == types::ASTUnaryOp::OP_UNARY_PLUS) {
                    return convertConstant(operand, type);
                }
                return negateConstant(operand, type);
            }
            case types::ASTUnaryOp::OP_SIZEOF:
                SymbolTable.diagnose("sizeof", "malformed sizeof operand", node);
                return makeError();
            case types::ASTUnaryOp::OP_NOOP:
                return operand;
            }
            return makeError();
        }

        ExprResult ExpressionChecker::checkAssignment(const types::BinaryExpressionNode* node) {
            auto left = check(node->getLeft(), false);
            auto right = check(node->getRight(), true);
            if (left.error || right.error) {
                return makeError();
            }
            if (!isModifiableLValue(left)) {
                SymbolTable.diagnose("assignment", "assignment target is not a modifiable lvalue", node);
                return makeError();
            }
            auto op = node->getBinaryOp();
            if (op == types::OP_ASSIGN) {
                if (!isAssignable(right.type, left.type, right)) {
                    SymbolTable.diagnose("conversion", "incompatible assignment conversion", node);
                    return makeError();
                }
            } else {
                auto underlying = op;
                switch (op) {
                case types::OP_ADD_ASSIGN:
                    underlying = types::OP_ADD;
                    break;
                case types::OP_SUB_ASSIGN:
                    underlying = types::OP_SUB;
                    break;
                case types::OP_MUL_ASSIGN:
                    underlying = types::OP_MUL;
                    break;
                case types::OP_DIV_ASSIGN:
                    underlying = types::OP_DIV;
                    break;
                case types::OP_MOD_ASSIGN:
                    underlying = types::OP_MOD;
                    break;
                case types::OP_AND_ASSIGN:
                    underlying = types::OP_BIT_AND;
                    break;
                case types::OP_OR_ASSIGN:
                    underlying = types::OP_BIT_OR;
                    break;
                case types::OP_XOR_ASSIGN:
                    underlying = types::OP_BIT_XOR;
                    break;
                case types::OP_LSHIFT_ASSIGN:
                    underlying = types::OP_LSHIFT;
                    break;
                case types::OP_RSHIFT_ASSIGN:
                    underlying = types::OP_RSHIFT;
                    break;
                default:
                    break;
                }
                bool pointerArithmetic = isPointer(left.type) &&
                                         (underlying == types::OP_ADD || underlying == types::OP_SUB) &&
                                         isInteger(right.type);
                if (pointerArithmetic) {
                    // valid pointer +/-= integer
                } else if (IsShift(underlying)) {
                    if (!isInteger(left.type) || !isInteger(right.type)) {
                        SymbolTable.diagnose("operator", "shift requires integer operands", node);
                        return makeError();
                    }
                } else if (IsBitwise(underlying)) {
                    if (!isInteger(left.type) || !isInteger(right.type)) {
                        SymbolTable.diagnose("operator", "bitwise operator requires integers", node);
                        return makeError();
                    }
                } else if (!isArithmetic(left.type) || !isArithmetic(right.type)) {
                    SymbolTable.diagnose("operator", "compound assignment requires arithmetic operands", node);
                    return makeError();
                }
            }
            auto result = left;
            result.category = ValueCategory::LValue;
            result.constantExpression = false;
            return result;
        }

        ExprResult ExpressionChecker::checkBinary(const types::BinaryExpressionNode* node) {
            auto op = node->getBinaryOp();
            if (IsAssignment(op)) {
                return checkAssignment(node);
            }
            if (op == types::OP_COMMA) {
                auto left = check(node->getLeft(), true);
                (void)left;
                auto result = check(node->getRight(), true);
                result.constantExpression = false;
                result.constant = {};
                return result;
            }
            if (op == types::OP_DOT || op == types::OP_ARROW) {
                SymbolTable.diagnose("member", "member access requires an identifier", node);
                return makeError();
            }
            auto left = check(node->getLeft(), true);
            if (left.error) {
                return makeError();
            }
            if (op == types::OP_LOGICAL_AND || op == types::OP_LOGICAL_OR) {
                if (!checkScalarCondition(left, node)) {
                    return makeError();
                }
                auto truth = convertConstant(left, TypeSystem.builtin(Builtin::Bool));
                bool definite = truth.constantExpression;
                bool leftTruth = definite && truth.constant.value != 0;
                bool decisive = definite && ((op == types::OP_LOGICAL_AND && !leftTruth) ||
                                             (op == types::OP_LOGICAL_OR && leftTruth));
                auto right = check(node->getRight(), true);
                if (right.error) {
                    return makeError();
                }
                if (!checkScalarCondition(right, node)) {
                    return makeError();
                }
                if (decisive) {
                    return makeBool(leftTruth);
                }
                auto rightTruth = convertConstant(right, TypeSystem.builtin(Builtin::Bool));
                if (definite && rightTruth.constantExpression) {
                    return makeBool(rightTruth.constant.value != 0);
                }
                return makeValue(TypeSystem.builtin(Builtin::Bool));
            }
            auto right = check(node->getRight(), true);
            if (right.error) {
                return makeError();
            }
            if (IsComparison(op)) {
                bool equality = op == types::OP_EQ || op == types::OP_NEQ;
                if (isArithmetic(left.type) && isArithmetic(right.type)) {
                    auto common = usualArithmetic(left.type, right.type, node);
                    return foldBinary(op, left, right, common, TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(left.type) && isPointer(right.type)) {
                    auto leftPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(left.type)).element);
                    auto rightPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(right.type)).element);
                    bool compatible = leftPointee == rightPointee || TypeSystem.isVoid(leftPointee) ||
                                      TypeSystem.isVoid(rightPointee);
                    if (!compatible) {
                        SymbolTable.diagnose("comparison", "incompatible pointer comparison", node);
                        return makeError();
                    }
                    if (!equality) {
                        auto leftKind = TypeSystem.get(leftPointee).kind;
                        auto rightKind = TypeSystem.get(rightPointee).kind;
                        if (leftKind == TypeKind::Function || rightKind == TypeKind::Function ||
                            TypeSystem.isVoid(leftPointee) || TypeSystem.isVoid(rightPointee)) {
                            SymbolTable.diagnose("comparison", "ordering comparison requires object pointers", node);
                            return makeError();
                        }
                    }
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(left.type) && isNullPointerConstant(right)) {
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(right.type) && isNullPointerConstant(left)) {
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                SymbolTable.diagnose("comparison", "incompatible comparison operands", node);
                return makeError();
            }
            if (IsShift(op)) {
                if (!isInteger(left.type) || !isInteger(right.type)) {
                    SymbolTable.diagnose("operator", "shift requires integer operands", node);
                    return makeError();
                }
                auto type = promote(left.type);
                return foldBinary(op, left, right, type, type, promote(right.type));
            }
            if (IsBitwise(op)) {
                if (!isInteger(left.type) || !isInteger(right.type)) {
                    SymbolTable.diagnose("operator", "bitwise operator requires integers", node);
                    return makeError();
                }
                auto type = usualArithmetic(left.type, right.type, node);
                if (!type.value) {
                    return makeError();
                }
                return foldBinary(op, left, right, type, type);
            }
            if (IsArithmeticOperation(op)) {
                if (isPointer(left.type) || isPointer(right.type)) {
                    if ((op == types::OP_ADD || op == types::OP_SUB)) {
                        if (isPointer(left.type) && isInteger(right.type)) {
                            if (!isCompleteObject(TypeSystem.get(TypeSystem.unConst(left.type)).element)) {
                                SymbolTable.diagnose("pointer", "pointer arithmetic requires a complete object type",
                                                     node);
                                return makeError();
                            }
                            return makeValue(left.type);
                        }
                        if (op == types::OP_ADD && isInteger(left.type) && isPointer(right.type)) {
                            if (!isCompleteObject(TypeSystem.get(TypeSystem.unConst(right.type)).element)) {
                                SymbolTable.diagnose("pointer", "pointer arithmetic requires a complete object type",
                                                     node);
                                return makeError();
                            }
                            return makeValue(right.type);
                        }
                        if (op == types::OP_SUB && isPointer(left.type) && isPointer(right.type)) {
                            auto leftPointee =
                                TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(left.type)).element);
                            auto rightPointee =
                                TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(right.type)).element);
                            if (leftPointee != rightPointee || !isCompleteObject(leftPointee)) {
                                SymbolTable.diagnose("pointer", "pointer difference requires compatible pointers",
                                                     node);
                                return makeError();
                            }
                            return makeValue(TypeSystem.builtin(Builtin::Long));
                        }
                    }
                    SymbolTable.diagnose("operator", "invalid pointer arithmetic", node);
                    return makeError();
                }
                if (!isArithmetic(left.type) || !isArithmetic(right.type)) {
                    SymbolTable.diagnose("operator", "arithmetic operator requires arithmetic operands", node);
                    return makeError();
                }
                if (op == types::OP_MOD && (!isInteger(left.type) || !isInteger(right.type))) {
                    SymbolTable.diagnose("operator", "remainder requires integer operands", node);
                    return makeError();
                }
                auto type = usualArithmetic(left.type, right.type, node);
                if (!type.value) {
                    return makeError();
                }
                return foldBinary(op, left, right, type, type);
            }
            SymbolTable.diagnose("operator", "unsupported binary operator", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkTernary(const types::TernaryOpNode* node) {
            auto condition = check(node->getCondition(), true);
            auto thenValue = check(node->getThen(), false);
            auto elseValue = check(node->getElse(), false);
            if (condition.error || thenValue.error || elseValue.error) {
                return makeError();
            }
            if (!checkScalarCondition(condition, node)) {
                return makeError();
            }
            const bool lvalue = thenValue.type == elseValue.type && thenValue.category == ValueCategory::LValue &&
                                elseValue.category == ValueCategory::LValue;
            if (!lvalue) {
                thenValue = check(node->getThen(), true);
                elseValue = check(node->getElse(), true);
            }
            TypeId type;
            if (lvalue) {
                type = thenValue.type;
            } else if (isArithmetic(thenValue.type) && isArithmetic(elseValue.type)) {
                type = usualArithmetic(thenValue.type, elseValue.type, node);
            } else if (isPointer(thenValue.type) && isPointer(elseValue.type)) {
                auto leftPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(thenValue.type)).element);
                auto rightPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(elseValue.type)).element);
                bool qualified = TypeSystem.isConst(TypeSystem.get(TypeSystem.unConst(thenValue.type)).element) ||
                                 TypeSystem.isConst(TypeSystem.get(TypeSystem.unConst(elseValue.type)).element);
                if (leftPointee == rightPointee) {
                    type = TypeSystem.pointer(TypeSystem.constant(leftPointee, qualified));
                } else if (TypeSystem.isVoid(leftPointee) || TypeSystem.isVoid(rightPointee)) {
                    type = TypeSystem.pointer(TypeSystem.constant(TypeSystem.builtin(Builtin::Void), qualified));
                }
            } else if (isPointer(thenValue.type) && isNullPointerConstant(elseValue)) {
                type = thenValue.type;
            } else if (isPointer(elseValue.type) && isNullPointerConstant(thenValue)) {
                type = elseValue.type;
            } else if (TypeSystem.unConst(thenValue.type) == TypeSystem.unConst(elseValue.type)) {
                type = thenValue.type;
            }
            if (!type.value) {
                SymbolTable.diagnose("conditional", "incompatible conditional operand types", node);
                return makeError();
            }
            auto truth = convertConstant(condition, TypeSystem.builtin(Builtin::Bool));
            auto result = truth.constantExpression ? convertConstant(truth.constant.value ? thenValue : elseValue, type)
                                                   : makeValue(type);
            if (lvalue) {
                result.category = ValueCategory::LValue;
            }
            return result;
        }

        ExprResult ExpressionChecker::checkCast(const types::CastFactorNode* node) {
            if (!node->getType()) {
                return makeError();
            }
            auto target = node->getType()->semanticInfo().Resolved ? TypeId{node->getType()->semanticInfo().Type}
                                                                   : ASTResolver.typeName(*node->getType());
            if (!node->getType()->semanticInfo().Resolved && node->getDeclarator()) {
                target = ASTResolver.declarator(target, node->getDeclarator());
            }
            if (!target.value) {
                return makeError();
            }
            auto source = check(node->getExpression(), true);
            if (source.error) {
                return makeError();
            }
            auto targetKind = TypeSystem.get(TypeSystem.unConst(target)).kind;
            if (targetKind == TypeKind::Array || targetKind == TypeKind::Function) {
                SymbolTable.diagnose("cast", "cast target cannot be an array or function", node);
                return makeError();
            }
            if (isArithmetic(target) && isArithmetic(source.type)) {
                return convertConstant(source, target);
            }
            if (isPointer(target) && isPointer(source.type)) {
                return makeValue(target);
            }
            if (isPointer(target) && isInteger(source.type)) {
                return makeValue(target);
            }
            if (isInteger(target) && isPointer(source.type)) {
                return makeValue(target);
            }
            if (TypeSystem.unConst(target) == TypeSystem.unConst(source.type)) {
                return makeValue(target);
            }
            SymbolTable.diagnose("cast", "unsupported cast conversion", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkSizeof(const types::SizeofNode* node) {
            TypeId measured;
            const auto& payload = node->getPayload();
            if (std::holds_alternative<types::SizeofNode::SizeofTypeVariant>(payload)) {
                const auto& variant = std::get<types::SizeofNode::SizeofTypeVariant>(payload);
                if (!variant.Type) {
                    return makeError();
                }
                measured = variant.Type->semanticInfo().Resolved ? TypeId{variant.Type->semanticInfo().Type}
                                                                 : ASTResolver.typeName(*variant.Type);
                if (!variant.Type->semanticInfo().Resolved && variant.AbstractDecl) {
                    measured = ASTResolver.declarator(measured, variant.AbstractDecl);
                }
            } else {
                auto* expression = std::get<types::ExpressionNode*>(payload);
                if (!expression) {
                    return makeError();
                }
                auto value = check(expression, false);
                if (value.error) {
                    return makeError();
                }
                measured = value.type;
            }
            if (!measured.value) {
                return makeError();
            }
            auto layout = TypeSystem.layout(measured);
            if (!layout) {
                SymbolTable.diagnose(
                    "sizeof", layout.message.empty() ? "sizeof requires a complete object type" : layout.message, node);
                return makeError();
            }
            return makeConstant(TypeSystem.sizeType(), static_cast<int64_t>(layout.layout.size));
        }

        ExprResult ExpressionChecker::checkPostfix(const types::UnaryPostfixExpr* postfix) {
            auto operand = check(postfix->getOperand(), false);
            if (operand.error) {
                return operand;
            }
            if (!isModifiableLValue(operand)) {
                SymbolTable.diagnose("mutation", "increment/decrement requires a modifiable lvalue", postfix);
                return makeError();
            }
            if (!isScalar(operand.type)) {
                SymbolTable.diagnose("mutation", "increment/decrement requires a scalar", postfix);
                return makeError();
            }
            if (isPointer(operand.type) &&
                !isCompleteObject(TypeSystem.get(TypeSystem.unConst(operand.type)).element)) {
                SymbolTable.diagnose("mutation", "pointer increment requires a complete object pointee", postfix);
                return makeError();
            }
            operand.constantExpression = false;
            operand.constant = {};
            operand.category = ValueCategory::RValue;
            return operand;
        }

    }
}
