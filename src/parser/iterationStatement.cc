#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ForBlockStatementNode* Parser::ForBlockStatement() {
            if (!this->Match(types::TokenType::K_FOR)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Decl = nullptr;
            if (!Match(types::TokenType::SEMICOLON)) {
                Decl = this->Expression();
                if (!Match(types::TokenType::SEMICOLON)) {
                    // throw error
                }
            }
            types::ExpressionNode* Cond = nullptr;
            if (!Match(types::TokenType::SEMICOLON)) {
                Cond = this->Expression();
                if (!Match(types::TokenType::SEMICOLON)) {
                    // throw error
                }
            }
            types::ExpressionNode* Upd = nullptr;
            if (!Match(types::TokenType::SEMICOLON) && !Check(types::TokenType::RPAREN_ROUND)) {
                Upd = this->Expression();
                if (!Match(types::TokenType::SEMICOLON) && !Check(types::TokenType::RPAREN_ROUND)) {
                    // throw error
                }
            }
            types::IdentifierNode* Label = nullptr;
            if (Check(types::TokenType::IDENTIFIER)) {
                Label = this->Identifier();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();

            types::ForBlockStatementNode* Node = ASTALLOC.Alloc<types::ForBlockStatementNode>(Decl, Cond, Upd, Label, Body);
            return Node;
        }

        types::WhileBlockStatementNode* Parser::WhileBlockStatement() {
            if (!this->Match(types::TokenType::K_WHILE)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Cond = nullptr;
            Cond = this->Expression();
            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::SEMICOLON)) {
                if (Check(types::TokenType::IDENTIFIER)) {
                    Label = this->Identifier();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();

            types::WhileBlockStatementNode* Node = ASTALLOC.Alloc<types::WhileBlockStatementNode>(Cond, Label, Body);
            return Node;
        }

        types::DoWhileBlockStatementNode* Parser::DoWhileBlockStatement() {
            if (!this->Match(types::TokenType::K_DO)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();


            if (!this->Match(types::TokenType::K_WHILE)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Cond = nullptr;
            Cond = this->Expression();

            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::SEMICOLON)) {
                if (Check(types::TokenType::IDENTIFIER)) {
                    Label = this->Identifier();
                }
            }


            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            if (!Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::DoWhileBlockStatementNode* Node = ASTALLOC.Alloc<types::DoWhileBlockStatementNode>(Cond, Label, Body);
            return Node;
        }

        types::IterationStatementNode* Parser::IterationStatement() {
            if (this->Check(types::TokenType::K_FOR)) {
                return ForBlockStatement();
            }
            else if (this->Check(types::TokenType::K_WHILE)) {
                return WhileBlockStatement();
            }
            else if (this->Check(types::TokenType::K_DO)) {
                return DoWhileBlockStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
