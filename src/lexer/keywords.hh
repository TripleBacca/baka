#pragma once

#include <string_view>
#include <unordered_map>
#include "types/token/all.hh"

namespace baka {
    namespace lexer {
        const inline std::unordered_map<std::string_view, types::TokenType> Kw_lexeme_to_type = {
            {"bool", types::TokenType::K_BOOL},
            {"break", types::TokenType::K_BREAK},
            {"case", types::TokenType::K_CASE},
            {"char", types::TokenType::K_CHAR},
            {"class", types::TokenType::K_CLASS},
            {"const", types::TokenType::K_CONST},
            {"continue", types::TokenType::K_CONTINUE},
            {"default", types::TokenType::K_DEFAULT},
            {"delete", types::TokenType::K_DELETE},
            {"do", types::TokenType::K_DO},
            {"double", types::TokenType::K_DOUBLE},
            {"else", types::TokenType::K_ELSE},
            {"enum", types::TokenType::K_ENUM},
            {"false", types::TokenType::K_FALSE},
            {"float", types::TokenType::K_FLOAT},
            {"for", types::TokenType::K_FOR},
            {"goto", types::TokenType::K_GOTO},
            {"if", types::TokenType::K_IF},
            {"int", types::TokenType::K_INT},
            {"long", types::TokenType::K_LONG},
            {"namespace", types::TokenType::K_NAMESPACE},
            {"new", types::TokenType::K_NEW},
            {"nullptr", types::TokenType::K_NULLPTR},
            {"return", types::TokenType::K_RETURN},
            {"short", types::TokenType::K_SHORT},
            {"signed", types::TokenType::K_SIGNED},
            {"sizeof", types::TokenType::K_SIZEOF},
            {"static", types::TokenType::K_STATIC},
            {"struct", types::TokenType::K_STRUCT},
            {"switch", types::TokenType::K_SWITCH},
            {"this", types::TokenType::K_THIS},
            {"true", types::TokenType::K_TRUE},
            {"typedef", types::TokenType::K_TYPEDEF},
            {"union", types::TokenType::K_UNION},
            {"unsigned", types::TokenType::K_UNSIGNED},
            {"using", types::TokenType::K_USING},
            {"void", types::TokenType::K_VOID},
            {"while", types::TokenType::K_WHILE},
        };
    }
}
