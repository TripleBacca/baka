#include "expressions.hh"
#include "memory/custom_arenas.hh"
#include <cmath>
#include <limits>

namespace baka {
    namespace semantic {
        void ExpressionChecker::planConversion(const types::ExpressionNode* expression, TypeId target) {
            if (!Annotating || !expression) {
                return;
            }
            // InitializerNode is syntax, never a runtime operand. Push a scalar
            // destination down through transparent/single-element brace wrappers.
            while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                if (list->expressions().size() != 1) {
                    return;
                }
                expression = list->expressions().front();
            }
            ConversionTargets[expression] = TypeSystem.unConst(target);
        }

        bool ExpressionChecker::isBoundInstanceFunction(const types::ExpressionNode* expression) const {
            while (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
                if (comma->getExpressions().size() != 1) {
                    break;
                }
                expression = comma->getExpressions().front();
            }
            if (!expression) {
                return false;
            }
            const auto& metadata = expression->semanticInfo();
            if (!metadata.MemberOwner || !metadata.MemberIndex || !metadata.Type ||
                TypeSystem.get(TypeSystem.unConst({metadata.Type})).kind != TypeKind::Function) {
                return false;
            }
            const auto& members = TypeSystem.get({metadata.MemberOwner}).members;
            if (metadata.MemberIndex > members.size()) {
                return false;
            }
            const auto& member = members[metadata.MemberIndex - 1];
            return !member.isStatic && TypeSystem.get(TypeSystem.unConst(member.type)).kind == TypeKind::Function;
        }

        types::ExpressionNode* ExpressionChecker::wrapConversion(types::ExpressionNode* expression, TypeId target,
                                                                 types::ImplicitConversionKind kind) {
            auto* cast = base::ASTNodeArena::getInstance().Alloc<types::ImplicitCastNode>(
                expression, expression->semanticInfo().Type, target.value, kind);
            cast->semanticInfo() = expression->semanticInfo();
            cast->semanticInfo().Type = target.value;
            cast->semanticInfo().LValue = false;
            cast->semanticInfo().Annotated = true;
            auto& metadata = cast->semanticInfo();
            ExprResult source;
            source.type = {expression->semanticInfo().Type};
            source.constantExpression = metadata.ConstantExpression;
            if (metadata.HasConstant) {
                source.constant = {ConstantState::Value, metadata.Constant};
            }
            if (metadata.HasFloatingConstant) {
                source.floatingConstant = metadata.FloatingConstant;
            }
            auto converted = convertConstant(source, target);
            metadata.ConstantExpression = converted.constantExpression;
            metadata.HasConstant = converted.constant.state == ConstantState::Value;
            metadata.Constant = converted.constant.value;
            metadata.HasFloatingConstant = converted.floatingConstant.has_value();
            metadata.FloatingConstant = converted.floatingConstant.value_or(0);
            metadata.NullPointerConstant = converted.nullPointerConstant;
            // Binding identity belongs to the operand, not the converted value.
            cast->semanticInfo().Symbol = 0;
            cast->semanticInfo().CallSignature = cast->semanticInfo().MemberOwner = cast->semanticInfo().MemberIndex =
                0;
            cast->semanticInfo().Construction = false;
            auto span = expression->getSourceSpan();
            if (span.Valid) {
                cast->setSourceSpan(span.Line, span.Col);
            }
            return cast;
        }

        types::ExpressionNode* ExpressionChecker::convertValue(types::ExpressionNode* expression, TypeId target) {
            using K = types::ImplicitConversionKind;
            const auto* original = expression;
            if (!expression || expression->semanticInfo().Error || !expression->semanticInfo().Type) {
                return expression;
            }
            if (isBoundInstanceFunction(expression)) {
                return expression;
            }
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
            if (!target.value || source == target) {
                return expression;
            }
            if (TypeSystem.isAggregate(target)) {
                auto selection = ConstructorConversions.find({original, target.value});
                if (selection == ConstructorConversions.end()) {
                    SymbolTable.diagnose("conversion", "aggregate argument lacks a checked constructor conversion",
                                         expression);
                    return expression;
                }
                const auto& selected = selection->second;
                auto parameter = TypeSystem.get(selected.signature).parameters.front();
                expression = convertValue(expression, parameter);
                auto* converted = wrapConversion(expression, target, K::ConstructorConversion);
                auto& info = converted->semanticInfo();
                info.Construction = true;
                info.CallSignature = selected.signature.value;
                info.MemberOwner = selected.owner.value;
                info.MemberIndex = selected.member;
                info.ConstantExpression = info.HasConstant = info.HasFloatingConstant = info.NullPointerConstant =
                    false;
                return converted;
            }
            K conversion = K::NumericConversion;
            if (target == TypeSystem.builtin(Builtin::Bool)) {
                conversion = K::BooleanConversion;
            } else if (isPointer(target) && !isPointer(source)) {
                conversion = K::NullToPointer;
            } else if (isPointer(source) && isPointer(target)) {
                auto from = TypeSystem.get(TypeSystem.unConst(source)).element;
                auto to = TypeSystem.get(target).element;
                conversion = TypeSystem.unConst(from) == TypeSystem.unConst(to) ? K::QualificationConversion
                                                                                : K::PointerConversion;
            } else if (TypeSystem.unConst(source) == target) {
                conversion = K::QualificationConversion;
            } else if (isInteger(source) && promote(source) == target) {
                conversion = K::IntegralPromotion;
            }
            return wrapConversion(expression, target, conversion);
        }

