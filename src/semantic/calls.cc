#include "expressions.hh"

namespace baka {
    namespace semantic {
        size_t ExpressionChecker::requiredArguments(SymbolId symbol, TypeId type, uint64_t localPoint) const {
            size_t required = TypeSystem.get(type).parameters.size();
            if (!symbol.value) {
                return required; // function pointers never inherit defaults
            }
            const auto& entry = SymbolTable.get(symbol);
            if (entry.kind != SymbolKind::Function && entry.kind != SymbolKind::BuiltinFunction) {
                return required;
            }
            while (required && required <= entry.defaultArguments.size() &&
                   SymbolTable.defaultVisible(symbol, required - 1, localPoint)) {
                --required;
            }
            return required;
        }

        std::vector<ExprResult> ExpressionChecker::argumentList(const types::ExpressionNode* list) {
            std::vector<ExprResult> result;
            if (!list) {
                return result;
            }
            auto* comma = dynamic_cast<const types::CommaExpressionNode*>(list);
            if (!comma) {
                result.push_back(check(list, true));
                return result;
            }
            for (auto* expression : comma->getExpressions()) {
                result.push_back(check(expression, true));
            }
            return result;
        }

        std::vector<SymbolId> ExpressionChecker::resolveCallee(const types::ExpressionNode* callee,
                                                               const types::ASTNode* source) {
            std::vector<SymbolId> result;
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(callee);
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                if (identifier) {
                    result = lookupIdentifier(identifier);
                }
            }
            (void)source;
            return result;
        }

