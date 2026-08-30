#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
    namespace parser {
        types::IdentifierNode* Parser::ParseIdentifier() {
            if (!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier");
                if (!Check(types::TokenType::EOF_TOKEN)) {
                    Advance();
                }
                auto* Errored = ASTALLOC.Alloc<types::IdentifierNode>(std::string_view("?"));
                Errored->setHasError();
                return Errored;
            }
            return ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(this->Advance().Value));
        }
    }
}
