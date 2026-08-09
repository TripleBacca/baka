#pragma once
#include "../types/token.hh"
#include <vector>
#include <string>

namespace baka {
namespace lexer {

std::vector<types::Token> Tokenize(const std::string& SourceCode);

} // namespace lexer
} // namespace baka
