#pragma once
#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class JumpStatementNode : public StatementNode {
        public:
            virtual ~JumpStatementNode() = default;
        };

        class ReturnStatementNode : public JumpStatementNode {
            ExpressionNode* Expr;

        public:
            ReturnStatementNode(ExpressionNode* expr) : Expr(expr) {
            }

            ExpressionNode* getExpression() const { return Expr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Return(" << "\n";

                if (Expr) Expr->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class GotoStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            GotoStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class BreakStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            BreakStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Break(\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ContinueStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            ContinueStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Continue(\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
