#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        // Labels own the statement they prefix. The parser produces the attached body;
        // semantic checking decides whether the label placement is legal.
        types::GotoLabelStatementNode* Parser::GotoLabelStatement() {
            const size_t SourceStart = current;
            types::IdentifierNode* Tag = this->ParseIdentifier();

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after label", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::GotoLabelStatementNode>(SourceStart, Tag, Stmt);
            return Node;
        }

        types::CaseLabelStatementNode* Parser::CaseLabelStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CASE)) {
                // throw error
            }

            types::ExpressionNode* Tag = this->Expression();
            if (Tag == nullptr) {
                // the expression parser already diagnosed; resync to the ':'.
                SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after case value", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::CaseLabelStatementNode>(SourceStart, Tag, Stmt);
            return Node;
        }

        types::DefaultLabelStatementNode* Parser::DefaultLabelStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_DEFAULT)) {
                // throw error
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after 'default'", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::DefaultLabelStatementNode>(SourceStart, Stmt);
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
                return nullptr;
            }
        }
    }
}
