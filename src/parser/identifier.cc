#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
    namespace parser {
        types::IdentifierNode* Parser::ParseIdentifier() {
            if (!Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            return ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(this->Advance().Value));
        }
    }
}
