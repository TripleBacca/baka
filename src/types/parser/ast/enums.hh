#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
#include <vector>
#include "expression.hh"
namespace baka {
namespace types {

    class EnumNode : public ASTNode {
        IdentifierNode* EnumName; // cann be nullptr - anonymous enums
        std::vector<ExpressionNode*> Initializers;
        std::vector<IdentifierNode*> EnumValues; // TODO use lsit mayube?
    public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getEnumName()); for (auto* child : getEnumValues()) visit(child); for (auto* child : getInitializers()) visit(child); }

        EnumNode(IdentifierNode* name)
            : EnumName(name) {}

        IdentifierNode* getEnumName() const {
            return EnumName;
        }

        const std::vector<IdentifierNode*>& getEnumValues() const {
            return EnumValues;
        }


        const std::vector<ExpressionNode*>& getInitializers() const {
            return Initializers;
        }


        void addEnumValue(IdentifierNode* value, ExpressionNode* initializer = nullptr) {
            EnumValues.push_back(value);
            Initializers.push_back(initializer);
        }

        bool isAnonymous() const { return EnumName == nullptr; }

        void Print(size_t Tabs = 0) const {
            INDENT(Tabs);
            std::cout << "Enum(" << (EnumName ? EnumName->GetName() : "") << ")" << std::endl;
            for (size_t i = 0; i < EnumValues.size(); ++i) {
                EnumValues[i]->Print(Tabs + 1);
                if (Initializers[i]) Initializers[i]->Print(Tabs + 2);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }

    };


}
}
