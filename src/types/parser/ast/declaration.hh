#pragma once


#include <utility>

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"

namespace baka {
namespace types {


    class DeclarationIdentifierNode : public ASTNode
    {
        size_t PointerCount;
        std::variant<DeclarationIdentifierNode*, IdentifierNode*> VariableName;
        std::vector<ExpressionNode*> ArraySizes;

    public:
        DeclarationIdentifierNode() = default;

        DeclarationIdentifierNode(size_t PointerCount, DeclarationIdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(PointerCount), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        DeclarationIdentifierNode(IdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(0), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        void Print(size_t Tabs) const override
        {
            INDENT(Tabs);
            std::cout << "DeclarationIdentifier(" << std::endl;

            INDENT(Tabs + 1);
            std::cout << "PointerCount: " << PointerCount << std::endl;

            if(std::holds_alternative<DeclarationIdentifierNode*>(VariableName))
            {
                std::get<DeclarationIdentifierNode*>(VariableName)->Print(Tabs + 2);
            }
            else
            {
                std::get<IdentifierNode*>(VariableName)->Print(Tabs + 2);
            }

            INDENT(Tabs + 1);
            std::cout << "ArraySizes: " << std::endl;
            for (const auto& ArraySize : ArraySizes)
            {
                ArraySize->Print(Tabs + 2);
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

        void appendPointer()
        {
            PointerCount++;
        }

        void appendArraySize(ExpressionNode* ArraySize)
        {
            ArraySizes.push_back(ArraySize);
        }

        void setInnerDeclaration(std::variant<DeclarationIdentifierNode*, IdentifierNode*> InnerDeclaration)
        {
            VariableName = InnerDeclaration;
        }

    };

    class SingleDeclarationNode : public ASTNode {

        DeclarationIdentifierNode* VariableName;
        ExpressionNode* Initialization; // can be nullptr
    public:
        SingleDeclarationNode(DeclarationIdentifierNode* VariableName, ExpressionNode* Initialization) :
        VariableName(VariableName), Initialization(Initialization) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "SingleDeclarationNode(" << "\n";

            VariableName->Print(Tabs + 1);
            if (Initialization) {
                Initialization->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "nullptr\n";
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }
    };

    class DeclarationList : public ASTNode {
        bool IsStatic, IsConst;
        IdentifierNode* TypeName{}; // todo
        std::vector<SingleDeclarationNode*> Declarations;

        public:
        DeclarationList(bool IsStatic, bool IsConst, IdentifierNode* TypeName, std::vector<SingleDeclarationNode*> Declarations) :
        IsStatic(IsStatic), IsConst(IsConst), TypeName(TypeName), Declarations(std::move(Declarations)) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "DeclarationList(" << "\n";
            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << "\n";
            if (TypeName) {
                TypeName->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "nullptr\n";
            }
            for (const auto& Declaration : Declarations) {
                Declaration->Print(Tabs + 1);
            }
            INDENT(Tabs);
            std::cout << ")\n";
        }
    };



}
}
