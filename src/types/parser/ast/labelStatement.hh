#pragma once
#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class LabelStatementNode : public StatementNode {
        public:
            virtual ~LabelStatementNode() = default;
        };

        class GotoLabelStatementNode : public LabelStatementNode {
            IdentifierNode* Tag;
            StatementNode* Stmt;

        public:
            GotoLabelStatementNode(IdentifierNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto Label(";
                if (Tag) Tag->Print();
                std::cout << ":\n";

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class CaseLabelStatementNode : public LabelStatementNode {
            //TODO check this
            ExpressionNode* Tag;
            StatementNode* Stmt;

        public:
            CaseLabelStatementNode(ExpressionNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Case Label(";
                if (Tag) Tag->Print();
                std::cout << ":\n";

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class DefaultLabelStatementNode : public LabelStatementNode {
            StatementNode* Stmt;

        public:
            DefaultLabelStatementNode(StatementNode* stmt) : Stmt(stmt) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Default Label(:\n";

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
