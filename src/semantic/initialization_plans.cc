#include "execution_plans.hh"
#include "expressions.hh"
#include <algorithm>

namespace baka {
    namespace semantic {
        namespace {
            const types::ExpressionNode* UnwrapInitializer(const types::ExpressionNode* expression) {
                for (;;) {
                    if (auto* list = dynamic_cast<const types::InitializerNode*>(expression);
                        list && !list->isBraced() && list->expressions().size() == 1) {
                        expression = list->expressions().front();
                    } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression);
                               comma && comma->getExpressions().size() == 1) {
                        expression = comma->getExpressions().front();
                    } else {
                        return expression;
                    }
                }
            }

            std::optional<std::string> StringInitializer(const types::ExpressionNode* expression) {
                auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(UnwrapInitializer(expression));
                if (!primary) {
                    return {};
                }
                auto* constant = std::get_if<types::ConstantNode*>(&primary->getExpression());
                if (!constant) {
                    return {};
                }
                if (auto* text = dynamic_cast<const types::ConstantTNode<base::SLString>*>(*constant)) {
                    return std::string(text->getValue());
                }
                if (auto* text = dynamic_cast<const types::ConstantTNode<std::string>*>(*constant)) {
                    return text->getValue();
                }
                return {};
            }
        }

        InitializationPlan& ExecutionPlans::buildInitialization(TypeId target, const types::ExpressionNode* initializer,
                                                                bool zero) {
            target = TypesView.unConst(target);
            const InitKey key{target.value, initializer, zero};
            auto found = Initializers.find(key);
            if (found != Initializers.end()) {
                return found->second;
            }
            auto& plan = Initializers[key];
            if (!initializer && !zero) {
                if (lifetime(target).construction) {
                    appendInitialization(plan, target, 0, nullptr, InitializationKind::Construct);
                }
            } else {
                initializeObject(plan, target, 0, initializer);
            }
            return plan;
        }

        void ExecutionPlans::appendInitialization(InitializationPlan& plan, TypeId type, uint64_t offset,
                                                  const types::ExpressionNode* expression, InitializationKind kind) {
            InitializationStep step;
            step.offset = offset;
            step.type = TypesView.unConst(type);
            step.expression = expression;
            step.kind = kind;
            plan.steps.push_back(std::move(step));
        }

        void ExecutionPlans::consumeInitializer(InitializationPlan& plan, TypeId type, uint64_t offset,
                                                const std::vector<types::ExpressionNode*>& expressions,
                                                size_t& position) {
            if (position == expressions.size()) {
                initializeObject(plan, type, offset, nullptr);
                return;
            }
            auto* expression = UnwrapInitializer(expressions[position]);
            const auto kind = TypesView.get(TypesView.unConst(type)).kind;
            if ((TypesView.isAggregate(type) && expression &&
                 TypesView.isAggregate({expression->semanticInfo().Type})) ||
                dynamic_cast<const types::InitializerNode*>(expression) ||
                (kind != TypeKind::Array && !TypesView.isAggregate(type)) || StringInitializer(expression)) {
                ++position;
                initializeObject(plan, type, offset, expression);
                return;
            }
            if (kind == TypeKind::Array) {
                initializeElements(plan, type, offset, expressions, position);
            } else {
                for (const auto& child : lifetime(type).children) {
                    consumeInitializer(plan, child.type, offset + child.offset, expressions, position);
                }
            }
        }

        void ExecutionPlans::initializeElements(InitializationPlan& plan, TypeId type, uint64_t offset,
                                                const std::vector<types::ExpressionNode*>& expressions,
                                                size_t& position) {
            const auto& info = TypesView.get(TypesView.unConst(type));
            const auto size = TypesView.layout(info.element).layout.size;
            uint64_t count = 0;
            while (count < info.bound.count && position < expressions.size()) {
                consumeInitializer(plan, info.element, offset + count * size, expressions, position);
                ++count;
            }
            if (count < info.bound.count) {
                // Keep the omitted suffix as one range, including brace-elided arrays.
                auto kind = lifetime(type).construction ? InitializationKind::Construct : InitializationKind::Zero;
                appendInitialization(plan, type, offset + count * size, nullptr, kind);
                plan.steps.back().byteCount = (info.bound.count - count) * size;
                plan.steps.back().elements = info.bound.count - count;
            }
        }

        void ExecutionPlans::initializeMembers(InitializationPlan& plan, TypeId type, uint64_t offset,
                                               const std::vector<types::ExpressionNode*>& expressions,
                                               size_t& position) {
            for (const auto& child : lifetime(type).children) {
                if (position < expressions.size()) {
                    consumeInitializer(plan, child.type, offset + child.offset, expressions, position);
                    continue;
                }
                const auto start = plan.steps.size();
                initializeObject(plan, child.type, offset + child.offset, child.initializer);
                if (child.initializer) {
                    for (size_t i = start; i < plan.steps.size(); ++i) {
                        if (!plan.steps[i].receiver.value) {
                            plan.steps[i].receiver = type;
                            plan.steps[i].receiverOffset = offset;
                        }
                    }
                }
            }
        }

        void ExecutionPlans::initializeObject(InitializationPlan& plan, TypeId type, uint64_t offset,
                                              const types::ExpressionNode* expression) {
            type = TypesView.unConst(type);
            expression = UnwrapInitializer(expression);
            const auto& info = TypesView.get(type);
            if (!expression) {
                auto kind = lifetime(type).construction ? InitializationKind::Construct : InitializationKind::Zero;
                appendInitialization(plan, type, offset, nullptr, kind);
                return;
            }
            if (TypesView.isAggregate(type) && TypesView.isAggregate({expression->semanticInfo().Type}) &&
                !dynamic_cast<const types::InitializerNode*>(expression)) {
                appendInitialization(plan, type, offset, expression, InitializationKind::Value);
                return;
            }
            auto* list = dynamic_cast<const types::InitializerNode*>(expression);
            if (info.kind != TypeKind::Array && !TypesView.isAggregate(type)) {
                if (list) {
                    if (list->expressions().size() > 1) {
                        plan.failure = "excess scalar initializer";
                    }
                    expression = list->expressions().empty() ? nullptr : list->expressions().front();
                }
                auto kind = expression ? InitializationKind::Value : InitializationKind::Zero;
                appendInitialization(plan, type, offset, expression, kind);
                return;
            }
            if (TypesView.isAggregate(type) && list &&
                std::any_of(info.members.begin(), info.members.end(),
                            [](const Member& member) { return member.isConstructor; })) {
                if (!list->expressions().empty()) {
                    plan.failure = "braced class construction is unsupported; use Type(arguments)";
                }
                appendInitialization(plan, type, offset, nullptr, InitializationKind::Construct);
                return;
            }
            auto text =
                StringInitializer(list && list->expressions().size() == 1 ? list->expressions().front() : expression);
            if (info.kind == TypeKind::Array && text) {
                appendInitialization(plan, type, offset, expression, InitializationKind::String);
                plan.steps.back().bytes = *text;
                return;
            }
            if (!list) {
                plan.failure = "aggregate initializer lacks a checked value or initializer list";
                return;
            }
            if (list->expressions().empty() && info.kind == TypeKind::Array) {
                initializeObject(plan, type, offset, nullptr);
                return;
            }
            size_t position = 0;
            if (info.kind == TypeKind::Array) {
                initializeElements(plan, type, offset, list->expressions(), position);
            } else {
                initializeMembers(plan, type, offset, list->expressions(), position);
            }
            if (position != list->expressions().size()) {
                plan.failure = "aggregate initializer was not consumed completely";
            }
        }
    }
}
