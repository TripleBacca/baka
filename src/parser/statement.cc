#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::StatementNode* Parser::Statement() {
            //TODO implement proper statement logic
            if (Check(types::TokenType::K_IF)) {
                return this->SelectionStatement();
            }
            else {
                return this->JumpStatement();
            }
        }
    }
}
