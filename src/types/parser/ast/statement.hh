#pragma once
#include "ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/typedef.hh"
#include <variant>
#include <vector>


namespace baka {
namespace types {

    class StructDefinitionNode;
    class StructDeclarationNode;
    class ClassDefinitionNode;
    class ClassDeclarationNode;
    class UnionNode;
    class EnumNode;

    enum class StatementNodeType {
        CompoundStatement,
        JumpStatement,
        LabeledStatement,
        IterationStatement,
        SelectionStatement,
        ExpressionStatement,
    };

    class StatementNode : public ASTNode {
    public:
        virtual ~StatementNode() = default;
    };

    class CompoundStatementNode : public StatementNode
    {
    public:
        void visitChildren(const NodeVisitor& visit) const override;
        using StatementVariant = std::variant<StatementNode*, DeclarationList*, TypedefNode*, FunctionNode*,
                                              StructDefinitionNode*, StructDeclarationNode*, ClassDefinitionNode*,
                                              ClassDeclarationNode*, UnionNode*, EnumNode*>;
        using StatementList = std::vector<StatementVariant>;

    private:
        StatementList Statements;

    public:
        CompoundStatementNode(StatementList Statements) : Statements(std::move(Statements)) {}

        const StatementList& getStatements() const { return Statements; }

        void addStatement(StatementVariant Statement) { Statements.push_back(Statement); }

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "CompoundStatement(" << std::endl;

            for (const auto& Statement : Statements) {
                std::visit([&](auto* node) {
                    if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
                }, Statement);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

    class EmptyStatementNode : public StatementNode {
    public:
        void visitChildren(const NodeVisitor& visit) const override {  }
        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "EmptyStatement()" << std::endl;
        }
    };

    class ExpressionStatementNode : public StatementNode {
        ExpressionNode* Expression; // can be nullptr
        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getExpression()); }
            ExpressionStatementNode(ExpressionNode* Expression) : Expression(Expression) {}
            ExpressionNode* getExpression() const { return Expression; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expression = rewrite(Expression); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Expression); }

            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "ExpressionStatement(" << std::endl;

                if (Expression)
                    Expression->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


}
}
