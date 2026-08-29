#pragma once
#include "ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typedef.hh"
#include <variant>
#include <vector>


namespace baka {
namespace types {

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
        std::vector<std::variant<StatementNode*, DeclarationList*, TypedefNode*>> Statements;
    public:
        CompoundStatementNode(std::vector<std::variant<StatementNode*, DeclarationList*, TypedefNode*>> Statements) : Statements(std::move(Statements)) {}

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "CompoundStatement(" << std::endl;

            for (const auto& Statement : Statements) {
                if (std::holds_alternative<DeclarationList*>(Statement))
                    std::get<DeclarationList*>(Statement)->Print(Tabs + 1);
                else
                    std::get<StatementNode*>(Statement)->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

    class ExpressionStatementNode : public StatementNode {
        ExpressionNode* Expression; // can be nullptr
        public:
            ExpressionStatementNode(ExpressionNode* Expression) : Expression(Expression) {}
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
