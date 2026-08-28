#pragma once


#include <iostream>
#include <utility>
#include <vector>

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"

namespace baka {
namespace types {


    class DeclarationIdentifierNode : public ASTNode
    {
        size_t PointerCount;
        // TODO: use typenode instead of identifier
        std::variant<DeclarationIdentifierNode*, IdentifierNode*> VariableName;
        std::vector<ExpressionNode*> ArraySizes;

        ASTNode* FunctionParameters = nullptr;

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

            if(FunctionParameters)
            {
                INDENT(Tabs + 1);
                std::cout << "FunctionParameters: " << std::endl;
                FunctionParameters->Print(Tabs + 2);
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

        void setFunctionParameters(ASTNode* Params)
        {
            this->FunctionParameters = Params;
        }

        size_t getPointerCount() const
        {
            return PointerCount;
        }

        // Returns true if this node or any nested DeclarationIdentifierNode
        // has at least one pointer level - used to validate function pointer
        // need for: int ((*fp))(int x) : we need to check if the innermost thingy has a * cuz it has to be a ptr
        bool hasPointerAtAnyLevel() const
        {
            if (PointerCount > 0) return true;
            if (std::holds_alternative<DeclarationIdentifierNode*>(VariableName))
                return std::get<DeclarationIdentifierNode*>(VariableName)->hasPointerAtAnyLevel();
            return false;
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

        bool hasInitalizer() const {
            return Initialization != nullptr;
        }
    };

    class DeclarationList : public ASTNode {
        bool IsStatic, IsConst;
        bool isClassOrStruct, isEnum;

        bool isUnsigned;
        IdentifierNode* TypeName{}; // todo
        std::vector<SingleDeclarationNode*> Declarations;

        public:
        DeclarationList(bool IsStatic, bool IsConst, bool isClassOrStruct, bool isEnum, bool isUnsigned, IdentifierNode* TypeName, std::vector<SingleDeclarationNode*> Declarations) :
        IsStatic(IsStatic), IsConst(IsConst), isClassOrStruct(isClassOrStruct), isEnum(isEnum), isUnsigned(isUnsigned), TypeName(TypeName), Declarations(std::move(Declarations)) {
            if(isUnsigned && !detail::isUnsignedTypeName(TypeName->GetName())) {
                // todo throw error
                assert(false);
            }
        }

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
            INDENT(Tabs + 1);
            std::cout << "isUnsigned: " << isUnsigned << "\n";

            for (const auto& Declaration : Declarations) {
                Declaration->Print(Tabs + 1);
            }
            INDENT(Tabs);
            std::cout << ")\n";
        }
    };


    class FunctionParameter : public ASTNode {
        bool IsConst = false;
        bool isStructOrClass = false;
        bool isEnum = false;

        bool isUnsigned = false;

        SingleDeclarationNode* SDeclarationNode; // can be nullptr cuz bare type: int balls(int,int)
        IdentifierNode* TypeName;

        public:
        FunctionParameter(bool IsConst, IdentifierNode* TypeName, SingleDeclarationNode* SingleDeclarationNode, bool isStructOrClass, bool isEnum, bool isUnsigned) :
        IsConst(IsConst), TypeName(TypeName), SDeclarationNode(SingleDeclarationNode), isStructOrClass(isStructOrClass), isEnum(isEnum), isUnsigned(isUnsigned) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "FunctionParameter(" << "\n";
            INDENT(Tabs + 1);
            std::cout << "IsConst: " << IsConst << "\n";
            INDENT(Tabs + 1);
            std::cout << "isStructOrClass: " << isStructOrClass << "\n";
            INDENT(Tabs + 1);
            std::cout << "isEnum: " << isEnum << "\n";
            INDENT(Tabs + 1);
            std::cout << "isUnsigned: " << isUnsigned << "\n";
            if (TypeName) {
                TypeName->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "nullptr\n";
            }
            if (SDeclarationNode) {
                SDeclarationNode->Print(Tabs + 1);
            }
            INDENT(Tabs);
            std::cout << ")\n";
        }


        bool hasInitalizer() const {
            if(SDeclarationNode)
                return SDeclarationNode->hasInitalizer();
            return false;
        }
    };

    // TODO: use pimpl idiom

    class FunctionParameterList : public ASTNode {
        std::vector<FunctionParameter*> Parameters;
    public:
        FunctionParameterList(std::vector<FunctionParameter*> parameters) : Parameters(std::move(parameters)) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Args(" << '\n';

            for (const auto param : Parameters) {
                param->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };



}
}
