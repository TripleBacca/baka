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
            DeclarationIdentifierNode* AbstractDecl;
        };

        std::variant<SizeofTypeVariant,
                    ExpressionNode*
        > Payload;

        SizeofNode(std::variant<SizeofTypeVariant, ExpressionNode*> Payload) : Payload(Payload) {}

        const std::variant<SizeofTypeVariant, ExpressionNode*>& getPayload() const { return Payload; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { if (auto* expr = std::get_if<ExpressionNode*>(&Payload)) *expr = rewrite(*expr); }

            void Print(size_t Tabs = 0) const override
        {
            INDENT(Tabs);
            std::cout << "SizeofNode(\n";

            if(std::holds_alternative<SizeofTypeVariant>(Payload)) {
                const auto& Variant = std::get<SizeofTypeVariant>(Payload);
                if(Variant.Type) {
                    Variant.Type->Print(Tabs + 1);
                }
                if(Variant.AbstractDecl) {
                    Variant.AbstractDecl->Print(Tabs + 1);
                }
            } else {
                const auto& Expr = std::get<ExpressionNode*>(Payload);
                Expr->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

    };

}
}
