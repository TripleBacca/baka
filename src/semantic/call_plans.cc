#include "execution_plans.hh"
#include "expressions.hh"
#include <algorithm>

namespace baka {
    namespace semantic {
        void ExecutionPlans::addExpression(const types::ExpressionNode* expression) {
            if (!expression) {
                return;
            }
            expression->visitExpressions([&](const auto* child) { addExpression(child); });
            const auto* callNode = dynamic_cast<const types::FunctionCallPostfixExpr*>(expression);
            if (callNode) {
                auto kind = types::heapCallKind(callNode);
                if (kind != types::HeapCallKind::None) {
                    if (kind == types::HeapCallKind::Allocate && !Calls.count(expression)) {
                        CallPlan plan;
                        plan.owner = {expression->semanticInfo().MemberOwner};
                        plan.member = expression->semanticInfo().MemberIndex;
                        plan.signature = {expression->semanticInfo().CallSignature};
                        auto* arguments = dynamic_cast<const types::CommaExpressionNode*>(callNode->getArguments());
                        if (arguments) {
                            auto& values = arguments->getExpressions();
                            plan.arguments.assign(values.begin() + 1, values.end());
                        }
                        if (plan.member) {
                            RequiredConstructors.insert({plan.owner.value, plan.member});
                            requireMembers(plan.owner, expression);
                            const auto* constructor = static_cast<const types::ConstructorNode*>(
                                TypesView.get(plan.owner).members.at(plan.member - 1).source);
                            const auto count = TypesView.get(plan.signature).parameters.size();
                            while (plan.arguments.size() < count) {
                                auto index = plan.arguments.size();
                                auto* argument = constructor->getArgs()->getParameters().at(index)
                                                     ->getDeclaration()->getInitializer();
                                if (!argument) {
                                    Errors.push_back({expression, "selected constructor lacks a default argument"});
                                    break;
                                }
                                plan.arguments.push_back(argument);
                            }
                        } else {
                            auto pointer = TypesView.unConst({expression->semanticInfo().Type});
                            auto target = TypesView.unConst(TypesView.get(pointer).element);
                            while (TypesView.get(target).kind == TypeKind::Array) {
                                target = TypesView.unConst(TypesView.get(target).element);
                            }
                            requireDefault(target, expression);
                        }
                        Calls.emplace(expression, std::move(plan));
                        for (auto* argument : Calls.at(expression).arguments) {
                            addExpression(argument);
                        }
                    }
                    return;
                }
            }
            const auto* conversion = dynamic_cast<const types::ImplicitCastNode*>(expression);
            if (!callNode && (!conversion || conversion->getConversionKind() !=
                                                 types::ImplicitConversionKind::ConstructorConversion)) {
                return;
            }
            if (Calls.count(expression)) {
                return;
            }
            const auto& metadata = expression->semanticInfo();
            CallPlan plan{{metadata.CallSignature}, {metadata.MemberOwner}, {metadata.Symbol},
                          metadata.MemberIndex,     metadata.Construction,  {}};
            if (conversion) {
                plan.arguments.push_back(conversion->getOperand());
            } else if (auto* arguments = dynamic_cast<const types::CommaExpressionNode*>(callNode->getArguments())) {
                plan.arguments.assign(arguments->getExpressions().begin(), arguments->getExpressions().end());
            } else if (callNode->getArguments()) {
                plan.arguments.push_back(callNode->getArguments());
            }
            const types::ConstructorNode* constructor = nullptr;
            if (plan.construction) {
                if (!plan.member) {
                    requireDefault(plan.owner, expression);
                } else {
                    RequiredConstructors.insert({plan.owner.value, plan.member});
                    constructor = static_cast<const types::ConstructorNode*>(
                        TypesView.get(plan.owner).members.at(plan.member - 1).source);
                    requireMembers(plan.owner, expression);
                }
            }
            const auto count = plan.signature.value ? TypesView.get(plan.signature).parameters.size() : 0;
            while (plan.arguments.size() < count) {
                const auto index = plan.arguments.size();
                const types::ExpressionNode* argument = nullptr;
                if (constructor) {
                    argument = constructor->getArgs()->getParameters().at(index)->getDeclaration()->getInitializer();
                } else if (plan.symbol.value) {
                    const auto& defaults = SymbolsView.get(plan.symbol).defaultArguments;
                    if (index < defaults.size()) {
                        argument = static_cast<const types::ExpressionNode*>(defaults[index]);
                    }
                }
                if (!argument) {
                    Errors.push_back({expression, "selected call lacks a checked default argument"});
                    break;
                }
                plan.arguments.push_back(argument);
            }
            Calls.emplace(expression, std::move(plan));
            for (auto* argument : Calls.at(expression).arguments) {
                if (argument != expression) {
                    addExpression(argument);
                }
            }
        }

        const CallPlan& ExecutionPlans::call(const types::ExpressionNode* expression) const {
            return Calls.at(expression);
        }
    }
}
