#include "execution_plans.hh"
#include "expressions.hh"
#include <algorithm>

namespace baka {
    namespace semantic {
        LifetimePlan& ExecutionPlans::buildLifetime(TypeId id, const ExpressionChecker& checker) {
            id = TypesView.unConst(id);
            if (auto found = Lifetimes.find(id.value); found != Lifetimes.end()) {
                return found->second;
            }
            auto& plan = Lifetimes[id.value];
            plan.defaultConstructor.owner = id;
            const auto& type = TypesView.get(id);
            if (type.kind == TypeKind::Array) {
                const auto child = buildLifetime(type.element, checker);
                plan.construction = child.construction;
                plan.destruction = child.destruction;
                plan.defaultConstructor.available = child.defaultConstructor.available;
                plan.defaultConstructor.failure = child.defaultConstructor.failure;
                return plan;
            }
            if (!TypesView.isAggregate(id) || type.completion != Completion::Complete) {
                return plan;
            }
            plan.defaultConstructor = checker.defaultConstructor(id);
            for (const auto& member : type.members) {
                plan.construction |= member.isConstructor;
                plan.destruction |= member.isDestructor;
            }
            for (auto child : TypesView.subobjects(id)) {
                const auto offset = child.memberIndex ? type.objectLayout->offsets.at(*child.memberIndex)
                                                      : type.objectLayout->baseOffset;
                const auto* initializer = child.memberIndex ? type.members[*child.memberIndex].initializer : nullptr;
                plan.children.push_back({child.type, *offset, initializer});
                const auto& traits = buildLifetime(child.type, checker);
                plan.construction |= traits.construction || initializer;
                plan.destruction |= traits.destruction;
                if (!initializer && !traits.defaultConstructor.available && plan.defaultConstructor.available) {
                    plan.defaultConstructor.available = false;
                    plan.defaultConstructor.failure = traits.defaultConstructor.failure;
                }
                if (type.kind == TypeKind::Union) {
                    break;
                }
            }
            return plan;
        }

        void ExecutionPlans::prepare(const ExpressionChecker& checker) {
            for (uint32_t i = 1; i <= TypesView.typeCount(); ++i) {
                buildLifetime({i}, checker);
            }
            // Index each object's function once, rather than scanning all symbols
            // and walking ancestry again for every function emitted by lowering.
            for (uint32_t i = 1; i <= SymbolsView.symbolCount(); ++i) {
                const auto& symbol = SymbolsView.get({i});
                if (symbol.isStatic || (symbol.kind != SymbolKind::Object && symbol.kind != SymbolKind::Parameter)) {
                    continue;
                }
                auto scope = symbol.scope;
                while (scope.value && SymbolsView.scope(scope).kind != ScopeKind::Function) {
                    if (SymbolsView.scope(scope).kind == ScopeKind::Prototype) {
                        scope = {};
                        break;
                    }
                    scope = SymbolsView.scope(scope).parent;
                }
                if (scope.value) {
                    Locals[scope.value].push_back({i});
                }
            }
            for (uint32_t i = 1; i <= TypesView.typeCount(); ++i) {
                auto id = TypesView.unConst({i});
                if (!TypesView.storedLayout(id) && !TypesView.layout(id)) {
                    continue;
                }
                buildInitialization(id, nullptr, false);
                buildInitialization(id, nullptr, true);
                for (const auto& child : lifetime(id).children) {
                    buildInitialization(child.type, child.initializer, false);
                }
            }
        }

        void ExecutionPlans::requireMembers(TypeId type, const types::ASTNode* source) {
            for (const auto& child : lifetime(type).children) {
                auto& plan = buildInitialization(child.type, child.initializer, false);
                validate(plan, source);
            }
        }

        void ExecutionPlans::requireDefault(TypeId type, const types::ASTNode* source) {
            type = TypesView.unConst(type);
            const auto& info = TypesView.get(type);
            if (info.kind == TypeKind::Array) {
                requireDefault(info.element, source);
                return;
            }
            const auto& plan = lifetime(type);
            if (!plan.defaultConstructor.available) {
                Errors.push_back({source, plan.defaultConstructor.failure});
                return;
            }
            if (!plan.construction || !RequiredDefaults.insert(type.value).second) {
                return;
            }
            if (plan.defaultConstructor.member) {
                RequiredConstructors.insert({type.value, plan.defaultConstructor.member});
            }
            for (auto* argument : plan.defaultConstructor.arguments) {
                addExpression(argument);
            }
            requireMembers(type, source);
        }

        void ExecutionPlans::validate(const InitializationPlan& plan, const types::ASTNode* source) {
            if (!plan.failure.empty()) {
                Errors.push_back({source, plan.failure});
                return;
            }
            for (const auto& step : plan.steps) {
                if (step.kind == InitializationKind::Construct) {
                    requireDefault(step.type, source);
                }
                if (step.expression) {
                    addExpression(step.expression);
                }
            }
        }

        void ExecutionPlans::addInitialization(TypeId type, const types::ExpressionNode* expression,
                                               const types::ASTNode* source) {
            const auto& plan = buildInitialization(type, expression, false);
            validate(plan, source);
        }

        const LifetimePlan& ExecutionPlans::lifetime(TypeId type) const {
            return Lifetimes.at(TypesView.unConst(type).value);
        }

        const InitializationPlan& ExecutionPlans::initialization(TypeId type, const types::ExpressionNode* expression,
                                                                 bool zero) const {
            return Initializers.at({TypesView.unConst(type).value, expression, zero});
        }

        const std::vector<SymbolId>& ExecutionPlans::locals(ScopeId function) const {
            static const std::vector<SymbolId> empty;
            auto it = Locals.find(function.value);
            return it == Locals.end() ? empty : it->second;
        }

        bool ExecutionPlans::requiresConstructor(TypeId type, uint32_t member) const {
            return member ? RequiredConstructors.count({TypesView.unConst(type).value, member})
                          : RequiredDefaults.count(TypesView.unConst(type).value);
        }
    }
}
