#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ReturnStatementNode* Parser::ReturnStatement() {
            if (!this->Match(types::TokenType::K_RETURN)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::ReturnStatementNode* Node = ASTALLOC.Alloc<types::ReturnStatementNode>(Expression);
            return Node;
        }

        types::GotoStatementNode* Parser::GotoStatement() {
            if (!this->Match(types::TokenType::K_GOTO)) {
                // throw error
            }

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }

            types::IdentifierNode* Label = this->Identifier();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::GotoStatementNode* Node = ASTALLOC.Alloc<types::GotoStatementNode>(Label);
            return Node;
        }

        types::BreakStatementNode* Parser::BreakStatement() {
            if (!this->Match(types::TokenType::K_BREAK)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;

            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->Identifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::BreakStatementNode* Node = ASTALLOC.Alloc<types::BreakStatementNode>(Label);
            return Node;
        }

        types::ContinueStatementNode* Parser::ContinueStatement() {
            if (!this->Match(types::TokenType::K_CONTINUE)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;
            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->Identifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::ContinueStatementNode* Node = ASTALLOC.Alloc<types::ContinueStatementNode>(Label);
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
