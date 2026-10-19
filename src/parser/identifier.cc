#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
    namespace parser {
        types::IdentifierNode* Parser::ParseIdentifier() {
            const size_t SourceStart = current;
            if (!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier");
                if (!Check(types::TokenType::EOF_TOKEN)) {
                    Advance();
                }
                auto* Errored = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?"));
                Errored->setHasError();
                return Errored;
            }
            return AllocNodeAt<types::IdentifierNode>(SourceStart, std::get<std::string_view>(this->Advance().Value));
        }
    }
}
