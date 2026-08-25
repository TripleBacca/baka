#pragma once

#include "types/token/token.hh"
#include <iostream>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace baka {
    namespace types {
        inline void Indent(size_t Tabs) {
            for (size_t i = 0; i < Tabs; i++) std::cout << "    ";
        }

        class ASTNode {
        public:
            ASTNode() = default;
            virtual ~ASTNode() = default;

            virtual void Print(size_t Tabs = 0) const = 0;
        };

        class StatementNode : public ASTNode {
        public:
            virtual ~StatementNode() = default;
        };

        class ExpressionNode : public ASTNode {
        public:
            virtual ~ExpressionNode() = default;
        };


        class ConstantNode : public ExpressionNode {
            int Value;

        public:
            ConstantNode(int val) : Value(val) {
            }

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Constant(" << Value << ")" << std::endl;
            }
        };


        enum class ASTUnaryOp {
            OP_UNARY_MINUS,
            OP_TILDE,
            OP_NOOP
        };

        inline static std::unordered_map<TokenType, ASTUnaryOp> TokenTypeToASTUnaryOp = {
            {TokenType::OP_UNARY_MINUS, ASTUnaryOp::OP_UNARY_MINUS},
            {TokenType::OP_TILDE, ASTUnaryOp::OP_TILDE},
        };

        inline std::string_view UnaryOpToStr(ASTUnaryOp op) {
            switch (op) {
            case ASTUnaryOp::OP_UNARY_MINUS: return "-";
            case ASTUnaryOp::OP_TILDE: return "~";
            case ASTUnaryOp::OP_NOOP: return "NOOP";
            default: return "?";
            }
        }

        class UnaryExpressionNode : public ExpressionNode {
            ASTUnaryOp Op;
            ExpressionNode* Expr;

        public:
            UnaryExpressionNode(ASTUnaryOp op, ExpressionNode* expr) : Op(op), Expr(expr) {
            }

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Unary(" << UnaryOpToStr(Op) << ',' << "\n";

                if (Expr) Expr->Print(Tabs + 1);

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class StructDeclarationStatementNode : public StatementNode {
            std::string_view DataType;
            std::string_view VariableName;

        public:
            StructDeclarationStatementNode(std::string_view dataType, std::string_view variableName) :
                DataType(dataType), VariableName(variableName) {
            }

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "StructDeclaration(" << DataType << ", " << '\n';

                Indent(Tabs + 1);
                std::cout << VariableName << '\n';

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ReturnStatementNode : public StatementNode {
            ExpressionNode* Expr;

        public:
            ReturnStatementNode(ExpressionNode* expr) : Expr(expr) {
            }

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Return(" << "\n";

                if (Expr) Expr->Print(Tabs + 1);

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class StructNode : public ASTNode {
            std::string_view StructName;
            std::vector<StructDeclarationStatementNode*> Body;

        public:
            StructNode(std::string_view identifier,
                       std::vector<StructDeclarationStatementNode*> body) : StructName(identifier),
                                                                            Body(std::move(body)) {
            }

            ~StructNode() = default;

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Struct(" << StructName << ", " << '\n';

                for (auto decl : Body) {
                    decl->Print(Tabs + 1);
                }

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class FunctionNode : public ASTNode {
            std::string_view ReturnType;
            std::string_view FuncName;
            StatementNode* Body;

        public:
            FunctionNode(std::string_view returnType, std::string_view funcName, StatementNode* body) :
                ReturnType(returnType), FuncName(funcName), Body(body) {
            }

            ~FunctionNode() = default;

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Function(" << ReturnType << ", " << FuncName << ", " << '\n';

                Body->Print(Tabs + 1);

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class ProgramNode : public ASTNode {
            std::vector<ASTNode*> Body;

        public:
            ProgramNode(std::vector<ASTNode*> body) : Body(std::move(body)) {
            }

            ~ProgramNode() = default;

            void Print(size_t Tabs = 0) const override {
                Indent(Tabs);
                std::cout << "Program(" << '\n';
                for (const auto tlBlock : Body) {
                    tlBlock->Print(Tabs + 1);
                }

                Indent(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
