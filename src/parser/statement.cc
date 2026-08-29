#include "types/parser/ast/statement.hh"
#include "parser.hh"
#include "types/token/token.hh"

namespace baka {
    namespace parser {
        types::StatementNodeType Parser::DetermineStatementType() {
            if (Check(types::TokenType::K_IF) || Check(types::TokenType::K_SWITCH)) {
                return types::StatementNodeType::SelectionStatement;
            }
            else if (Check(types::TokenType::K_FOR) || Check(types::TokenType::K_WHILE) || Check(types::TokenType::K_DO)) {
                return types::StatementNodeType::IterationStatement;
            }
            else if ((Check(types::TokenType::IDENTIFIER) && Check2(types::TokenType::OP_COLON)) || Check(types::TokenType::K_CASE) || Check(types::TokenType::K_DEFAULT)) {
                return types::StatementNodeType::LabeledStatement;
            }
            else if (Check(types::TokenType::K_GOTO) || Check(types::TokenType::K_RETURN) || Check(types::TokenType::K_BREAK) || Check(types::TokenType::K_CONTINUE)) {
                return types::StatementNodeType::JumpStatement;
            } else if(Check(types::TokenType::LPAREN_CURLY)) {
                return types::StatementNodeType::CompoundStatement;
            } else {
                return types::StatementNodeType::ExpressionStatement;
            }
        }



        types::StatementNode* Parser::ParseStatement() {
            switch(DetermineStatementType()) {
                case types::StatementNodeType::SelectionStatement:
                    return this->SelectionStatement();
                case types::StatementNodeType::IterationStatement:
                    return this->IterationStatement();
                case types::StatementNodeType::LabeledStatement:
                    return this->LabelStatement();
                case types::StatementNodeType::JumpStatement:
                    return this->JumpStatement();
                case types::StatementNodeType::CompoundStatement:
                    return this->CompoundStatement();
                case types::StatementNodeType::ExpressionStatement:
                    return this->ParseExpressionStatement();
                default:
                    return nullptr;
            }
        }
    }
}
