#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"

namespace baka {
namespace types {


    class SingleDeclarationNode;


    class DeclarationList : public ASTNode {
        std::vector<SingleDeclarationNode*> Declarations;
        public:
        DeclarationList(std::vector<SingleDeclarationNode*> Declarations) : Declarations(Declarations) {};

        void Print(size_t Tabs = 0) const override;
    };

    class SingleDeclarationNode : public ASTNode {
        IdentifierNode* TypeName; // todo
        IdentifierNode* VariableName;
        ExpressionNode* Intialization;

        SingleDeclarationNode(IdentifierNode* TypeName, IdentifierNode* VariableName, ExpressionNode* Intialization);
    };


}
}
