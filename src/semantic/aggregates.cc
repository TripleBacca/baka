#include "aggregates.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/initializer.hh"
#include <variant>

namespace baka {
    namespace semantic {
        ASTAggregates::ASTAggregates(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                                     ASTDeclarations& ASTResolver)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
              ASTResolver(ASTResolver) {}

        TypeId ASTAggregates::forward(types::IdentifierNode* name, TypeKind kind, types::ASTNode* source) {
            if (!name || name->hasError()) {
                return {};
            }
            return DeclarationChecker.aggregate(std::string(name->GetName()), kind, source);
        }

        namespace {
            // The pure specifier is the literal `0`, possibly as a one-element initializer.
            bool PureSpecifier(const types::ExpressionNode* initializer) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(initializer)) {
                    if (list->expressions().size() != 1) {
                        return false;
                    }
                    initializer = list->expressions().front();
                }
                auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(initializer);
                if (!primary || !std::holds_alternative<types::ConstantNode*>(primary->getExpression())) {
                    return false;
                }
                auto* constant =
                    dynamic_cast<const types::ConstantTNode<int>*>(std::get<types::ConstantNode*>(primary->getExpression()));
                return constant && constant->getValue() == 0;
            }

            // Written access, or the default of the class-key used by the definition.
            Access Resolve(types::AccessSpecifier written, TypeKind kind) {
                switch (written) {
                case types::AccessSpecifier::Public: return Access::Public;
                case types::AccessSpecifier::Protected: return Access::Protected;
                case types::AccessSpecifier::Private: return Access::Private;
                default: return kind == TypeKind::Class ? Access::Private : Access::Public;
                }
            }
        }

        TypeId ASTAggregates::definition(types::IdentifierNode* name, types::IdentifierNode* parent,
                                         types::AccessSpecifier baseAccess, types::StructBodyNode& body,
                                         TypeKind kind, MemberSignature signature, types::ASTNode* source) {
            auto id = DeclarationChecker.aggregate(name ? std::string(name->GetName()) : std::string{}, kind, source);
            if (!DeclarationChecker.beginAggregate(id, source)) {
                return {};
            }
            bool valid = true;
            if (parent) {
                valid = DeclarationChecker.base(id, SymbolTable.lookupType(std::string(parent->GetName()), parent),
                                                Resolve(baseAccess, kind), parent);
            }
            auto aggregateScope = SymbolTable.enter(ScopeKind::Aggregate);
            body.semanticInfo().Scope = aggregateScope.value;
            const auto& members = body.getMembers();
            for (size_t memberIndex = 0; memberIndex < members.size(); ++memberIndex) {
                auto* member = members[memberIndex];
                const auto access = Resolve(body.getMemberAccess(memberIndex), kind);
                if (auto* list = dynamic_cast<types::DeclarationList*>(member)) {
                    auto base =
                        ASTResolver.specifier(list->getTypeName(), list->getModifier(), list->getIsConst(), list);
                    for (auto* item : list->getDeclarations()) {
                        if (!item || !item->getDeclarator() || !item->getDeclarator()->getIdentifier()) {
                            valid = false;
                            continue;
                        }
                        auto type = ASTResolver.declarator(base, item->getDeclarator());
                        Member MemberInfo{
                            std::string(item->getDeclarator()->getVariableName()), type, {}, list->getIsStatic()};
                        MemberInfo.source = item;
                        MemberInfo.initializer = item->getInitializer();
                        MemberInfo.access = access;
                        if (TypeSystem.get(TypeSystem.unConst(type)).kind == TypeKind::Function &&
                            MemberInfo.initializer) {
                            // `int area() = 0;` declares a pure method; it is a
                            // specifier, not a value to initialize anything with.
                            if (!PureSpecifier(MemberInfo.initializer)) {
                                SymbolTable.diagnose("pure", "a method declaration can only be followed by '= 0'",
                                                     item);
                                valid = false;
                                continue;
                            }
                            MemberInfo.isPure = true;
                            MemberInfo.initializer = nullptr;
                        }
                        bool accepted = DeclarationChecker.member(id, MemberInfo);
                        if (accepted) {
                            bool function = TypeSystem.get(TypeSystem.unConst(type)).kind == TypeKind::Function;
                            auto symbol = SymbolTable.declare({MemberInfo.name,
                                                               function ? SymbolKind::Function : SymbolKind::Object,
                                                               type, false, MemberInfo.isStatic, Linkage::None, item});
                            accepted = symbol.value != 0;
                            item->semanticInfo() = {type.value, symbol.value, aggregateScope.value, 0, true};
                            item->getDeclarator()->semanticInfo().Symbol = symbol.value;
                            auto* identifier = item->getDeclarator()->getIdentifier();
                            if (identifier && symbol.value) {
                                identifier->resolve({symbol.value},
                                                    MemberInfo.name + "." + std::to_string(symbol.value));
                                identifier->semanticInfo().Type = type.value;
                            }
                            if (accepted) {
                                TypeSystem.bindMemberSymbol(id, TypeSystem.get(id).members.size() - 1, symbol);
                            }
                        }
                        valid = accepted && valid;
                    }
                } else if (auto* alias = dynamic_cast<types::TypedefNode*>(member)) {
                    auto aliases = ASTResolver.aliases(*alias);
                    for (auto symbol : aliases) {
                        if (!symbol.value) {
                            valid = false;
                            continue;
                        }
                        const auto& entry = SymbolTable.get(symbol);
                        Member MemberInfo{entry.name, entry.type, symbol};
                        MemberInfo.isType = true;
                        MemberInfo.source = alias;
                        MemberInfo.access = access;
                        valid = DeclarationChecker.member(id, MemberInfo) && valid;
                    }
                } else {
                    auto MemberInfo = signature ? signature(member, id) : std::nullopt;
                    if (!MemberInfo) {
                        SymbolTable.diagnose("member-signature", "member signature service unavailable", member);
                        valid = false;
                        continue;
                    }
                    MemberInfo->source = member;
                    MemberInfo->access = access;
                    bool accepted = DeclarationChecker.member(id, *MemberInfo);
                    SymbolId symbol;
                    if (accepted && !MemberInfo->isConstructor && !MemberInfo->isDestructor) {
                        symbol = SymbolTable.declare({MemberInfo->name, SymbolKind::Function, MemberInfo->type, true,
                                                      MemberInfo->isStatic, Linkage::None, member});
                        if (symbol.value) {
                            TypeSystem.bindMemberSymbol(id, TypeSystem.get(id).members.size() - 1, symbol);
                        }
                    }
                    member->semanticInfo() = {MemberInfo->type.value, symbol.value, aggregateScope.value, 0, true};
                    valid = accepted && valid;
                }
            }
            SymbolTable.leave();
            if (!valid || !DeclarationChecker.finishAggregate(id, source)) {
                return {};
            }
            if (source) {
                source->semanticInfo() = {id.value, 0, aggregateScope.value, 0, true};
            }
            return id;
        }

        TypeId ASTAggregates::definition(types::StructDefinitionNode& node, MemberSignature signature) {
            return definition(node.getName(), node.getBaseName(), node.getBaseAccess(), *node.getBody(), TypeKind::Struct,
                              std::move(signature), &node);
        }

        TypeId ASTAggregates::definition(types::ClassDefinitionNode& node, MemberSignature signature) {
            return definition(node.getName(), node.getBaseName(), node.getBaseAccess(), *node.getBody(), TypeKind::Class,
                              std::move(signature), &node);
        }

        TypeId ASTAggregates::definition(types::UnionNode& node, MemberSignature signature) {
            if (!node.getBody()) {
                return forward(node.getName(), TypeKind::Union, &node);
            }
            return definition(node.getName(), nullptr, types::AccessSpecifier::Unspecified, *node.getBody(),
                              TypeKind::Union, std::move(signature), &node);
        }
    }
}
