#pragma once
#include "ast_node.hh"
#include <iostream>
#include <utility>
#include <vector>
#include "statement.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
namespace types {


    class FunctionParameter : public ASTNode {
        bool IsConst = false;
        bool isStructOrClass = false;
        bool isEnum = false;

        bool isUnsigned = false;

        SingleDeclarationNode* SDeclarationNode;
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
            return SDeclarationNode->hasInitalizer();
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

    class FunctionNode : public ASTNode {
        IdentifierNode* ReturnType;
        IdentifierNode* FuncName;
        FunctionParameterList* Args;
        StatementNode* Body;

    public:
        FunctionNode(IdentifierNode* returnType, IdentifierNode* funcName, FunctionParameterList* args, StatementNode* body) :
            ReturnType(returnType), FuncName(funcName), Args(args), Body(body) {
        }

        ~FunctionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Function(\n";

            FuncName->Print(Tabs + 1);
            ReturnType->Print(Tabs + 1);
            Args->Print(Tabs + 1);
            Body->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
