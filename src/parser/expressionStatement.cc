#include "types/parser/ast/expression.hh"
#include "types/parser/ast/statement.hh"
#include "parser.hh"

namespace baka {
namespace parser {

    types::ExpressionStatementNode* Parser::ParseExpressionStatement() {
        types::ExpressionNode* Expression = this->Expression();
        return new types::ExpressionStatementNode(Expression);
    }
}
}
