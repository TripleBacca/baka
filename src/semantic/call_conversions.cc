#include "expressions.hh"

namespace baka {
    namespace semantic {
        types::ExpressionNode* ExpressionChecker::materializeArgument(types::ExpressionNode* argument, size_t index,
                                                                      TypeId signature) {
            const auto& info = TypeSystem.get(signature);
            TypeId expected;
            if (index < info.parameters.size()) {
                expected = info.parameters[index];
            } else if (argument && argument->semanticInfo().Type) {
                auto type = TypeSystem.unConst(decay(TypeId{argument->semanticInfo().Type}));
                // Default argument promotions apply only to ellipsis args.
                expected =
                    type == TypeSystem.builtin(Builtin::Float) ? TypeSystem.builtin(Builtin::Double) : promote(type);
                if (!isArithmetic(type)) {
                    expected = {};
                }
            }
            return materializeImpl(argument, true, expected);
        }

        types::ExpressionNode* ExpressionChecker::materializeCallee(types::FunctionCallPostfixExpr* call,
                                                                    types::ExpressionNode* callee) {
            // Bound methods keep their receiver; free/static functions decay normally.
            if (call->semanticInfo().Construction) {
                return callee;
            }
            return materializeImpl(callee, !isBoundInstanceFunction(callee));
        }

        void ExpressionChecker::materializeCall(types::FunctionCallPostfixExpr* call) {
            TypeId signature{call->semanticInfo().CallSignature};
            if (signature.value) {
                // The outer comma node is an argument-list container. A nested
                // comma expression is one argument and stays nested.
                auto* arguments = call->getArguments();
                if (auto* list = dynamic_cast<types::CommaExpressionNode*>(arguments)) {
                    size_t i = 0;
                    list->rewriteExpressions([&](auto* child) { return materializeArgument(child, i++, signature); });
                } else if (arguments) {
                    call->rewriteExpressions([&](auto* child) {
                        return child == arguments ? materializeArgument(child, 0, signature)
                                                  : materializeCallee(call, child);
                    });
                    return;
                }
                // A constructor's type-name callee is not a function designator.
                if (!call->semanticInfo().Construction) {
                    call->rewriteExpressions(
                        [&](auto* child) { return child == arguments ? child : materializeCallee(call, child); });
                }
            }
        }

    }
}
