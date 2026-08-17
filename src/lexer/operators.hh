#pragma once

#include <string_view>
#include <unordered_map>
#include "types/token/all.hh"


namespace baka::lexer {
    inline const std::unordered_map<std::string_view, types::TokenType> lexeme_to_operator = {
        {"~", types::TokenType::OP_TILDE},
        {"-", types::TokenType::OP_UNARY_MINUS},
        {"++", types::TokenType::OP_INC},
        {"--", types::TokenType::OP_DEC},
        {"*", types::TokenType::OP_MUL},
        {"/", types::TokenType::OP_DIV},
        {"%", types::TokenType::OP_MOD},
        {"&", types::TokenType::OP_BIT_AND},
        {"|", types::TokenType::OP_BIT_OR},
        {"^", types::TokenType::OP_BIT_XOR},
        {"<<", types::TokenType::OP_LSHIFT},
        {">>", types::TokenType::OP_RSHIFT},
        {"&&", types::TokenType::OP_LOGICAL_AND},
        {"||", types::TokenType::OP_LOGICAL_OR},
        {"==", types::TokenType::OP_EQ},
        {"!=", types::TokenType::OP_NEQ},
        {">", types::TokenType::OP_GT},
        {"<", types::TokenType::OP_LT},
        {">=", types::TokenType::OP_GE},
        {"<=", types::TokenType::OP_LE},
        {"+", types::TokenType::OP_PLUS},
        {"!", types::TokenType::OP_LOGICAL_NOT},
        {"=", types::TokenType::OP_ASSIGN},
        {"+=", types::TokenType::OP_ADD_ASSIGN},
        {"-=", types::TokenType::OP_SUB_ASSIGN},
        {"*=", types::TokenType::OP_MUL_ASSIGN},
        {"/=", types::TokenType::OP_DIV_ASSIGN},
        {"%=", types::TokenType::OP_MOD_ASSIGN},
        {"&=", types::TokenType::OP_AND_ASSIGN},
        {"|=", types::TokenType::OP_OR_ASSIGN},
        {"^=", types::TokenType::OP_XOR_ASSIGN},
        {"<<=", types::TokenType::OP_LSHIFT_ASSIGN},
        {">>=", types::TokenType::OP_RSHIFT_ASSIGN},
        {":", types::TokenType::OP_COLON},
        {"::", types::TokenType::OP_DOUBLE_COLON},
        {",", types::TokenType::OP_COMMA},
        {"?", types::TokenType::OP_QUESTION},
        {".", types::TokenType::OP_DOT},
        {"->", types::TokenType::OP_ARROW},
        {"...", types::TokenType::OP_ELLIPSIS}
    };
}
