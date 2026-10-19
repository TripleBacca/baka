#include "expressions.hh"
#include "memory/custom_arenas.hh"
#include <cmath>
#include <limits>

namespace baka::semantic {
    namespace {
        bool Assignment(types::ASTBinaryOp op) {
            return op >= types::OP_ASSIGN && op <= types::OP_RSHIFT_ASSIGN;
        }
        bool Comparison(types::ASTBinaryOp op) { return op >= types::OP_EQ && op <= types::OP_GE; }
        types::ASTBinaryOp Underlying(types::ASTBinaryOp op) {
            switch (op) {
            case types::OP_ADD_ASSIGN: return types::OP_ADD;
            case types::OP_SUB_ASSIGN: return types::OP_SUB;
            case types::OP_MUL_ASSIGN: return types::OP_MUL;
            case types::OP_DIV_ASSIGN: return types::OP_DIV;
            case types::OP_MOD_ASSIGN: return types::OP_MOD;
            case types::OP_AND_ASSIGN: return types::OP_BIT_AND;
            case types::OP_OR_ASSIGN: return types::OP_BIT_OR;
            case types::OP_XOR_ASSIGN: return types::OP_BIT_XOR;
            case types::OP_LSHIFT_ASSIGN: return types::OP_LSHIFT;
            case types::OP_RSHIFT_ASSIGN: return types::OP_RSHIFT;
            default: return op;
            }
        }
    }

