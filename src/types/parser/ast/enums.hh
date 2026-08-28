#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
#include <list>
namespace baka {
namespace types {

    class EnumNode : public ASTNode {
        IdentifierNode* EnumName;
        std::list<IdentifierNode*> EnumValues; // change most vectors to lists
    public:

        EnumNode(IdentifierNode* name)
            : EnumName(name) {}

        IdentifierNode* getEnumName() const { return EnumName; }
        const std::list<IdentifierNode*>& getEnumValues() const { return EnumValues; }

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
