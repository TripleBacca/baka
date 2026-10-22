#include "lowering.hh"

namespace baka {
    namespace ir {
        Operand Lowerer::value(const types::ExpressionNode* expression) {
            if (!expression) {
                return {};
            }
            auto span = expression->getSourceSpan();
            if (span.Valid) {
                State.Source = {span.Line, span.Col};
            }
            const auto resultType = type(expression);
            if (auto* castNode = dynamic_cast<const types::ImplicitCastNode*>(expression)) {
                return implicitValue(castNode);
            }
            if (auto* compound = dynamic_cast<const types::CompoundAssignmentNode*>(expression)) {
                return compoundAssignment(compound, false);
            }
            if (dynamic_cast<const types::CompoundTargetNode*>(expression)) {
                return load(address(expression), resultType);
            }
            if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
                Operand result;
                for (auto* child : comma->getExpressions()) {
                    result = value(child);
                }
                return result;
            }
            if (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
                if (initializer->expressions().empty()) {
                    return integer(resultType, 0);
                }
                if (initializer->expressions().size() != 1) {
                    fail(expression, "aggregate initializer used as a runtime value");
                }
                return value(initializer->expressions().front());
            }
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                return primaryValue(primary);
            }
            if (auto* binaryNode = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
                return binaryValue(binaryNode);
            }
            if (auto* unary = dynamic_cast<const types::FactorNode*>(expression)) {
                return unaryValue(unary);
            }
            if (auto* postfix = dynamic_cast<const types::UnaryPostfixExpr*>(expression)) {
                return increment(postfix->getOperand(), postfix->getOp() == types::ASTUnaryOp::OP_DEC, true);
            }
            if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
                return conditionalValue(ternary);
            }
            if (auto* explicitCast = dynamic_cast<const types::CastFactorNode*>(expression)) {
                return castValue(explicitCast);
            }
            if (auto* size = dynamic_cast<const types::SizeofNode*>(expression)) {
                if (!size->semanticInfo().HasConstant) {
                    fail(size, "sizeof lacks a semantic constant size");
                }
                return integer(resultType, static_cast<uint64_t>(size->semanticInfo().Constant));
            }
            if (auto* callNode = dynamic_cast<const types::FunctionCallPostfixExpr*>(expression)) {
                return call(callNode);
            }
            if (dynamic_cast<const types::MemberPostfixExpr*>(expression) ||
                dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                return memberValue(expression);
            }
            if (dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
                return load(address(expression), resultType);
            }
            fail(expression, "unsupported expression node in IR lowering");
        }

        Operand Lowerer::implicitValue(const types::ImplicitCastNode* castNode) {
            const auto resultType = type(castNode);
            using C = types::ImplicitConversionKind;
            switch (castNode->getConversionKind()) {
            case C::ConstructorConversion:
                return constructionValue(castNode, Analyzer.executionPlans().call(castNode).arguments);
            case C::LValueToRValue:
                return load(address(castNode->getOperand()), resultType);
            case C::ArrayToPointer:
                return cast(address(castNode->getOperand()), resultType);
            case C::FunctionToPointer:
                return cast(value(castNode->getOperand()), resultType);
            default:
                return cast(value(castNode->getOperand()), resultType);
            }
        }

        Operand Lowerer::castValue(const types::CastFactorNode* explicitCast) {
            const auto resultType = type(explicitCast);
            auto operand = value(explicitCast->getExpression());
            auto result = cast(operand, resultType);
            if (Result.type(resultType).kind == TypeKind::Record) {
                if (auto location = destination(explicitCast)) {
                    store(location, result);
                    transfer(result);
                    State.ValueLocations[result.id] = location;
                } else if (needsDestruction(resultType) && State.ValueOwners[result.id].empty()) {
                    result = ownTemporary(result);
                }
            }
            return result;
        }
    }
}
