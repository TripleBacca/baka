#pragma once
#include <string_view>


// todo: define all token types
enum class TokenType {
    KEY,
    LPAREN_ROUND,
    RPAREN_ROUND
};

struct TokenSourceLocation {
    size_t row;
    size_t col;
};


struct Token {
    TokenType token_type;
    std::string_view lexeme; // just a string bro
};
