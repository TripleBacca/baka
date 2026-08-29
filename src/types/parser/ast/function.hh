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
        IdentifierNode* BoundReturnType;

        DeclarationIdentifierNode* FuncType;

        StatementNode* Body;

        bool IsStatic = false, IsConst = false;
        bool isClassOrStruct = false, isEnum = false, isUnion = false, isUnsigned = false;

    public:
        FunctionNode(IdentifierNode* BindedReturnType, DeclarationIdentifierNode* funcType, StatementNode* body, bool IsStatic, bool IsConst, bool isClassOrStruct, bool isEnum, bool isUnion, bool isUnsigned) :
            BoundReturnType(BindedReturnType), FuncType(funcType), Body(body), IsStatic(IsStatic), IsConst(IsConst), isClassOrStruct(isClassOrStruct), isEnum(isEnum), isUnion(isUnion), isUnsigned(isUnsigned) {
            if(isUnsigned && !detail::isUnsignedTypeName(BoundReturnType->GetName())) {
                // todo throw error
                assert(false);
            }
            if ((int)isClassOrStruct + (int)isEnum + (int)isUnion > 1) {
                // todo throw error
                assert(false && "Cannot be more than one of class/struct/enum/union");
            }
        }

        ~FunctionNode() override = default;

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "Function(\n";

            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << '\n';

            INDENT(Tabs+1);
            std::cout << "isClassOrStruct: " << isClassOrStruct << ", isEnum: " << isEnum << ", isUnion" << isUnion << ", isUnsigned: " << isUnsigned << std::endl;

            INDENT(Tabs+1);
            std::cout << "FuncType: " << '\n';
            FuncType->Print(Tabs + 1);

            INDENT(Tabs+1);
            std::cout << "ReturnType: " << '\n';
            BoundReturnType->Print(Tabs + 1);

            INDENT(Tabs + 1);
            std::cout << "FunctionBody:" << '\n';
            Body->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
