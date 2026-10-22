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
        void visitChildren(const NodeVisitor& visit) const override { visit(getTag()); visit(getStatement()); }
            GotoLabelStatementNode(IdentifierNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            IdentifierNode* getTag() const { return Tag; }
            StatementNode* getStatement() const { return Stmt; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto Label(";
                if (Tag) Tag->Print();
                std::cout << ":\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

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
        void visitChildren(const NodeVisitor& visit) const override { visit(getTag()); visit(getStatement()); }
            CaseLabelStatementNode(ExpressionNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            ExpressionNode* getTag() const { return Tag; }
            StatementNode* getStatement() const { return Stmt; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Tag = rewrite(Tag); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Tag); }

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
        void visitChildren(const NodeVisitor& visit) const override { visit(getStatement()); }
            DefaultLabelStatementNode(StatementNode* stmt) : Stmt(stmt) {
            }

            StatementNode* getStatement() const { return Stmt; }

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
