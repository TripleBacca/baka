#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::IfSubBlockStatementNode* Parser::IfSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_IF)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                // condition failed; error already reported deep. resync to the closing
                // paren or a semicolon so the offending tokens aren't re-parsed as a body
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            types::IfSubBlockStatementNode* Node = AllocNodeAt<types::IfSubBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::ElseIfSubBlockStatementNode* Parser::ElseIfSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_ELSE)) {
                // throw error
            }
            if (!this->Match(types::TokenType::K_IF)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ElseIfSubBlockStatementNode* Node = AllocNodeAt<types::ElseIfSubBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::ElseSubBlockStatementNode* Parser::ElseSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_ELSE)) {
                // throw error
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ElseSubBlockStatementNode* Node = AllocNodeAt<types::ElseSubBlockStatementNode>(SourceStart,  Body);
            return Node;
        }

        types::IfSuperBlockStatementNode* Parser::IfSuperBlockStatement() {
            const size_t SourceStart = current;
            if (!Check(types::TokenType::K_IF)) {
                // throw error
            }
            types::IfSubBlockStatementNode* IfSubBlock = this->IfSubBlockStatement();

            std::vector<types::ElseIfSubBlockStatementNode*> ElseIfSubBlocks;
            while (Check(types::TokenType::K_ELSE) && Check2(types::TokenType::K_IF)) {
                ElseIfSubBlocks.push_back(this->ElseIfSubBlockStatement());
            }

            types::ElseSubBlockStatementNode* ElseSubBlock = nullptr;
            if (Check(types::TokenType::K_ELSE) && !Check2(types::TokenType::K_IF)) {
                ElseSubBlock = this->ElseSubBlockStatement();
            }

            types::IfSuperBlockStatementNode* Node = AllocNodeAt<types::IfSuperBlockStatementNode>(SourceStart, IfSubBlock, ElseIfSubBlocks, ElseSubBlock);
            return Node;
        }

        types::SwitchBlockStatementNode* Parser::SwitchBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_SWITCH)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            // A switch body is any statement; braces are the common case, not a grammar rule.
            types::StatementNode* Body = this->ParseStatement();

            types::SwitchBlockStatementNode* Node = AllocNodeAt<types::SwitchBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::SelectionStatementNode* Parser::SelectionStatement() {
            if (this->Check(types::TokenType::K_IF)) {
                return IfSuperBlockStatement();
            }
            else if (this->Check(types::TokenType::K_SWITCH)) {
                return SwitchBlockStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
