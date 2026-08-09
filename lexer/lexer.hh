#pragma once
#include "../types/token/all.hh"
#include <vector>

namespace baka {
namespace lexer {

std::vector<types::Token> Tokenize(std::string_view SourceCode);


} // namespace lexer
} // namespace baka
