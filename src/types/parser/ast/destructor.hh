#pragma once
#include "ast_node.hh"
#include <iostream>

#include "statement.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
    namespace types {
        class DestructorNode : public ASTNode {
            StatementNode* Body;

        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getBody()); }
            DestructorNode(StatementNode* body) :
                 Body(body) {
            }

            ~DestructorNode() = default;

            StatementNode* getBody() const { return Body; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Destructor(\n";

                Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
