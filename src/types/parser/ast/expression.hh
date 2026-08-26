#pragma once
#include "ast_node.hh"
#include "constant.hh"
#include <variant>
#include "identifier.hh"
#include "utils.hh"
#include "operators.hh"
#include <vector>
#include <iostream>


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
        std::variant<ConstantNode*, IdentifierNode*> Expr;

        public:

        PrimaryExpressionNode(std::variant<ConstantNode*, IdentifierNode*> Expr) : Expr(Expr) {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "PrimaryExpression(" << std::endl;

            if(std::holds_alternative<ConstantNode*>(Expr)) {
                std::get<ConstantNode*>(Expr)->Print(Tabs + 1);
            } else {
                std::get<IdentifierNode*>(Expr)->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };


    class FactorNode : public ExpressionNode {
        ASTUnaryOp UnaryOp;
        ExpressionNode* Expr;
        public:
            FactorNode(ASTUnaryOp UnaryOp, ExpressionNode* Expr)
                : UnaryOp(UnaryOp), Expr(Expr) {}


            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Factor(" << std::endl;

                INDENT(Tabs + 1);
                std::cout << "UnaryOp: " << ASTUnaryOpToString(UnaryOp) << ",\n";

                Expr->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    class BinaryExpressionNode : public ExpressionNode {
        ASTBinaryOp BinaryOp;
        ExpressionNode* LHSExpr;
        ExpressionNode* RHSExpr;
        public:
            BinaryExpressionNode(ASTBinaryOp BinaryOp, ExpressionNode* LHSExpr, ExpressionNode* RHSExpr)
                : BinaryOp(BinaryOp), LHSExpr(LHSExpr), RHSExpr(RHSExpr) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "BinaryExpression(" << std::endl;

                INDENT(Tabs + 1);
                std::cout << "BinaryOp: " << ASTBinaryOpToString(BinaryOp) << ",\n";

                LHSExpr->Print(Tabs + 1);
                RHSExpr->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    class CommaExpressionNode : public ExpressionNode {
        std::vector<ExpressionNode*> ExpressionList;
        public:
            CommaExpressionNode(std::vector<ExpressionNode*> ExpressionList)
                : ExpressionList(std::move(ExpressionList)) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "CommaExpression(" << std::endl;

                for(auto* expr : ExpressionList) {
                    expr->Print(Tabs + 1);
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // POSTFIX EXPRESSION NODES: . , ->, [] , (),


    class IndexPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        ExpressionNode* Index;
        public:
            IndexPostfixExpr(ExpressionNode* LHSExpr, ExpressionNode* Index)
                : LHSExpr(LHSExpr), Index(Index) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "IndexPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                Index->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    // for ++ and -- postfix:
    class UnaryPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        ASTUnaryOp Op;
        public:
            UnaryPostfixExpr(ExpressionNode* LHSExpr, ASTUnaryOp Op)
                : LHSExpr(LHSExpr), Op(Op) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "UnaryPostfixExpr(" << '\n';

                LHSExpr->Print(Tabs + 1);

                INDENT(Tabs + 1);
                std::cout << ASTUnaryOpToString(Op) << '\n';

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // for . postfix expr
    class MemberPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        IdentifierNode* MemberName;
        public:
            MemberPostfixExpr(ExpressionNode* LHSExpr, IdentifierNode* MemberName)
                : LHSExpr(LHSExpr), MemberName(MemberName) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "MemberPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                MemberName->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // for -> postfix expr
    class ArrowPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        IdentifierNode* MemberName;
        public:
            ArrowPostfixExpr(ExpressionNode* LHSExpr, IdentifierNode* MemberName)
                : LHSExpr(LHSExpr), MemberName(MemberName) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "ArrowPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                MemberName->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    class FunctionCallPostfixExpr :  public PostfixExpressionNode {
        ExpressionNode* FunctionIdentifier;
        ExpressionNode* ArgumentList;

        public:
            FunctionCallPostfixExpr(ExpressionNode* FunctionIdentifier, ExpressionNode* ArgumentList)
                : FunctionIdentifier(FunctionIdentifier), ArgumentList(ArgumentList) {}

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "FunctionCallPostfixExpr(" << std::endl;

                FunctionIdentifier->Print(Tabs + 1);
                ArgumentList->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

}
}
