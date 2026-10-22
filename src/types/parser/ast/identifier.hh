#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <string_view>
#include <string>
#include <vector>

namespace baka {

namespace types {

class IdentifierNode : public ASTNode {
    std::string_view IdentifierName;
    std::string ResolvedName;
    std::vector<uint32_t> Bindings;
public:
        void visitChildren(const NodeVisitor& visit) const override {  }
    IdentifierNode(std::string_view Name) : IdentifierName(Name) {}

    void Print(size_t Tabs = 0) const override {
        INDENT(Tabs);
        std::cout << "Identifier(" << IdentifierName << ")" << std::endl;
    }

    std::string_view GetName() const {
        return IdentifierName;
    }
    // Keep the source spelling for diagnostics and parser golden output. The
    // owned resolved name is the unique renamed identity used after resolution.
    const std::string& getResolvedName() const { return ResolvedName; }
    const std::vector<uint32_t>& getBindings() const { return Bindings; }
    void resolve(std::vector<uint32_t> bindings, std::string uniqueName) {
        Bindings = std::move(bindings);
        ResolvedName = std::move(uniqueName);
        semanticInfo().Resolved = true;
        semanticInfo().Symbol = Bindings.size() == 1 ? Bindings.front() : 0;
    }
};


}
}
