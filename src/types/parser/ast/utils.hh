#pragma once
#include "types/token/token.hh"
#include <string_view>
#include <unordered_map>

#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "

namespace baka {
    namespace types {

        enum class ASTUnaryOp {
            OP_UNARY_MINUS,
            OP_TILDE,
            OP_NOOP
        };

        inline std::string_view ASTUnaryOpToString(ASTUnaryOp op) {
            switch (op) {
                case ASTUnaryOp::OP_UNARY_MINUS: return "OP_UNARY_MINUS";
                case ASTUnaryOp::OP_TILDE: return "OP_TILDE";
                case ASTUnaryOp::OP_NOOP: return "OP_NOOP";
            }
            return "";
        }

        inline static std::unordered_map<TokenType, ASTUnaryOp> TokenTypeToASTUnaryOp = {
            {TokenType::OP_UNARY_MINUS, ASTUnaryOp::OP_UNARY_MINUS},
            {TokenType::OP_TILDE, ASTUnaryOp::OP_TILDE},
        };

    }
}
