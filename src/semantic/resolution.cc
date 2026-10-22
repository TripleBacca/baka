#include "semantic_analyzer.hh"
#include <algorithm>
#include <type_traits>

namespace baka {
    namespace semantic {
        size_t SemanticAnalyzer::queue(types::ExpressionNode* expression, const types::ASTNode* source,
                                       ExpressionUse use, SymbolId symbol, TypeId expected, bool staticStorage) {
            auto context = Current;
            context.scope = SymbolTable.currentScope();
            Expressions.push_back({expression, source, context, use, symbol, expected, staticStorage, {}});
            return Expressions.size() - 1;
        }

        void SemanticAnalyzer::resolveTypeOperand(types::TypeNode* type, types::DeclarationIdentifierNode* declarator) {
            if (!type) {
                return;
            }
            auto resolved = ASTResolver.typeName(*type);
            if (declarator) {
                resolved = ASTResolver.declarator(resolved, declarator);
            }
            type->semanticInfo().Type = resolved.value;
            type->semanticInfo().Resolved = true;
        }

        void SemanticAnalyzer::resolveInitializer(types::ExpressionNode* expression) {
            if (auto* initializer = dynamic_cast<types::InitializerNode*>(expression)) {
                initializer->semanticInfo().Resolved = true;
                initializer->semanticInfo().Scope = SymbolTable.currentScope().value;
                for (auto* child : initializer->expressions()) {
                    resolveInitializer(child);
                }
            } else {
                resolveExpression(expression);
            }
        }

        void SemanticAnalyzer::resolveExpression(types::ExpressionNode* expression) {
            if (!expression || expression->semanticInfo().Resolved) {
                return;
            }
            auto& metadata = expression->semanticInfo();
            metadata.Resolved = true;
            metadata.Scope = SymbolTable.currentScope().value;
            metadata.DeclarationPoint = SymbolTable.declarationPoint();
            if (auto* primary = dynamic_cast<types::PrimaryExpressionNode*>(expression)) {
                auto payload = primary->getExpression();
                if (auto* name = std::get_if<types::IdentifierNode*>(&payload); name && *name) {
                    auto* identifier = *name;
                    auto found = SymbolTable.lookup(std::string(identifier->GetName()));
                    // Local names shadow members; members shadow the surrounding global
                    // scope in an instance/static method. Aggregate scopes persist.
                    if (Current.owner.value &&
                        (found.empty() ||
                         SymbolTable.scope(SymbolTable.get(found.front()).scope).kind == ScopeKind::Global)) {
                        auto member = TypeSystem.lookupMember(Current.owner, std::string(identifier->GetName()));
                        if (!member.indices.empty()) {
                            found.clear();
                            for (auto index : member.indices) {
                                const auto& entry = TypeSystem.get(member.owner).members[index];
                                if (entry.symbol.value) {
                                    found.push_back(entry.symbol);
                                }
                            }
                        }
                    }
                    if (found.empty()) {
                        error("undeclared", "undeclared name '" + std::string(identifier->GetName()) + "'", identifier);
                        metadata.Error = true;
                        identifier->resolve({}, {});
                    } else {
                        std::vector<uint32_t> ids;
                        for (auto id : found) {
                            ids.push_back(id.value);
                        }
                        identifier->resolve(std::move(ids), std::string(identifier->GetName()) + "." +
                                                                std::to_string(found.front().value));
                        identifier->semanticInfo().Scope = SymbolTable.currentScope().value;
                        metadata.Symbol = found.size() == 1 ? found.front().value : 0;
                    }
                } else if (auto* self = std::get_if<types::ThisNode*>(&payload); self && *self) {
                    auto found = SymbolTable.lookup("this");
                    if (!found.empty()) {
                        metadata.Symbol = found.front().value;
                        (*self)->semanticInfo().Symbol = found.front().value;
                        (*self)->semanticInfo().Resolved = true;
                    }
                }
            } else if (auto* cast = dynamic_cast<types::CastFactorNode*>(expression)) {
                resolveTypeOperand(cast->getType(), cast->getDeclarator());
            } else if (auto* size = dynamic_cast<types::SizeofNode*>(expression)) {
                if (auto* type = std::get_if<types::SizeofNode::SizeofTypeVariant>(&size->Payload)) {
                    resolveTypeOperand(type->Type, type->AbstractDecl);
                }
            }
            expression->visitMutableExpressions([&](auto* child) { resolveExpression(child); });
        }

