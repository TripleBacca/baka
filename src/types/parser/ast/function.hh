#pragma once
#include "ast_node.hh"
#include <iostream>
#include <string_view>
#include <utility>
#include "statement.hh"
#include "types/parser/ast/utils.hh"

namespace baka {
namespace types {
    class FunctionArgumentStatementNode : public StatementNode {
        std::string_view DataType;
        std::string_view VariableName;

    public:
        FunctionArgumentStatementNode(std::string_view dataType, std::string_view variableName) : DataType(dataType), VariableName(variableName) {
        }

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "ArgumentDecl(" << DataType << ", " << '\n';

            INDENT(Tabs + 1);
            std::cout << VariableName << '\n';

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

    class FunctionArgumentsNode : public ASTNode {
        std::vector<FunctionArgumentStatementNode*> Declarations;
        bool HasEllipsis;

    public:
        FunctionArgumentsNode(std::vector<FunctionArgumentStatementNode*> declarations, bool hasEllipses) : Declarations(std::move(declarations)), HasEllipsis(hasEllipses) {
        }

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Args(" << '\n';

            for (const auto decl : Declarations) {
                decl->Print(Tabs + 1);
            }

            if (HasEllipsis) {
                INDENT(Tabs + 1);
                std::cout << "...\n";
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

    class FunctionNode : public ASTNode {
        std::string_view ReturnType;
        std::string_view FuncName;
        FunctionArgumentsNode* Args;
        StatementNode* Body;

    public:
        FunctionNode(std::string_view returnType, std::string_view funcName, FunctionArgumentsNode* args, StatementNode* body) :
            ReturnType(returnType), FuncName(funcName), Args(args), Body(body) {
        }

        ~FunctionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Function(" << ReturnType << ", " << FuncName << ", " << '\n';

            Args->Print(Tabs + 1);

            Body->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
