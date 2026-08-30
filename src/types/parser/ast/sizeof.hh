#pragma once


#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typenode.hh"
#include <variant>

#include "declaration.hh"

namespace baka {
namespace types {

    class SizeofNode : public ExpressionNode {
    public:

        struct SizeofTypeVariant {
            TypeNode* Type;
            DeclarationIdentifierNode* Variable;
        };

        std::variant<SizeofTypeVariant,
                    ExpressionNode*
        > Expression;

        SizeofNode(std::variant<SizeofTypeVariant, ExpressionNode*> Expression) : Expression(Expression) {}

        void Print(size_t Tabs = 0) const override
        {
            INDENT(Tabs);
            std::cout << "SizeofNode(\n";

            if(std::holds_alternative<SizeofTypeVariant>(Expression)) {
                const auto& Variant = std::get<SizeofTypeVariant>(Expression);
                if(Variant.Type) {
                    Variant.Type->Print(Tabs + 1);
                }
                if(Variant.Variable) {
                    Variant.Variable->Print(Tabs + 1);
                }
            } else {
                const auto& Expr = std::get<ExpressionNode*>(Expression);
                Expr->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

    };

}
}
