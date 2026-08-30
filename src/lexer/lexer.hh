#pragma once
#include "types/token/all.hh"
#include <vector>

namespace baka {
    namespace lexer {
        struct Lexed {
            std::vector<types::Token> Tokens;
            std::vector<types::TokenSourceLocation> Locations;
        };

        Lexed Tokenize(std::string_view SourceCode);
    } // namespace lexer
} // namespace baka