    void ExpressionChecker::planConversion(const types::ExpressionNode* expression, TypeId target) {
        if (!Annotating || !expression) return;
        // InitializerNode is syntax, never a runtime operand. Push a scalar
        // destination down through transparent/single-element brace wrappers.
        while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
            if (list->expressions().size() != 1) return;
            expression = list->expressions().front();
        }
        ConversionTargets[expression] = TypeSystem.unConst(target);
    }

    bool ExpressionChecker::isBoundInstanceFunction(const types::ExpressionNode* expression) const {
        while (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
            if (comma->getExpressions().size() != 1) break;
            expression = comma->getExpressions().front();
        }
        if (!expression) return false;
        const auto& metadata = expression->semanticInfo();
        if (!metadata.MemberOwner || !metadata.MemberIndex || !metadata.Type ||
            TypeSystem.get(TypeSystem.unConst({metadata.Type})).kind != TypeKind::Function) return false;
        const auto& members = TypeSystem.get({metadata.MemberOwner}).members;
        if (metadata.MemberIndex > members.size()) return false;
        const auto& member = members[metadata.MemberIndex - 1];
        return !member.isStatic && TypeSystem.get(TypeSystem.unConst(member.type)).kind == TypeKind::Function;
    }

    types::ExpressionNode* ExpressionChecker::wrapConversion(types::ExpressionNode* expression, TypeId target,
                                                            types::ImplicitConversionKind kind) {
        auto* cast = base::ASTNodeArena::getInstance().Alloc<types::ImplicitCastNode>(expression, expression->semanticInfo().Type, target.value, kind);
        cast->semanticInfo() = expression->semanticInfo();
        cast->semanticInfo().Type = target.value;
        cast->semanticInfo().LValue = false;
        cast->semanticInfo().Annotated = true;
        auto& metadata = cast->semanticInfo();
        auto arithmetic = TypeSystem.arithmetic(target);
        if (kind != types::ImplicitConversionKind::LValueToRValue && arithmetic) {
            std::optional<long double> number;
            if (metadata.HasFloatingConstant) number = metadata.FloatingConstant;
            else if (metadata.HasConstant) number = static_cast<long double>(metadata.Constant);
            if (target == TypeSystem.builtin(Builtin::Bool) && number) {
                metadata.Constant = *number != 0; metadata.HasConstant = true;
                metadata.HasFloatingConstant = false;
            } else if (arithmetic->floating && number) {
                metadata.FloatingConstant = target == TypeSystem.builtin(Builtin::Float) ?
                    static_cast<long double>(static_cast<float>(*number)) :
                    static_cast<long double>(static_cast<double>(*number));
                metadata.HasFloatingConstant = true; metadata.HasConstant = false;
            } else if (!arithmetic->floating) {
                if (metadata.HasFloatingConstant) {
                    auto number = metadata.FloatingConstant;
                    // Avoid undefined host casts at the integer limits. Runtime
                    // conversion remains explicit even when we cannot fold it.
                    metadata.HasConstant = std::isfinite(number) &&
                        number >= static_cast<long double>(std::numeric_limits<int64_t>::min()) &&
                        number < -static_cast<long double>(std::numeric_limits<int64_t>::min());
                    if (metadata.HasConstant) metadata.Constant = static_cast<int64_t>(number);
                    metadata.HasFloatingConstant = false;
                    if (!metadata.HasConstant) metadata.ConstantExpression = false;
                }
                if (metadata.HasConstant && arithmetic->bits < 64) {
                    auto bits = static_cast<uint64_t>(metadata.Constant) & ((uint64_t{1} << arithmetic->bits) - 1);
                    if (arithmetic->isSigned && (bits & (uint64_t{1} << (arithmetic->bits - 1))))
                        bits |= ~((uint64_t{1} << arithmetic->bits) - 1);
                    metadata.Constant = static_cast<int64_t>(bits);
                }
            }
            metadata.NullPointerConstant = !arithmetic->floating && metadata.ConstantExpression &&
                metadata.HasConstant && metadata.Constant == 0;
        } else if (isPointer(target)) {
            metadata.NullPointerConstant = false;
            metadata.HasConstant = metadata.HasFloatingConstant = false;
        }
        // Binding identity belongs to the operand, not the converted value.
        cast->semanticInfo().Symbol = 0;
        auto span = expression->getSourceSpan();
        if (span.Valid) cast->setSourceSpan(span.Line, span.Col);
        return cast;
    }

    types::ExpressionNode* ExpressionChecker::convertValue(types::ExpressionNode* expression, TypeId target) {
        using K = types::ImplicitConversionKind;
        if (!expression || expression->semanticInfo().Error || !expression->semanticInfo().Type) return expression;
        if (isBoundInstanceFunction(expression)) return expression;
        TypeId source{expression->semanticInfo().Type};
        auto kind = TypeSystem.get(TypeSystem.unConst(source)).kind;
        if (kind == TypeKind::Array) {
            expression = wrapConversion(expression, decay(source), K::ArrayToPointer);
        } else if (kind == TypeKind::Function) {
            expression = wrapConversion(expression, decay(source), K::FunctionToPointer);
        } else if (expression->semanticInfo().LValue) {
            expression = wrapConversion(expression, TypeSystem.unConst(source), K::LValueToRValue);
        }
        source = {expression->semanticInfo().Type};
        target = TypeSystem.unConst(target);
        if (!target.value || source == target) return expression;
        K conversion = K::NumericConversion;
        if (target == TypeSystem.builtin(Builtin::Bool)) conversion = K::BooleanConversion;
        else if (isPointer(target) && !isPointer(source)) conversion = K::NullToPointer;
        else if (isPointer(source) && isPointer(target)) {
            auto from = TypeSystem.get(TypeSystem.unConst(source)).element;
            auto to = TypeSystem.get(target).element;
            conversion = TypeSystem.unConst(from) == TypeSystem.unConst(to) ? K::QualificationConversion : K::PointerConversion;
        } else if (TypeSystem.unConst(source) == target) conversion = K::QualificationConversion;
        else if (isInteger(source) && promote(source) == target) conversion = K::IntegralPromotion;
        return wrapConversion(expression, target, conversion);
    }

    // Materialize only after successful typing. This walk is over collected
    // expression work, never another declaration/name-resolution traversal. Each
    // parent chooses value/address/unevaluated context for its children.
    types::ExpressionNode* ExpressionChecker::materializeImpl(types::ExpressionNode* expression, bool value, TypeId target) {
        if (!expression || expression->semanticInfo().Error) return expression;
        if (dynamic_cast<types::ImplicitCastNode*>(expression) || dynamic_cast<types::CompoundAssignmentNode*>(expression))
            return value ? convertValue(expression, target) : expression;
        if (auto it = ConversionTargets.find(expression); it != ConversionTargets.end() && !target.value) target = it->second;
        const TypeId own{expression->semanticInfo().Type};
        const auto boolean = TypeSystem.builtin(Builtin::Bool);
        if (auto* call = dynamic_cast<types::FunctionCallPostfixExpr*>(expression)) {
            TypeId signature{call->semanticInfo().CallSignature};
            if (signature.value) {
                const auto info = TypeSystem.get(signature);
                auto rewriteArgument = [&](types::ExpressionNode* argument, size_t i) {
                    TypeId expected;
                    if (i < info.parameters.size()) expected = info.parameters[i];
                    else if (argument && argument->semanticInfo().Type) {
                        auto type = TypeSystem.unConst(decay(TypeId{argument->semanticInfo().Type}));
                        // Default argument promotions apply only to ellipsis args.
                        expected = type == TypeSystem.builtin(Builtin::Float) ? TypeSystem.builtin(Builtin::Double) : promote(type);
                        if (!isArithmetic(type)) expected = {};
                    }
                    return materializeImpl(argument, true, expected);
                };
                // Bound instance methods retain their receiver and selected member
                // identity. Static/free functions decay; pointer fields are loaded.
                auto rewriteCallee = [&](types::ExpressionNode* callee) {
                    if (call->semanticInfo().Construction) return callee;
                    return materializeImpl(callee, !isBoundInstanceFunction(callee));
                };
                // The outer comma node is an argument-list container. A nested
                // comma expression is one argument and stays nested.
                auto* arguments = call->getArguments();
                if (auto* list = dynamic_cast<types::CommaExpressionNode*>(arguments)) {
                    size_t i = 0;
                    list->rewriteExpressions([&](auto* child) { return rewriteArgument(child, i++); });
                } else if (arguments) {
                    call->rewriteExpressions([&](auto* child) {
                        return child == arguments ? rewriteArgument(child, 0) :
                            rewriteCallee(child);
                    });
                    return value ? convertValue(expression, target) : expression;
                }
                // A constructor's type-name callee is not a function designator.
                if (!call->semanticInfo().Construction) call->rewriteExpressions([&](auto* child) {
                    return child == arguments ? child : rewriteCallee(child);
                });
            }
        } else if (auto* binary = dynamic_cast<types::BinaryExpressionNode*>(expression)) {
            auto op = binary->getBinaryOp();
            auto* left = binary->getLeft(); auto* right = binary->getRight();
            TypeId lt{left->semanticInfo().Type}, rt{right->semanticInfo().Type};
            lt = decay(lt); rt = decay(rt);
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
                    if (targetSpan.Valid) load->setSourceSpan(targetSpan.Line, targetSpan.Col);
                    auto underlying = Underlying(op);
                    bool shift = underlying == types::OP_LSHIFT || underlying == types::OP_RSHIFT;
                    TypeId computation = isPointer(lt) ? lt : shift ? promote(lt) : usualArithmetic(lt, rt, expression);
                    auto* lhs = convertValue(load, computation);
                    auto* rhs = materializeImpl(right, true, isPointer(lt) || shift ? promote(rt) : computation);
                    auto* operation = base::ASTNodeArena::getInstance().Alloc<types::BinaryExpressionNode>(underlying, lhs, rhs);
                    operation->semanticInfo() = expression->semanticInfo();
                    operation->semanticInfo().Type = computation.value;
                    operation->semanticInfo().LValue = false;
                    auto span = expression->getSourceSpan();
                    if (span.Valid) operation->setSourceSpan(span.Line, span.Col);
                    auto* assigned = convertValue(operation, TypeSystem.unConst(lt));
                    auto* replacement = base::ASTNodeArena::getInstance().Alloc<types::CompoundAssignmentNode>(location, assigned);
                    replacement->semanticInfo() = expression->semanticInfo();
                    if (span.Valid) replacement->setSourceSpan(span.Line, span.Col);
                    expression = replacement;
                }
            } else {
                TypeId leftTarget, rightTarget;
                if (op == types::OP_LOGICAL_AND || op == types::OP_LOGICAL_OR) leftTarget = rightTarget = boolean;
                else if (op == types::OP_LSHIFT || op == types::OP_RSHIFT) { leftTarget = promote(lt); rightTarget = promote(rt); }
                else if (isArithmetic(lt) && isArithmetic(rt) && op != types::OP_COMMA) {
                    leftTarget = rightTarget = usualArithmetic(lt, rt, expression);
                } else if (Comparison(op)) {
                    if (isPointer(lt) && !isPointer(rt)) rightTarget = lt;
                    else if (isPointer(rt) && !isPointer(lt)) leftTarget = rt;
                    else if (isPointer(lt) && isPointer(rt)) {
                        auto lp = TypeSystem.get(lt).element, rp = TypeSystem.get(rt).element;
                        auto element = TypeSystem.isVoid(lp) ? lp : rp;
                        element = TypeSystem.constant(element, TypeSystem.isConst(lp) || TypeSystem.isConst(rp));
                        leftTarget = rightTarget = TypeSystem.pointer(element);
                    }
                } else if (isPointer(lt) && isInteger(rt)) rightTarget = promote(rt);
                else if (isPointer(rt) && isInteger(lt)) leftTarget = promote(lt);
                binary->rewriteExpressions([&](auto* child) {
                    return materializeImpl(child, true, child == left ? leftTarget : rightTarget);
                });
            }
        } else if (auto* unary = dynamic_cast<types::FactorNode*>(expression)) {
            auto op = unary->getUnaryOp();
            bool reads = op != types::ASTUnaryOp::OP_ADDR_OF && op != types::ASTUnaryOp::OP_INC && op != types::ASTUnaryOp::OP_DEC;
            TypeId expected;
            if (op == types::ASTUnaryOp::OP_LOGICAL_NOT) expected = boolean;
            if (op == types::ASTUnaryOp::OP_TILDE || op == types::ASTUnaryOp::OP_UNARY_PLUS || op == types::ASTUnaryOp::OP_UNARY_MINUS)
                expected = own;
            unary->rewriteExpressions([&](auto* child) { return materializeImpl(child, reads, expected); });
        } else if (auto* index = dynamic_cast<types::IndexPostfixExpr*>(expression)) {
            auto* offset = index->getIndex();
            TypeId offsetType{offset->semanticInfo().Type};
            index->rewriteExpressions([&](auto* child) {
                return materializeImpl(child, true, child == offset ? promote(offsetType) : TypeId{});
            });
        } else if (auto* ternary = dynamic_cast<types::TernaryOpNode*>(expression)) {
            auto* condition = ternary->getCondition();
            ternary->rewriteExpressions([&](auto* child) { return materializeImpl(child, true, child == condition ? boolean : own); });
        } else if (auto* initializer = dynamic_cast<types::InitializerNode*>(expression)) {
            initializer->rewriteExpressions([&](auto* child) { return materializeImpl(child, false, target); });
            if (target.value) initializer->semanticInfo().Type = TypeSystem.unConst(target).value;
            return expression; // brace/syntax containers never become runtime casts
        } else if (auto* comma = dynamic_cast<types::CommaExpressionNode*>(expression)) {
            // A single child is also the parser's parentheses/expression wrapper.
            // Leave its result conversion outside so sizeof/address-of keep it.
            const auto count = comma->getExpressions().size();
            comma->rewriteExpressions([&](auto* child) { return materializeImpl(child, count != 1); });
        } else if (dynamic_cast<types::SizeofNode*>(expression)) {
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, false); });
        } else if (dynamic_cast<types::UnaryPostfixExpr*>(expression) || dynamic_cast<types::MemberPostfixExpr*>(expression)) {
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, false); });
        } else {
            // Index, arrow and explicit cast operands are value uses. The explicit
            // cast itself remains distinct and performs its written conversion.
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, true); });
        }
        // Initializer destinations also request value conversion when their parent
        // is a syntax container rather than an expression operator.
        return value || target.value ? convertValue(expression, target) : expression;
    }
}
