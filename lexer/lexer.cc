#include "../types/token.hh"
#include <bits/stdc++.h>
#include <regex>

namespace baka {
namespace lexer {


    std::vector<Token> Tokenize(const std::string& source_code) {
        std::vector<Token> tokens;

        size_t idx = 0;

        while(idx < source_code.size()) {
            char c = source_code[c];

            if (c == '(') {
                tokens.emplace_back(TokenType::LPAREN_ROUND, source_code.substr(idx, 1));
                break;
            } else if (c == ')') {
                tokens.emplace_back(TokenType::RPAREN_ROUND, source_code.substr(idx, 1));
                break;
            }
    }


}


}
