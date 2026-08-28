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
