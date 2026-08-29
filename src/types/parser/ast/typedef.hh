#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
namespace baka {
namespace types {

    class TypedefNode : public ASTNode {
        bool isConst = false;
        // struct,enum, union or unsigned
        TypeSpecifierModifier Modifier;
        IdentifierNode* BoundReturnType;
        std::vector<DeclarationIdentifierNode*> Variables;

    public:
        TypedefNode(IdentifierNode* BoundReturnType, TypeSpecifierModifier Modifier, bool isConst) :
        BoundReturnType(BoundReturnType), Variables(), Modifier(Modifier), isConst(isConst) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Typedef(\n";

            INDENT(Tabs + 1);
            std::cout << "isConst = " << isConst << "\n";

            BoundReturnType->Print(Tabs + 1);
            for (const auto& Variable : Variables) {
                Variable->Print(Tabs + 1);
            }

            INDENT(Tabs + 1);
            std::cout << "Modifier: " << TypeSpecifierModifierToStr[Modifier] << "\n";
            std::cout << ")\n";
        }

        void AddVariable(DeclarationIdentifierNode* Variable) {
            Variables.push_back(Variable);
        }
    };


}
}
