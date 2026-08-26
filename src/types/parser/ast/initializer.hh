#pragma once
#include "types/parser/ast/expression.hh"
#include <vector>

namespace baka {
namespace types {

    class InitializerNode : public ExpressionNode {
        std::vector<baka::types::ExpressionNode*> _expressions; // bunch of assignment exprs

    public:
        InitializerNode() = default;
        InitializerNode(std::vector<baka::types::ExpressionNode*> expressions) : _expressions(std::move(expressions)) {}
        const std::vector<baka::types::ExpressionNode*>& expressions() const { return _expressions; }
        void addExpression(baka::types::ExpressionNode* expression) { _expressions.push_back(expression); }


        void Print(size_t Tabs = 0) const override
        {
            INDENT(Tabs);
            std::cout << "InitializerNode(" << '\n';
            INDENT(Tabs + 1);
            std::cout << "Expressions: " << _expressions.size() << std::endl;
            for (const auto& Expression : _expressions)
            {
                Expression->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }

    };
}
}
