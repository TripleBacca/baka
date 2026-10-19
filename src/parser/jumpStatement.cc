#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ReturnStatementNode* Parser::ReturnStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_RETURN)) {
                // throw error
            }

            types::ExpressionNode* Expression = nullptr;
            if (!Check(types::TokenType::SEMICOLON)) {
                Expression = this->Expression();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after return statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::ReturnStatementNode* Node = AllocNodeAt<types::ReturnStatementNode>(SourceStart, Expression);
            return Node;
        }

        types::GotoStatementNode* Parser::GotoStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_GOTO)) {
                // throw error
            }

            types::IdentifierNode* Label = this->ParseIdentifier();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after goto statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::GotoStatementNode* Node = AllocNodeAt<types::GotoStatementNode>(SourceStart, Label);
            return Node;
        }

        types::BreakStatementNode* Parser::BreakStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_BREAK)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;

            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after break statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::BreakStatementNode* Node = AllocNodeAt<types::BreakStatementNode>(SourceStart, Label);
            return Node;
        }

        types::ContinueStatementNode* Parser::ContinueStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CONTINUE)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;
            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after continue statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::ContinueStatementNode* Node = AllocNodeAt<types::ContinueStatementNode>(SourceStart, Label);
            return Node;
        }

        types::JumpStatementNode* Parser::JumpStatement() {
            if (this->Check(types::TokenType::K_RETURN)) {
                return ReturnStatement();
            }
            else if (this->Check(types::TokenType::K_GOTO)) {
                return GotoStatement();
            }
            else if (this->Check(types::TokenType::K_BREAK)) {
                return BreakStatement();
            }
            else if (this->Check(types::TokenType::K_CONTINUE)) {
                return ContinueStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
