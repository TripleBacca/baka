#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::GotoLabelStatementNode* Parser::GotoLabelStatement() {
            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }

            types::IdentifierNode* Tag = this->ParseIdentifier();

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            // types::StatementNode* Stmt = this->Statement();
            types::StatementNode* Stmt = nullptr;

            types::GotoLabelStatementNode* Node = ASTALLOC.Alloc<types::GotoLabelStatementNode>(Tag, Stmt);
            return Node;
        }

        types::CaseLabelStatementNode* Parser::CaseLabelStatement() {
            if (!this->Match(types::TokenType::K_CASE)) {
                // throw error
            }

            types::ExpressionNode* Tag = this->Expression();

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            // types::StatementNode* Stmt = this->Statement();
            types::StatementNode* Stmt = nullptr;

            types::CaseLabelStatementNode* Node = ASTALLOC.Alloc<types::CaseLabelStatementNode>(Tag, Stmt);
            return Node;
        }

        types::DefaultLabelStatementNode* Parser::DefaultLabelStatement() {
            if (!this->Match(types::TokenType::K_DEFAULT)) {
                // throw error
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            types::StatementNode* Stmt = nullptr;
            // types::StatementNode* Stmt = this->Statement();

            types::DefaultLabelStatementNode* Node = ASTALLOC.Alloc<types::DefaultLabelStatementNode>(Stmt);
            return Node;
        }

        types::LabelStatementNode* Parser::LabelStatement() {
            if (this->Check(types::TokenType::IDENTIFIER) && this->Check2(types::TokenType::OP_COLON)) {
                return this->GotoLabelStatement();
            }
            else if (this->Check(types::TokenType::K_CASE)) {
                return this->CaseLabelStatement();
            }
            else if (this->Check(types::TokenType::K_DEFAULT)) {
                return this->DefaultLabelStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
