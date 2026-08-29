#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/structBody.hh"
#include "types/parser/ast/utils.hh"
namespace baka {
namespace types {

    using UnionBodyNode = StructBodyNode; // union body is the same as struct body

    class UnionNode : public ASTNode {
        UnionBodyNode* body;
    public:
        UnionNode(UnionBodyNode* body) : body(body) {}
        ~UnionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Union(" << std::endl;
            body->Print(Tabs + 1);
            INDENT(Tabs);
            std::cout << ")\n";
        }
    };

}
}
