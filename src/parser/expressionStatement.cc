#include "types/parser/ast/expression.hh"
#include "types/parser/ast/statement.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
namespace parser {

    types::ExpressionStatementNode* Parser::ParseExpressionStatement() {
        types::ExpressionNode* Expression = this->Expression();
        if(!Match(types::TokenType::SEMICOLON)) {
            // todo throw error
            assert(false);
        }

        types::ExpressionStatementNode* Node = ASTALLOC.Alloc<types::ExpressionStatementNode>(Expression);
        return Node;
    }
}
}
