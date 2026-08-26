#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::IfSubBlockStatementNode* Parser::IfSubBlockStatement() {
            if (!this->Match(types::TokenType::K_IF)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();

            types::IfSubBlockStatementNode* Node = ASTALLOC.Alloc<types::IfSubBlockStatementNode>(Expression, Body);
            return Node;
        }

        types::ElseIfSubBlockStatementNode* Parser::ElseIfSubBlockStatement() {
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

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();

            types::ElseIfSubBlockStatementNode* Node = ASTALLOC.Alloc<types::ElseIfSubBlockStatementNode>(Expression, Body);
            return Node;
        }

        types::ElseSubBlockStatementNode* Parser::ElseSubBlockStatement() {
            if (!this->Match(types::TokenType::K_ELSE)) {
                // throw error
            }

            types::StatementNode* Body = this->Statement();

            types::ElseSubBlockStatementNode* Node = ASTALLOC.Alloc<types::ElseSubBlockStatementNode>( Body);
            return Node;
        }

        types::IfSuperBlockStatementNode* Parser::IfSuperBlockStatement() {
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

            types::IfSuperBlockStatementNode* Node = ASTALLOC.Alloc<types::IfSuperBlockStatementNode>(IfSubBlock, ElseIfSubBlocks, ElseSubBlock);
            return Node;
        }

        types::SelectionStatementNode* Parser::SelectionStatement() {
            if (this->Check(types::TokenType::K_IF)) {
                return IfSuperBlockStatement();
            }
            //TODO add support for switch case
            /*else if (this->Check(types::TokenType::K_SWITCH)) {
                return SwitchStatement();
            }*/
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
