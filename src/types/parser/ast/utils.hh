#pragma once
#include "types/token/token.hh"
#include <unordered_map>

#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "

namespace baka {
    namespace types {

        enum class ASTUnaryOp {
            OP_UNARY_MINUS,
            OP_TILDE,
            OP_NOOP
        };

        inline static std::unordered_map<TokenType, ASTUnaryOp> TokenTypeToASTUnaryOp = {
            {TokenType::OP_UNARY_MINUS, ASTUnaryOp::OP_UNARY_MINUS},
            {TokenType::OP_TILDE, ASTUnaryOp::OP_TILDE},
        };

    }
}
