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
        // TODO: fill this shit up
        switch (Type) {
            case types::TokenType::OP_UNARY_MINUS:
            case types::TokenType::OP_TILDE:
                return true;
            default:
                return false;
        }
    }
}
}
}
