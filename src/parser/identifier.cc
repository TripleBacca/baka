#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
    namespace parser {
        types::IdentifierNode* Parser::Identifier() {
            if (!Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            return ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(this->Advance().Value));
        }
    }
}
