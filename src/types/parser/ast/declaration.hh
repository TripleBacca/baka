#pragma once


#include <utility>

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"

namespace baka {
namespace types {


    class SingleDeclarationNode;

    class DeclarationIdentifierNode : public ASTNode
    {
    public:
        size_t PointerCount;
        std::variant<DeclarationIdentifierNode*, IdentifierNode*> VariableName;
        std::vector<ExpressionNode*> ArraySizes;

        DeclarationIdentifierNode(size_t PointerCount, DeclarationIdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(PointerCount), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        DeclarationIdentifierNode(IdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(0), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        void Print(size_t Tabs) const override
        {
            INDENT(Tabs);
            std::cout << "DeclarationIdentifier(" << std::endl;
            // TODO:
        }
    };

    class DeclarationList : public ASTNode {
        bool IsStatic, IsConst;
        IdentifierNode* TypeName{}; // todo
        std::vector<SingleDeclarationNode*> Declarations;
        public:
        DeclarationList(bool IsStatic, bool IsConst, IdentifierNode* TypeName, std::vector<SingleDeclarationNode*> Declarations) :
        IsStatic(IsStatic), IsConst(IsConst), TypeName(TypeName), Declarations(std::move(Declarations)) {}

        void Print(size_t Tabs = 0) const override;
    };

    class SingleDeclarationNode : public ASTNode {

        DeclarationIdentifierNode* VariableName;
        ExpressionNode* Initialization;
    public:
        SingleDeclarationNode(DeclarationIdentifierNode* VariableName, ExpressionNode* Initialization) :
        VariableName(VariableName), Initialization(Initialization) {}
    };

}
}
