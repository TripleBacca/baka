#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
#include <vector>
namespace baka {
namespace types {

    class EnumNode : public ASTNode {
        IdentifierNode* EnumName; // cann be nullptr - anonymous enums
        std::vector<IdentifierNode*> EnumValues; // TODO use lsit mayube?
    public:

        EnumNode(IdentifierNode* name)
            : EnumName(name) {}

        IdentifierNode* getEnumName() const { return EnumName; }
        const std::vector<IdentifierNode*>& getEnumValues() const { return EnumValues; }

        void addEnumValue(IdentifierNode* value) {
            EnumValues.push_back(value);
        }

        void Print(size_t Tabs = 0) const {
            INDENT(Tabs);
            std::cout << "Enum(" << EnumName->GetName() << ")" << std::endl;
            for (const auto& value : EnumValues) {
                value->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }

    };


}
}
