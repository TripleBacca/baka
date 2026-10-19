#include "aggregates.hh"

namespace baka {
    namespace semantic {
        ASTAggregates::ASTAggregates(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                                     ASTDeclarations& ASTResolver)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
              ASTResolver(ASTResolver) {}

        TypeId ASTAggregates::forward(const types::IdentifierNode* name, TypeKind kind, const void* source) {
            if (!name || name->hasError()) {
                return {};
            }
            return DeclarationChecker.aggregate(std::string(name->GetName()), kind, source);
        }

        TypeId ASTAggregates::definition(const types::IdentifierNode* name, const types::IdentifierNode* parent,
                                         const types::StructBodyNode& body, TypeKind kind,
                                         MemberSignature signature, const void* source) {
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
            auto aggregateScope = SymbolTable.enter(ScopeKind::Aggregate);
            const_cast<types::StructBodyNode&>(body).semanticInfo().Scope = aggregateScope.value;
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
                            const_cast<types::SingleDeclarationNode*>(item)->semanticInfo() =
                                {type.value, symbol.value, aggregateScope.value, 0, true};
                            item->getDeclarator()->semanticInfo().Symbol = symbol.value;
                            auto* identifier = item->getDeclarator()->getIdentifier();
                            if (identifier && symbol.value) {
                                identifier->resolve({symbol.value}, MemberInfo.name + "." + std::to_string(symbol.value));
                                identifier->semanticInfo().Type = type.value;
                            }
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
                    bool accepted = DeclarationChecker.member(id, *MemberInfo);
                    SymbolId symbol;
                    if (accepted && !MemberInfo->isConstructor && !MemberInfo->isDestructor) {
                        symbol = SymbolTable.declare({MemberInfo->name, SymbolKind::Function, MemberInfo->type,
                                                     true, MemberInfo->isStatic, Linkage::None, member});
                        if (symbol.value) TypeSystem.bindMemberSymbol(id, TypeSystem.get(id).members.size()-1, symbol);
                    }
                    const_cast<types::ASTNode*>(member)->semanticInfo() =
                        {MemberInfo->type.value, symbol.value, aggregateScope.value, 0, true};
                    valid = accepted && valid;
                }
            }
            SymbolTable.leave();
            if (!valid || !DeclarationChecker.finishAggregate(id, source)) {
                return {};
            }
            if (source) const_cast<types::ASTNode*>(static_cast<const types::ASTNode*>(source))->semanticInfo() =
                {id.value, 0, aggregateScope.value, 0, true};
            return id;
        }

        TypeId ASTAggregates::definition(const types::StructDefinitionNode& node, MemberSignature signature) {
            return definition(node.getName(), node.getBaseName(), *node.getBody(), TypeKind::Struct,
                              std::move(signature), &node);
        }

        TypeId ASTAggregates::definition(const types::ClassDefinitionNode& node, MemberSignature signature) {
            return definition(node.getName(), node.getBaseName(), *node.getBody(), TypeKind::Class,
                              std::move(signature), &node);
        }

        TypeId ASTAggregates::definition(const types::UnionNode& node, MemberSignature signature) {
            if (!node.getBody()) {
                return forward(node.getName(), TypeKind::Union, &node);
            }
            return definition(node.getName(), nullptr, *node.getBody(), TypeKind::Union, std::move(signature),
                              &node);
        }
    }
}
