#include "types/parser/ast/expression.hh"
#include "types/parser/ast/statement.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
namespace parser {

    types::ExpressionStatementNode* Parser::ParseExpressionStatement() {
        const size_t SourceStart = current;
        types::ExpressionNode* Expression = this->Expression();
        if (!Match(types::TokenType::SEMICOLON)) {
            if (Expression != nullptr) {
                ReportError("expected ';' after expression", true);
            }
            // expression error was already reported deeper; resync to the end of the doomed statement
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        types::ExpressionStatementNode* Node = AllocNodeAt<types::ExpressionStatementNode>(SourceStart, Expression);
        if (Expression == nullptr) {
            Node->setHasError();
        }
        return Node;
    }
}
}
