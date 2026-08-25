#pragma once
#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
namespace types {

    class ReturnStatementNode : public StatementNode {
         ExpressionNode* Expr;

     public:
         ReturnStatementNode(ExpressionNode* expr) : Expr(expr) {
         }

         void Print(size_t Tabs = 0) const override {
             INDENT(Tabs);
             std::cout << "Return(" << "\n";

             if (Expr) Expr->Print(Tabs + 1);

             INDENT(Tabs);
             std::cout << ")" << std::endl;
         }
     };


}
}
