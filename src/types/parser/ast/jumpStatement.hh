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

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Return(" << "\n";

                if (Expr) Expr->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class GotoStatementNode : public JumpStatementNode {
            IdentiferNode* Label;

        public:
            GotoStatementNode(IdentiferNode* label) : Label(label) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto(@, " << "\n";

                Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class BreakStatementNode : public JumpStatementNode {
            IdentiferNode* Label;

        public:
            BreakStatementNode(IdentiferNode* label) : Label(label) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Break(";
                if (Label) std::cout << "@, ";
                std::cout << "\n";

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ContinueStatementNode : public JumpStatementNode {
            IdentiferNode* Label;

        public:
            ContinueStatementNode(IdentiferNode* label) : Label(label) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Continue(";
                if (Label) std::cout << "@, ";
                std::cout << "\n";

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
