#pragma once

#include "ast_declarations.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"

namespace baka {
    namespace semantic {
        // B resolves function/special-member signatures from its owned AST. Bodies must be
        // checked by C/B after this service finishes collecting metadata and layout.
        using MemberSignature = std::function<std::optional<Member>(types::ASTNode*, TypeId)>;

        class ASTAggregates {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            ASTDeclarations& ASTResolver;

        public:
            ASTAggregates(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                          ASTDeclarations& ASTResolver);

            TypeId forward(types::IdentifierNode* name, TypeKind kind, types::ASTNode* source = nullptr);

            TypeId definition(types::IdentifierNode* name, types::IdentifierNode* parent, types::StructBodyNode& body,
                              TypeKind kind, MemberSignature signature = {}, types::ASTNode* source = nullptr);

            TypeId definition(types::StructDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(types::ClassDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(types::UnionNode& node, MemberSignature signature = {});
        };
    }
}
