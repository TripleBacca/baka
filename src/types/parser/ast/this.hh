#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
namespace baka {
namespace types {

    class ThisNode : public ASTNode {
    public:
        void visitChildren(const NodeVisitor& visit) const override {  }
        ThisNode() = default;
        ~ThisNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "This()\n" << std::endl;
        }
    };


}
}