        void SemanticAnalyzer::registerLocal(types::DeclarationList* list) {
            const bool isStatic = list->getIsStatic();
            // The callback executes after EACH insertion, before the next declarator.
            // Resolving all initializers after registering the whole list would make
            // int a=b,b=1 incorrectly bind the later local b.
            ASTResolver.declarationList(*list, [this, isStatic](SymbolId symbol, types::ExpressionNode* raw) {
                auto* expression = raw;
                resolveInitializer(expression);
                auto source = static_cast<const types::ASTNode*>(SymbolTable.get(symbol).source);
                queue(expression, source, ExpressionUse::Initializer, symbol, SymbolTable.get(symbol).type, isStatic);
                auto type = TypeSystem.unConst(SymbolTable.get(symbol).type);
                while (TypeSystem.get(type).kind == TypeKind::Array) {
                    type = TypeSystem.unConst(TypeSystem.get(type).element);
                }
                if (!isStatic && (expression || (TypeSystem.isAggregate(type) &&
                                                 DeclarationChecker.specialMembers(type).nontrivial))) {
                    Initialized.push_back(symbol);
                }
                // Declaration-time const dependencies are the same explicit exception
                // as enum/bound evaluation. Ordinary expression annotations wait for pass 3.
                if (expression && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                    TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                    auto value =
                        Checker.integer(expression, {EvaluationContext::Purpose::ConstantInitializer,
                                                     SymbolTable.currentScope(), SymbolTable.get(symbol).type});
                    if (value.state == ConstantState::Value) {
                        SymbolTable.setConstant(symbol, value.value);
                    }
                }
            });
        }

        void SemanticAnalyzer::resolveBody(BodyWork& body) {
            select(body.context);
            Current.scope = SymbolTable.enter(ScopeKind::Function);
            body.declaration->semanticInfo().Scope = Current.scope.value;
            body.declaration->semanticInfo().Resolved = true;
            if (Current.owner.value && !Current.isStatic) {
                auto type = TypeSystem.pointer(TypeSystem.constant(Current.owner, Current.isConst));
                SymbolTable.declare(
                    {"this", SymbolKind::Parameter, type, true, false, Linkage::None, body.declaration});
            }
            if (body.parameters) {
                ASTResolver.parameters(*body.parameters);
            }
            CurrentFlow = Flows.size();
            Flows.emplace_back();
            Initialized.clear();
            ActiveTargets.clear();
            ActiveLoopNames.clear();
            ActiveSwitches.clear();
            resolveStatement(body.body, true);
            // Gotos are function-wide, unlike loop suffixes. Resolve them after this
            // body's single traversal so forward labels need no extra AST walk.
            for (auto& jump : Flows.back().jumps) {
                if (auto* go = dynamic_cast<types::GotoStatementNode*>(jump.statement)) {
                    for (const auto& label : Flows.back().labels) {
                        if (go->getLabel()->GetName() == label.label->getTag()->GetName()) {
                            jump.target = label.label;
                            go->semanticInfo().Target = label.label->semanticInfo().Target;
                            go->getLabel()->semanticInfo().Target = go->semanticInfo().Target;
                            go->getLabel()->semanticInfo().Resolved = true;
                            break;
                        }
                    }
                }
            }
            SymbolTable.leave();
        }

        bool SemanticAnalyzer::resolveAndRename() {
            if (Phase != SemanticPhase::Registered || !Diagnostics.empty()) {
                return false;
            }
            const auto registeredExpressions = Expressions.size();
            for (size_t i = 0; i < registeredExpressions; ++i) {
                auto context = Expressions[i].context;
                auto* expression = Expressions[i].expression;
                select(context);
                resolveInitializer(expression);
                auto symbol = Expressions[i].symbol;
                if (symbol.value && expression && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                    TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                    auto value = Checker.integer(expression, {EvaluationContext::Purpose::ConstantInitializer,
                                                              context.scope, SymbolTable.get(symbol).type});
                    if (value.state == ConstantState::Value) {
                        SymbolTable.setConstant(symbol, value.value);
                    }
                }
            }
            for (auto& body : Bodies) {
                resolveBody(body);
            }
            Phase = SemanticPhase::Resolved;
            refreshDiagnostics();
            return Diagnostics.empty();
        }

