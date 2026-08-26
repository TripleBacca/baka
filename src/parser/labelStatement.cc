#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::GotoLabelStatementNode* Parser::GotoLabelStatementNode() {
            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }

            types::IdentifierNode* Tag = this->Identifier();

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            types::StatementNode* Stmt = this->Statement();

            types::GotoLabelStatementNode* Node = ASTALLOC.Alloc<types::GotoLabelStatementNode>(Tag, Stmt);
            return Node;
        }

        types::CaseLabelStatementNode* Parser::CaseLabelStatementNode() {
            if (!this->Match(types::TokenType::K_CASE)) {
                // throw error
            }

            types::ExpressionNode* Tag = this->Expression();

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            types::StatementNode* Stmt = this->Statement();

            types::CaseLabelStatementNode* Node = ASTALLOC.Alloc<types::CaseLabelStatementNode>(Tag, Stmt);
            return Node;
        }

        types::DefaultLabelStatementNode* Parser::DefaultLabelStatementNode() {
            if (!this->Match(types::TokenType::K_DEFAULT)) {
                // throw error
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                // throw error
            }

            types::StatementNode* Stmt = this->Statement();

            types::DefaultLabelStatementNode* Node = ASTALLOC.Alloc<types::DefaultLabelStatementNode>(Stmt);
            return Node;
        }
    }
}