        std::vector<ExpressionChecker::FunctionCandidate>
        ExpressionChecker::functionCandidates(const types::ExpressionNode* callee, const types::ASTNode* source) {
            std::vector<FunctionCandidate> candidates;
            while (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(callee)) {
                if (comma->getExpressions().size() != 1) {
                    break;
                }
                callee = comma->getExpressions().front();
            }
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(callee);
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                if (!identifier) {
                    return candidates;
                }
                for (auto symbolId : lookupIdentifier(identifier)) {
                    const auto& symbol = SymbolTable.get(symbolId);
                    auto type = symbol.type;
                    if (symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::BuiltinFunction) {
                        if (InStaticContext && !symbol.isStatic &&
                            SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate) {
                            SymbolTable.diagnose("member-context", "instance member requires an object", identifier);
                            continue;
                        }
                        TypeId owner{};
                        uint32_t memberIndex = 0;
                        if (CurrentClass.value && SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate) {
                            auto member = TypeSystem.lookupMember(CurrentClass, symbol.name);
                            for (auto index : member.indices) {
                                if (TypeSystem.get(member.owner).members[index].symbol == symbolId) {
                                    owner = member.owner;
                                    memberIndex = static_cast<uint32_t>(index + 1);
                                }
                            }
                        }
                        candidates.push_back(
                            {type, symbolId, requiredArguments(symbolId, type, callee->semanticInfo().DeclarationPoint),
                             owner, memberIndex});
                        continue;
                    }
                    auto kind = TypeSystem.get(TypeSystem.unConst(type)).kind;
                    if (kind == TypeKind::Function) {
                        candidates.push_back(
                            {type, symbolId,
                             requiredArguments(symbolId, type, callee->semanticInfo().DeclarationPoint)});
                    } else if (kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.get(TypeSystem.unConst(type)).element;
                        if (TypeSystem.get(TypeSystem.unConst(pointee)).kind == TypeKind::Function) {
                            auto raw = check(callee, false);
                            if (!raw.error) {
                                candidates.push_back({pointee,
                                                      {},
                                                      TypeSystem.get(pointee).parameters.size(),
                                                      {callee->semanticInfo().MemberOwner},
                                                      callee->semanticInfo().MemberIndex});
                            }
                        }
                    }
                }
                return candidates;
            }
            if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(callee)) {
                auto base = check(member->getBase(), false);
                if (base.error) {
                    return candidates;
                }
                auto access = DeclarationChecker.memberAccess(base.type, std::string(member->getMember()->GetName()),
                                                              false, false, source);
                for (auto index : access.candidates) {
                    const auto& entry = TypeSystem.get(access.owner).members[index];
                    auto type = TypeSystem.unConst(entry.type);
                    if (TypeSystem.get(type).kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.unConst(TypeSystem.get(type).element);
                        if (TypeSystem.get(pointee).kind == TypeKind::Function && !check(callee, false).error) {
                            candidates.push_back({pointee,
                                                  {},
                                                  TypeSystem.get(pointee).parameters.size(),
                                                  access.owner,
                                                  static_cast<uint32_t>(index + 1)});
                        }
                    } else if (TypeSystem.get(type).kind == TypeKind::Function) {
                        candidates.push_back(
                            {type, entry.symbol,
                             entry.symbol.value
                                 ? requiredArguments(entry.symbol, type, callee->semanticInfo().DeclarationPoint)
                                 : entry.minimumArity,
                             access.owner, static_cast<uint32_t>(index + 1)});
                    }
                }
                return candidates;
            }
            if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(callee)) {
                auto base = check(arrow->getBase(), true);
                if (base.error) {
                    return candidates;
                }
                auto access = DeclarationChecker.memberAccess(base.type, std::string(arrow->getMember()->GetName()),
                                                              true, false, source);
                for (auto index : access.candidates) {
                    const auto& entry = TypeSystem.get(access.owner).members[index];
                    auto type = TypeSystem.unConst(entry.type);
                    if (TypeSystem.get(type).kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.unConst(TypeSystem.get(type).element);
                        if (TypeSystem.get(pointee).kind == TypeKind::Function && !check(callee, false).error) {
                            candidates.push_back({pointee,
                                                  {},
                                                  TypeSystem.get(pointee).parameters.size(),
                                                  access.owner,
                                                  static_cast<uint32_t>(index + 1)});
                        }
                    } else if (TypeSystem.get(type).kind == TypeKind::Function) {
                        candidates.push_back(
                            {type, entry.symbol,
                             entry.symbol.value
                                 ? requiredArguments(entry.symbol, type, callee->semanticInfo().DeclarationPoint)
                                 : entry.minimumArity,
                             access.owner, static_cast<uint32_t>(index + 1)});
                    }
                }
                return candidates;
            }
            auto value = check(callee, true);
            if (!value.error) {
                auto kind = TypeSystem.get(TypeSystem.unConst(value.type)).kind;
                if (kind == TypeKind::Function) {
                    candidates.push_back({value.type, {}, TypeSystem.get(value.type).parameters.size()});
                } else if (kind == TypeKind::Pointer) {
                    auto pointee = TypeSystem.get(TypeSystem.unConst(value.type)).element;
                    if (TypeSystem.get(TypeSystem.unConst(pointee)).kind == TypeKind::Function) {
                        candidates.push_back({pointee, {}, TypeSystem.get(pointee).parameters.size()});
                    }
                }
            }
            return candidates;
        }

        ExprResult ExpressionChecker::checkCall(const types::FunctionCallPostfixExpr* node) {
            auto arguments = argumentList(node->getArguments());
            for (const auto& argument : arguments) {
                if (argument.error) {
                    return makeError();
                }
            }

            // Constructor call: T(args) where T names an aggregate type.
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(node->getCallee());
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                auto found = lookupIdentifier(identifier);
                if (found.size() == 1 && SymbolTable.get(found.front()).kind == SymbolKind::Type) {
                    auto type = SymbolTable.get(found.front()).type;
                    if (TypeSystem.isAggregate(type)) {
                        if (!isCompleteObject(type)) {
                            SymbolTable.diagnose("constructor", "construction requires a complete object type", node);
                            return makeError();
                        }
                        if (Annotating) {
                            auto& metadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                            metadata.Construction = true;
                            metadata.MemberOwner = type.value;
                        }
                        auto special = DeclarationChecker.specialMembers(type);
                        if (special.constructors.empty()) {
                            if (!arguments.empty()) {
                                SymbolTable.diagnose("constructor", "aggregate has no matching constructor", node);
                                return makeError();
                            }
                        } else {
                            std::vector<FunctionCandidate> candidates;
                            for (auto index : special.constructors) {
                                const auto& constructor = TypeSystem.get(TypeSystem.unConst(type)).members[index];
                                candidates.push_back({constructor.type,
                                                      {},
                                                      constructor.minimumArity,
                                                      TypeSystem.unConst(type),
                                                      static_cast<uint32_t>(index + 1)});
                            }
                            auto selection = selectOverload(candidates, arguments);
                            if (selection.ambiguous) {
                                SymbolTable.diagnose("overload", "ambiguous overload resolution", node);
                                return makeError();
                            }
                            if (!selection.candidate) {
                                SymbolTable.diagnose("constructor", "no viable overload for this constructor call",
                                                     node);
                                return makeError();
                            }
                            retainConversions(node, selection);
                            if (Annotating) {
                                auto& metadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                                metadata.CallSignature = selection.candidate->type.value;
                                metadata.MemberIndex = selection.candidate->memberIndex;
                            }
                        }
                        return makeValue(type);
                    }
                }
            }

            const auto diagnosticsBeforeCandidates = SymbolTable.diagnostics().size();
            auto candidates = functionCandidates(node->getCallee(), node);
            if (candidates.empty()) {
                // Member/name lookup may already have explained why this callee
                // is invalid; avoid obscuring it with a second call diagnostic.
                if (SymbolTable.diagnostics().size() != diagnosticsBeforeCandidates ||
                    check(node->getCallee(), false).error) {
                    return makeError();
                }
                SymbolTable.diagnose("call", "called expression is not a function", node);
                return makeError();
            }
            const auto selection = selectOverload(candidates, arguments);
            if (selection.ambiguous) {
                SymbolTable.diagnose("overload", "ambiguous overload resolution", node);
                return makeError();
            }
            if (!selection.candidate) {
                SymbolTable.diagnose("argument", "no viable overload for this call", node);
                return makeError();
            }
            retainConversions(node, selection);
            const auto& selected = *selection.candidate;
            const auto best = selected.type;
            const auto bestSymbol = selected.symbol;
            const auto bestOwner = selected.owner;
            const auto bestMemberIndex = selected.memberIndex;
            if (!bestSymbol.value && check(node->getCallee(), false).error) {
                return makeError();
            }
            if (Annotating) {
                auto& callMetadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                callMetadata.CallSignature = best.value;
                callMetadata.MemberOwner = bestOwner.value;
                callMetadata.MemberIndex = bestMemberIndex;
                auto* callee = const_cast<types::ExpressionNode*>(node->getCallee());
                // A function-pointer object retains its pointer type and lvalue
                // category; selected signature is stored separately for lowering.
                auto original = RawResults.find(callee);
                if (original != RawResults.end()) {
                    callee->semanticInfo().Type = original->second.type.value;
                    callee->semanticInfo().LValue = original->second.category == ValueCategory::LValue;
                } else {
                    callee->semanticInfo().Type = best.value;
                }
                callee->semanticInfo().Symbol = bestSymbol.value;
                callee->semanticInfo().MemberOwner = bestOwner.value;
                callee->semanticInfo().MemberIndex = bestMemberIndex;
                callee->semanticInfo().Annotated = true;
                auto* selectedCallee = callee;
                while (auto* comma = dynamic_cast<types::CommaExpressionNode*>(selectedCallee)) {
                    if (comma->getExpressions().size() != 1) {
                        break;
                    }
                    selectedCallee = comma->getExpressions().front();
                }
                if (selectedCallee != callee && bestSymbol.value) {
                    selectedCallee->semanticInfo() = callee->semanticInfo();
                }
                types::IdentifierNode* selectedName = nullptr;
                if (auto* primaryCallee = dynamic_cast<types::PrimaryExpressionNode*>(selectedCallee)) {
                    auto payload = primaryCallee->getExpression();
                    if (auto* identifier = std::get_if<types::IdentifierNode*>(&payload)) {
                        selectedName = *identifier;
                    }
                } else if (auto* memberCallee = dynamic_cast<types::MemberPostfixExpr*>(selectedCallee)) {
                    selectedName = memberCallee->getMember();
                } else if (auto* arrowCallee = dynamic_cast<types::ArrowPostfixExpr*>(selectedCallee)) {
                    selectedName = arrowCallee->getMember();
                }
                if (selectedName && bestSymbol.value) {
                    selectedName->resolve({bestSymbol.value}, std::string(selectedName->GetName()) + "." +
                                                                  std::to_string(bestSymbol.value));
                    selectedName->semanticInfo().Type = best.value;
                    selectedName->semanticInfo().Annotated = true;
                }
                const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo().Symbol = bestSymbol.value;
            }
            auto resultType = TypeSystem.get(best).element;
            if (TypeSystem.isAggregate(resultType)) {
                if (!isCompleteObject(resultType)) {
                    SymbolTable.diagnose("call", "call requires a complete result type", node);
                    return makeError();
                }
            }
            return makeValue(resultType);
        }
    }
}