        void SemanticAnalyzer::resolveStatement(types::StatementNode* node, bool outerBody) {
            if (!node) {
                return;
            }
            node->semanticInfo().Scope = SymbolTable.currentScope().value;
            node->semanticInfo().Resolved = true;
            if (auto* block = dynamic_cast<types::CompoundStatementNode*>(node)) {
                if (!outerBody) {
                    SymbolTable.enter(ScopeKind::Block);
                }
                auto initializedCount = Initialized.size();
                for (const auto& item : block->getStatements()) {
                    std::visit(
                        [&](auto* child) {
                            using T = std::remove_pointer_t<decltype(child)>;
                            if constexpr (std::is_same_v<T, types::StatementNode>) {
                                resolveStatement(child);
                            } else if constexpr (std::is_same_v<T, types::DeclarationList>) {
                                registerLocal(child);
                            } else if constexpr (std::is_same_v<T, types::TypedefNode>) {
                                ASTResolver.aliases(*child);
                            } else {
                                InvalidNodes.push_back(child); // nested definitions deliberately unsupported
                            }
                        },
                        item);
                }
                Initialized.resize(initializedCount);
                if (!outerBody) {
                    SymbolTable.leave();
                }
            } else if (auto* statement = dynamic_cast<types::ExpressionStatementNode*>(node)) {
                resolveExpression(statement->getExpression());
                queue(statement->getExpression(), node);
            } else if (auto* chain = dynamic_cast<types::IfSuperBlockStatementNode*>(node)) {
                resolveStatement(chain->getIf());
                for (auto* branch : chain->getElseIfs()) {
                    resolveStatement(branch);
                }
                resolveStatement(chain->getElse());
            } else if (auto* branch = dynamic_cast<types::IfSubBlockStatementNode*>(node)) {
                resolveExpression(branch->getCondition());
                queue(branch->getCondition(), node, ExpressionUse::Condition);
                resolveStatement(branch->getBody());
            } else if (auto* branch = dynamic_cast<types::ElseIfSubBlockStatementNode*>(node)) {
                resolveExpression(branch->getCondition());
                queue(branch->getCondition(), node, ExpressionUse::Condition);
                resolveStatement(branch->getBody());
            } else if (auto* branch = dynamic_cast<types::ElseSubBlockStatementNode*>(node)) {
                resolveStatement(branch->getBody());
            } else if (auto* selection = dynamic_cast<types::SwitchBlockStatementNode*>(node)) {
                node->semanticInfo().Target = NextTarget++;
                resolveExpression(selection->getCondition());
                auto condition = queue(selection->getCondition(), node, ExpressionUse::Switch);
                auto index = Switches.size();
                Switches.push_back({selection, condition, {}, {}, Initialized});
                ActiveSwitches.push_back(index);
                ActiveTargets.push_back(node);
                resolveStatement(selection->getBody());
                ActiveTargets.pop_back();
                ActiveSwitches.pop_back();
            } else if (auto* loop = dynamic_cast<types::IterationStatementNode*>(node)) {
                node->semanticInfo().Target = NextTarget++;
                types::IdentifierNode* label = nullptr;
                types::StatementNode* body = nullptr;
                types::ExpressionNode* condition = nullptr;
                const bool isFor = dynamic_cast<types::ForBlockStatementNode*>(loop);
                auto initializedCount = Initialized.size();
                if (auto* forLoop = dynamic_cast<types::ForBlockStatementNode*>(loop)) {
                    SymbolTable.enter(ScopeKind::For);
                    std::visit(
                        [&](auto* initializer) {
                            using T = std::remove_pointer_t<decltype(initializer)>;
                            if (!initializer) {
                                return;
                            }
                            if constexpr (std::is_same_v<T, types::ExpressionNode>) {
                                resolveExpression(initializer);
                                queue(initializer, node);
                            } else if constexpr (std::is_same_v<T, types::DeclarationList>) {
                                registerLocal(initializer);
                            } else {
                                ASTResolver.aliases(*initializer);
                            }
                        },
                        forLoop->getInitializer());
                    condition = forLoop->getCondition();
                    body = forLoop->getBody();
                    label = forLoop->getLabel();
                    resolveExpression(forLoop->getUpdate());
                    queue(forLoop->getUpdate(), node);
                } else if (auto* whileLoop = dynamic_cast<types::WhileBlockStatementNode*>(loop)) {
                    condition = whileLoop->getCondition();
                    body = whileLoop->getBody();
                    label = whileLoop->getLabel();
                } else if (auto* doLoop = dynamic_cast<types::DoWhileBlockStatementNode*>(loop)) {
                    condition = doLoop->getCondition();
                    body = doLoop->getBody();
                    label = doLoop->getLabel();
                }
                resolveExpression(condition);
                if (condition) {
                    queue(condition, node, ExpressionUse::Condition);
                }
                std::string name = label ? std::string(label->GetName()) : std::string();
                if (!name.empty() &&
                    std::find(ActiveLoopNames.begin(), ActiveLoopNames.end(), name) != ActiveLoopNames.end()) {
                    ResolutionErrors.push_back({label, "duplicate active loop label '" + name + "'"});
                }
                // Prebind the label even for do-while: its source suffix follows the body.
                if (label) {
                    label->semanticInfo().Target = node->semanticInfo().Target;
                    label->semanticInfo().Resolved = true;
                }
                ActiveTargets.push_back(node);
                ActiveLoopNames.push_back(name);
                resolveStatement(body);
                ActiveTargets.pop_back();
                ActiveLoopNames.pop_back();
                Initialized.resize(initializedCount);
                if (isFor) {
                    SymbolTable.leave();
                }
            } else if (auto* result = dynamic_cast<types::ReturnStatementNode*>(node)) {
                resolveExpression(result->getExpression());
                queue(result->getExpression(), node, ExpressionUse::Return, {}, Current.result);
            } else if (auto* label = dynamic_cast<types::GotoLabelStatementNode*>(node)) {
                label->semanticInfo().Target = NextTarget++;
                label->getTag()->semanticInfo().Target = label->semanticInfo().Target;
                label->getTag()->semanticInfo().Resolved = true;
                Flows[CurrentFlow].labels.push_back({label, Initialized});
                resolveStatement(label->getStatement());
            } else if (auto* label = dynamic_cast<types::CaseLabelStatementNode*>(node)) {
                resolveExpression(label->getTag());
                auto index = queue(label->getTag(), node, ExpressionUse::Case);
                if (ActiveSwitches.empty()) {
                    ResolutionErrors.push_back({node, "case label is only valid inside a switch"});
                } else {
                    Switches[ActiveSwitches.back()].cases.push_back(index);
                    for (auto id : Initialized) {
                        if (std::find(Switches[ActiveSwitches.back()].initialized.begin(),
                                      Switches[ActiveSwitches.back()].initialized.end(),
                                      id) == Switches[ActiveSwitches.back()].initialized.end()) {
                            ResolutionErrors.push_back(
                                {label, "switch dispatch enters an uninitialized object lifetime"});
                            break;
                        }
                    }
                }
                resolveStatement(label->getStatement());
            } else if (auto* label = dynamic_cast<types::DefaultLabelStatementNode*>(node)) {
                if (ActiveSwitches.empty()) {
                    ResolutionErrors.push_back({node, "default label is only valid inside a switch"});
                } else {
                    Switches[ActiveSwitches.back()].defaults.push_back(label);
                    for (auto id : Initialized) {
                        if (std::find(Switches[ActiveSwitches.back()].initialized.begin(),
                                      Switches[ActiveSwitches.back()].initialized.end(),
                                      id) == Switches[ActiveSwitches.back()].initialized.end()) {
                            ResolutionErrors.push_back(
                                {label, "switch dispatch enters an uninitialized object lifetime"});
                            break;
                        }
                    }
                }
                resolveStatement(label->getStatement());
            } else if (dynamic_cast<types::GotoStatementNode*>(node)) {
                Flows[CurrentFlow].jumps.push_back({node, nullptr, Initialized});
            } else if (dynamic_cast<types::BreakStatementNode*>(node) ||
                       dynamic_cast<types::ContinueStatementNode*>(node)) {
                const bool isContinue = dynamic_cast<types::ContinueStatementNode*>(node);
                auto* label = isContinue ? static_cast<types::ContinueStatementNode*>(node)->getLabel()
                                         : static_cast<types::BreakStatementNode*>(node)->getLabel();
                const types::StatementNode* target = nullptr;
                size_t loopIndex = ActiveLoopNames.size();
                for (auto it = ActiveTargets.rbegin(); it != ActiveTargets.rend(); ++it) {
                    bool loop = dynamic_cast<const types::IterationStatementNode*>(*it);
                    if (loop) {
                        --loopIndex;
                    }
                    if (label) {
                        if (loop && ActiveLoopNames[loopIndex] == label->GetName()) {
                            target = *it;
                            break;
                        }
                    } else if (loop || !isContinue) {
                        target = *it;
                        break;
                    }
                }
                if (target) {
                    node->semanticInfo().Target = target->semanticInfo().Target;
                    if (label) {
                        label->semanticInfo().Target = node->semanticInfo().Target;
                        label->semanticInfo().Resolved = true;
                    }
                }
                Flows[CurrentFlow].jumps.push_back({node, target, Initialized});
            } else if (!dynamic_cast<types::EmptyStatementNode*>(node)) {
                InvalidNodes.push_back(node);
            }
        }
    }
}