        // Materialize only after successful typing. This walk is over collected
        // expression work, never another declaration/name-resolution traversal. Each
        // parent chooses value/address/unevaluated context for its children.
        types::ExpressionNode* ExpressionChecker::materializeImpl(types::ExpressionNode* expression, bool value,
                                                                  TypeId target) {
            if (!expression || expression->semanticInfo().Error) {
                return expression;
            }
            if (dynamic_cast<types::ImplicitCastNode*>(expression) ||
                dynamic_cast<types::CompoundAssignmentNode*>(expression)) {
                return value ? convertValue(expression, target) : expression;
            }
            if (auto it = ConversionTargets.find(expression); it != ConversionTargets.end() && !target.value) {
                target = it->second;
            }
            const TypeId own{expression->semanticInfo().Type};
            const auto boolean = TypeSystem.builtin(Builtin::Bool);
            if (auto* call = dynamic_cast<types::FunctionCallPostfixExpr*>(expression)) {
                materializeCall(call);
            } else if (auto* binary = dynamic_cast<types::BinaryExpressionNode*>(expression)) {
                expression = materializeBinary(binary);
            } else if (auto* unary = dynamic_cast<types::FactorNode*>(expression)) {
                auto op = unary->getUnaryOp();
                bool reads = op != types::ASTUnaryOp::OP_ADDR_OF && op != types::ASTUnaryOp::OP_INC &&
                             op != types::ASTUnaryOp::OP_DEC;
                TypeId expected;
                if (op == types::ASTUnaryOp::OP_LOGICAL_NOT) {
                    expected = boolean;
                }
                if (op == types::ASTUnaryOp::OP_TILDE || op == types::ASTUnaryOp::OP_UNARY_PLUS ||
                    op == types::ASTUnaryOp::OP_UNARY_MINUS) {
                    expected = own;
                }
                unary->rewriteExpressions([&](auto* child) { return materializeImpl(child, reads, expected); });
            } else if (auto* index = dynamic_cast<types::IndexPostfixExpr*>(expression)) {
                auto* offset = index->getIndex();
                TypeId offsetType{offset->semanticInfo().Type};
                index->rewriteExpressions([&](auto* child) {
                    return materializeImpl(child, true, child == offset ? promote(offsetType) : TypeId{});
                });
            } else if (auto* ternary = dynamic_cast<types::TernaryOpNode*>(expression)) {
                auto* condition = ternary->getCondition();
                ternary->rewriteExpressions([&](auto* child) {
                    const bool read = child == condition || !expression->semanticInfo().LValue;
                    const auto destination = child == condition ? boolean : read ? own : TypeId{};
                    return materializeImpl(child, read, destination);
                });
            } else if (auto* initializer = dynamic_cast<types::InitializerNode*>(expression)) {
                initializer->rewriteExpressions([&](auto* child) { return materializeImpl(child, false, target); });
                if (target.value) {
                    initializer->semanticInfo().Type = TypeSystem.unConst(target).value;
                }
                return expression; // brace/syntax containers never become runtime casts
            } else if (auto* comma = dynamic_cast<types::CommaExpressionNode*>(expression)) {
                // A single child is also the parser's parentheses/expression wrapper.
                // Leave its result conversion outside so sizeof/address-of keep it.
                const auto count = comma->getExpressions().size();
                comma->rewriteExpressions([&](auto* child) { return materializeImpl(child, count != 1); });
            } else if (dynamic_cast<types::SizeofNode*>(expression)) {
                expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, false); });
            } else if (dynamic_cast<types::UnaryPostfixExpr*>(expression) ||
                       dynamic_cast<types::MemberPostfixExpr*>(expression)) {
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
}
