#pragma once
#include "ast_node.hh"
#include "constant.hh"
#include <variant>
#include "identifer.hh"
#include "utils.hh"


namespace baka {

namespace types {

    class ExpressionNode : public ASTNode {
    public:
        virtual ~ExpressionNode() = default;
    };

    class PostfixExpressionNode : public ExpressionNode {
    public:
        virtual ~PostfixExpressionNode() = default;
    };



    class PrimaryExpressionNode : public ExpressionNode {
        std::variant<ConstantNode*, IdentiferNode*> Expr;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "PrimaryExpression(" << std::endl;

            if(std::holds_alternative<ConstantNode*>(Expr)) {
                std::get<ConstantNode*>(Expr)->Print(Tabs + 1);
            } else {
                std::get<IdentiferNode*>(Expr)->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };


}
}
