#pragma once
#include "expression.hh"
#include "utils.hh"

namespace baka {
namespace types {

    class TerneryOpNode : public ExpressionNode {
        ExpressionNode* ConditionExpr;
        ExpressionNode* ThenExpr;
        ExpressionNode* ElseExpr;

        public:
        TerneryOpNode(ExpressionNode* ConditionExpr, ExpressionNode* ThenExpr, ExpressionNode* ElseExpr) : ConditionExpr(ConditionExpr), ThenExpr(ThenExpr), ElseExpr(ElseExpr) {}

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "TerneryOpNode(" << '\n';

            ConditionExpr->Print(Tabs + 1);
            ThenExpr->Print(Tabs + 1);
            ElseExpr->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ')' << std::endl;
        }
    };


}
}
