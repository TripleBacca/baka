#pragma once
#include "ast_node.hh"
#include <iostream>
#include "statement.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
namespace types {

    class FunctionNode : public ASTNode {
        // todo use DeclarationIdentifierNode
        IdentifierNode* BindedReturnType;

        DeclarationIdentifierNode* FuncType;

        StatementNode* Body;

        bool IsStatic, IsConst;
        bool isClassOrStruct, isEnum;

        bool isUnsigned;

    public:
        FunctionNode(IdentifierNode* BindedReturnType, DeclarationIdentifierNode* funcType, StatementNode* body, bool IsStatic, bool IsConst, bool isClassOrStruct, bool isEnum, bool isUnsigned) :
            BindedReturnType(BindedReturnType), FuncType(funcType), Body(body), IsStatic(IsStatic), IsConst(IsConst), isClassOrStruct(isClassOrStruct), isEnum(isEnum), isUnsigned(isUnsigned) {
        }

        ~FunctionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Function(\n";

            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << '\n';

            INDENT(Tabs+1);
            std::cout << "isClassOrStruct: " << isClassOrStruct << ", isEnum: " << isEnum << ", isUnsigned: " << isUnsigned << std::endl;

            INDENT(Tabs+1);
            std::cout << "FuncType: " << '\n';
            FuncType->Print(Tabs + 1);

            INDENT(Tabs+1);
            std::cout << "ReturnType: " << '\n';
            BindedReturnType->Print(Tabs + 1);

            INDENT(Tabs + 1);
            std::cout << "FunctionBody:" << '\n';
            Body->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
