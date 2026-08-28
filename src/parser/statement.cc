#include "parser.hh"
#include "types/token/token.hh"

namespace baka {
    namespace parser {
        types::StatementNode* Parser::ParseStatement() {
            //TODO implement proper statement logic
            if (Check(types::TokenType::K_IF) || Check(types::TokenType::K_SWITCH)) {
                return this->SelectionStatement();
            }
            else if (Check(types::TokenType::K_FOR) || Check(types::TokenType::K_WHILE) || Check(types::TokenType::K_DO)) {
                return this->IterationStatement();
            }
            else if ((Check(types::TokenType::IDENTIFIER) && Check2(types::TokenType::OP_COLON)) || Check(types::TokenType::K_CASE) || Check(types::TokenType::K_DEFAULT)) {
                return this->LabelStatement();
            }
            else if (Check(types::TokenType::K_GOTO) || Check(types::TokenType::K_RETURN) || Check(types::TokenType::K_BREAK) || Check(types::TokenType::K_CONTINUE)) {
                return this->JumpStatement();
            }
            else {
                return this->CompoundStatement();
            }
        }
    }
}
