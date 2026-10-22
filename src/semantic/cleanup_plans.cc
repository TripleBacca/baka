#include "execution_plans.hh"
#include "expressions.hh"
#include "semantic_analyzer.hh"
#include <algorithm>

namespace baka {
    namespace semantic {
        void ExecutionPlans::appendObjects(const types::DeclarationList* declarations,
                                           std::vector<SymbolId>& active) const {
            for (auto* declaration : declarations->getDeclarations()) {
                SymbolId id{declaration->semanticInfo().Symbol};
                const auto& symbol = SymbolsView.get(id);
                const auto& plan = lifetime(symbol.type);
                if (!symbol.isStatic && (plan.construction || plan.destruction)) {
                    active.push_back(id);
                }
            }
        }

        void ExecutionPlans::addBody(const types::StatementNode* node, std::vector<SymbolId> active) {
            if (!node) {
                return;
            }
            if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(node)) {
                for (const auto& child : compound->getStatements()) {
                    std::visit(
                        [&](auto* item) {
                            if (auto* statement = dynamic_cast<const types::StatementNode*>(item)) {
                                addBody(statement, active);
                            }
                            if (auto* list = dynamic_cast<const types::DeclarationList*>(item)) {
                                appendObjects(list, active);
                            }
                        },
                        child);
                }
            } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(node)) {
                Labels[label->semanticInfo().Target] = {active};
                addBody(label->getStatement(), active);
            } else if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(node)) {
                Cases[label] = {active};
                addBody(label->getStatement(), active);
            } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(node)) {
                Cases[label] = {active};
                addBody(label->getStatement(), active);
            } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(node)) {
                addBody(conditional->getIf()->getBody(), active);
                for (auto* arm : conditional->getElseIfs()) {
                    addBody(arm->getBody(), active);
                }
                if (conditional->getElse()) {
                    addBody(conditional->getElse()->getBody(), active);
                }
            } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(node)) {
                BreakTargets[loop->semanticInfo().Target] = active;
                std::visit(
                    [&](auto* initializer) {
                        if (auto* list = dynamic_cast<const types::DeclarationList*>(initializer)) {
                            appendObjects(list, active);
                        }
                    },
                    loop->getInitializer());
                ContinueTargets[loop->semanticInfo().Target] = active;
                addBody(loop->getBody(), active);
            } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(node)) {
                BreakTargets[loop->semanticInfo().Target] = ContinueTargets[loop->semanticInfo().Target] = active;
                addBody(loop->getBody(), active);
            } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(node)) {
                BreakTargets[loop->semanticInfo().Target] = ContinueTargets[loop->semanticInfo().Target] = active;
                addBody(loop->getBody(), active);
            } else if (auto* selection = dynamic_cast<const types::SwitchBlockStatementNode*>(node)) {
                BreakTargets[selection->semanticInfo().Target] = active;
                addBody(selection->getBody(), active);
            } else if (dynamic_cast<const types::ReturnStatementNode*>(node)) {
                exit(node, active, {});
            } else if (dynamic_cast<const types::GotoStatementNode*>(node)) {
                PendingGotos.push_back({node, active});
            } else if (dynamic_cast<const types::BreakStatementNode*>(node)) {
                if (node->semanticInfo().Target) {
                    exit(node, active, BreakTargets.at(node->semanticInfo().Target));
                }
            } else if (dynamic_cast<const types::ContinueStatementNode*>(node)) {
                if (node->semanticInfo().Target) {
                    exit(node, active, ContinueTargets.at(node->semanticInfo().Target));
                }
            } else if (!dynamic_cast<const types::EmptyStatementNode*>(node) &&
                       !dynamic_cast<const types::ExpressionStatementNode*>(node)) {
                Errors.push_back({node, "unsupported statement in cleanup planning"});
            }
        }

        void ExecutionPlans::exit(const types::StatementNode* source, const std::vector<SymbolId>& active,
                                  const std::vector<SymbolId>& retained) {
            auto& result = Exits[source];
            for (auto it = active.rbegin(); it != active.rend(); ++it) {
                if (std::find(retained.begin(), retained.end(), *it) == retained.end()) {
                    result.push_back(*it);
                }
            }
        }

        void ExecutionPlans::finish() {
            for (const auto& [node, active] : PendingGotos) {
                auto target = Labels.find(node->semanticInfo().Target);
                if (target != Labels.end()) {
                    exit(node, active, target->second.active);
                }
            }
            PendingGotos.clear();
        }
    }
}
