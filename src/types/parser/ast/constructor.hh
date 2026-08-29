#pragma once
#include "ast_node.hh"
#include <iostream>

#include "statement.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
    namespace types {
        class ConstructorNode : public ASTNode {
            FunctionParameterList* Args;
            StatementNode* Body;

        public:
            ConstructorNode(FunctionParameterList* args, StatementNode* body) :
                Args(args), Body(body) {
            }


            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Constructor(\n";

                Args->Print(Tabs + 1);
                Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
