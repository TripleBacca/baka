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
                          ASTDeclarations& ASTResolver);

            TypeId forward(const types::IdentifierNode* name, TypeKind kind, const void* source = nullptr);

            TypeId definition(const types::IdentifierNode* name, const types::IdentifierNode* parent,
                              const types::StructBodyNode& body, TypeKind kind, MemberSignature signature = {},
                              const void* source = nullptr);

            TypeId definition(const types::StructDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(const types::ClassDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(const types::UnionNode& node, MemberSignature signature = {});
        };
    }
}
