#pragma once

#include "ast_declarations.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"

namespace baka {
    namespace semantic {
        // B resolves function/special-member signatures from its owned AST. Bodies must be
        // checked by C/B after this service finishes collecting metadata and layout.
        using MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>;

        class ASTAggregates {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            ASTDeclarations& ASTResolver;

        public:
            ASTAggregates(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                          ASTDeclarations& ASTResolver)
                : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
                  ASTResolver(ASTResolver) {}

            TypeId forward(const types::IdentifierNode* name, TypeKind kind, const void* source = nullptr) {
                if (!name || name->hasError()) {
                    return {};
                }
                return DeclarationChecker.aggregate(std::string(name->GetName()), kind, source);
            }

            TypeId definition(const types::IdentifierNode* name, const types::IdentifierNode* parent,
                              const types::StructBodyNode& body, TypeKind kind, MemberSignature signature = {},
                              const void* source = nullptr) {
                auto id =
                    DeclarationChecker.aggregate(name ? std::string(name->GetName()) : std::string{}, kind, source);
                if (!DeclarationChecker.beginAggregate(id, source)) {
                    return {};
                }
                bool valid = true;
                if (parent) {
                    valid = DeclarationChecker.base(id, SymbolTable.lookupType(std::string(parent->GetName()), parent),
                                                    parent);
                }
                SymbolTable.enter(ScopeKind::Aggregate);
                for (auto* member : body.getMembers()) {
                    if (auto* list = dynamic_cast<const types::DeclarationList*>(member)) {
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
                            bool accepted = DeclarationChecker.member(id, MemberInfo);
                            if (accepted) {
                                bool function = TypeSystem.get(TypeSystem.unConst(type)).kind == TypeKind::Function;
                                auto symbol = SymbolTable.declare(
                                    {MemberInfo.name, function ? SymbolKind::Function : SymbolKind::Object, type, false,
                                     MemberInfo.isStatic, Linkage::None, item});
                                accepted = symbol.value != 0;
                                if (accepted) {
                                    TypeSystem.bindMemberSymbol(id, TypeSystem.get(id).members.size() - 1, symbol);
                                }
                            }
                            valid = accepted && valid;
                        }
                    } else if (auto* alias = dynamic_cast<const types::TypedefNode*>(member)) {
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
                        valid = DeclarationChecker.member(id, *MemberInfo) && valid;
                    }
                }
                SymbolTable.leave();
                if (!valid || !DeclarationChecker.finishAggregate(id, source)) {
                    return {};
                }
                return id;
            }

            TypeId definition(const types::StructDefinitionNode& node, MemberSignature signature = {}) {
                return definition(node.getName(), node.getBaseName(), *node.getBody(), TypeKind::Struct,
                                  std::move(signature), &node);
            }

            TypeId definition(const types::ClassDefinitionNode& node, MemberSignature signature = {}) {
                return definition(node.getName(), node.getBaseName(), *node.getBody(), TypeKind::Class,
                                  std::move(signature), &node);
            }

            TypeId definition(const types::UnionNode& node, MemberSignature signature = {}) {
                if (!node.getBody()) {
                    return forward(node.getName(), TypeKind::Union, &node);
                }
                return definition(node.getName(), nullptr, *node.getBody(), TypeKind::Union, std::move(signature),
                                  &node);
            }
        };
    }
}
