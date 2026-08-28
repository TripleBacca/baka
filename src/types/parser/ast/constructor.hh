#pragma once
#include "ast_node.hh"
#include <iostream>

#include "statement.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
    namespace types {
        class ConstructorNode : public ASTNode {
            IdentifierNode* ConstructorName;
            FunctionParameterList* Args;
            StatementNode* Body;

        public:
            ConstructorNode(IdentifierNode* constructorName, FunctionParameterList* args, StatementNode* body) :
                ConstructorName(constructorName), Args(args), Body(body) {
            }

            ~ConstructorNode() = default;

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Constructor(\n";

                ConstructorName->Print(Tabs + 1);
                Args->Print(Tabs + 1);
                Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
