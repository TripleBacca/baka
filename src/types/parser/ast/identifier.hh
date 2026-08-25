#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <string_view>

namespace baka {

namespace types {

class IdentifierNode : public ASTNode {
    std::string_view IdentiferName;
public:
    IdentifierNode(std::string_view Name) : IdentiferName(Name) {}

    void Print(size_t Tabs = 0) const override {
        INDENT(Tabs);
        std::cout << "Identifer(" << IdentiferName << ")" << std::endl;
    }
};


}
}
