#pragma once
#include "types/token/all.hh"
#include "types/token/token.hh"

#define ASTALLOC baka::base::ASTNodeArena::getInstance()

namespace baka {
namespace parser {
namespace detail {

    inline bool isConstantToken(types::TokenType TokenType_v) {
        switch (TokenType_v) {
            case types::TokenType::LITERAL_CHARACTER:
            case types::TokenType::LITERAL_FP:
            case types::TokenType::LITERAL_STRING:
            case types::TokenType::LITERAL_INTEGER:
                return true;
            default:
                return false;
        }
    }

    inline bool IsUnaryOperator(types::TokenType Type) {
        switch (Type) {
            case types::TokenType::OP_UNARY_MINUS:   // unary -
            case types::TokenType::OP_PLUS:           // unary +
            case types::TokenType::OP_TILDE:          // bitwise NOT ~
            case types::TokenType::OP_LOGICAL_NOT:    // logical NOT !
            case types::TokenType::OP_INC:            // prefix ++
            case types::TokenType::OP_DEC:            // prefix --
            case types::TokenType::OP_MUL:            // dereference *
            case types::TokenType::OP_BIT_AND:        // address-of &
            case types::TokenType::K_SIZEOF:          // sizeof
                return true;
            default:
                return false;
        }
    }
}
}
}
