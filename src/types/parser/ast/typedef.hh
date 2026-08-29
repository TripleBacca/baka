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

        IdentifierNode* BoundReturnType;
        DeclarationIdentifierNode* Variable;

    public:
        TypedefNode(IdentifierNode* BoundReturnType, DeclarationIdentifierNode* Variable, bool isConst) :
        BoundReturnType(BoundReturnType), Variable(Variable), isConst(isConst) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Typedef(";

            INDENT(Tabs + 1) << "isConst = " << isConst << ")\n";
            BoundReturnType->Print(Tabs + 1);
            Variable->Print(Tabs + 1);

            INDENT(Tabs + 1);
            std::cout << ")\n";
        }
    };


}
}
