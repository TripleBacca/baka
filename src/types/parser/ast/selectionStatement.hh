#pragma once
#include <iostream>
#include <utility>
#include <vector>

#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class SelectionStatementNode : public StatementNode {
        public:
            virtual ~SelectionStatementNode() = default;
        };

        class IfSubBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            IfSubBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "If(" << "\n";

                Expr->Print(Tabs + 1);

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ElseIfSubBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            ElseIfSubBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Else If(" << "\n";

                Expr->Print(Tabs + 1);

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ElseSubBlockStatementNode : public SelectionStatementNode {
            StatementNode* Body;

        public:
            ElseSubBlockStatementNode(StatementNode* body) : Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Else(" << "\n";

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class IfSuperBlockStatementNode : public SelectionStatementNode {
            IfSubBlockStatementNode* IfSubBlock;
            std::vector<ElseIfSubBlockStatementNode*> ElseIfSubBlocks;
            ElseSubBlockStatementNode* ElseSubBlock;

        public:
            IfSuperBlockStatementNode(IfSubBlockStatementNode* ifSubBlock, std::vector<ElseIfSubBlockStatementNode*> elseIfSubBlocks,
                                      ElseSubBlockStatementNode* elseSubBlock) : IfSubBlock(ifSubBlock), ElseIfSubBlocks(std::move(elseIfSubBlocks)), ElseSubBlock(elseSubBlock) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "If Super Block(" << "\n";

                IfSubBlock->Print(Tabs + 1);

                for (auto elseIfSubBlock : ElseIfSubBlocks) {
                    elseIfSubBlock->Print(Tabs + 1);
                }

                if (ElseSubBlock) ElseSubBlock->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class SwitchBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            SwitchBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Switch(" << "\n";

                Expr->Print(Tabs + 1);

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
