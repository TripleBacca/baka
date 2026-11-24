#pragma once
#include "types/token/token.hh"
#include <unordered_map>

namespace baka {
namespace types {

    inline static std::unordered_map<TokenType, int> TokenTypeToPrecedence = {
        {TokenType::OP_DOT, 16},
        {TokenType::OP_ARROW, 16},
        {TokenType::OP_MUL, 13},
        {TokenType::OP_DIV, 13},
        {TokenType::OP_MOD, 13},
        {TokenType::OP_PLUS, 12},
        {TokenType::OP_UNARY_MINUS, 12}, // binary subtraction
        {TokenType::OP_LSHIFT, 11},
        {TokenType::OP_RSHIFT, 11},
        {TokenType::OP_LT, 10},
        {TokenType::OP_GT, 10},
        {TokenType::OP_LE, 10},
        {TokenType::OP_GE, 10},
        // Dynamic-class comparison binds tighter than ==/!=: its operands are never bool, so
        // `a == b ==== c` means `a == (b ==== c)`.
        {TokenType::OP_STRICT_EQ, 9},
        {TokenType::OP_STRICT_NEQ, 9},
        {TokenType::OP_EQ, 8},
        {TokenType::OP_NEQ, 8},
        {TokenType::OP_BIT_AND, 7},
        {TokenType::OP_BIT_XOR, 6},
        {TokenType::OP_BIT_OR, 5},
        {TokenType::OP_LOGICAL_AND, 4},
        {TokenType::OP_LOGICAL_OR, 3},
        {TokenType::OP_QUESTION, 1}, // ternary
        {TokenType::OP_ASSIGN, 1},
        {TokenType::OP_ADD_ASSIGN, 1},
        {TokenType::OP_SUB_ASSIGN, 1},
        {TokenType::OP_MUL_ASSIGN, 1},
        {TokenType::OP_DIV_ASSIGN, 1},
        {TokenType::OP_MOD_ASSIGN, 1},
        {TokenType::OP_AND_ASSIGN, 1},
        {TokenType::OP_OR_ASSIGN, 1},
        {TokenType::OP_XOR_ASSIGN, 1},
        {TokenType::OP_LSHIFT_ASSIGN, 1},
        {TokenType::OP_RSHIFT_ASSIGN, 1},
        {TokenType::OP_COMMA, 0},
    };


}
}
