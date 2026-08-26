#pragma once
#include "ast_node.hh"



namespace baka {
namespace types {

    class StatementNode : public ASTNode {
    public:
        virtual ~StatementNode() = default;
    };

    class CompoundStatementNode : public StatementNode
    {
        std::vector<std::variant<StatementNode*, DeclarationList*>> Statements;
    public:
        CompoundStatementNode(std::vector<std::variant<StatementNode*, DeclarationList*>> Statements) : Statements(std::move(Statements)) {}

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

}
}
