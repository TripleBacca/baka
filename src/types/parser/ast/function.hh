#pragma once
#include "ast_node.hh"
#include <iostream>
#include <string_view>
#include "statement.hh"
#include "types/parser/ast/utils.hh"

namespace baka {
namespace types {

    class FunctionNode : public ASTNode {
        std::string_view ReturnType;
        std::string_view FuncName;
        StatementNode* Body;

    public:
        FunctionNode(std::string_view returnType, std::string_view funcName, StatementNode* body) :
            ReturnType(returnType), FuncName(funcName), Body(body) {
        }

        ~FunctionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Function(" << ReturnType << ", " << FuncName << ", " << '\n';

            Body->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

}
}
