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

        //TODO: Fill this with the right precedence numbers (except comma)
        inline static std::unordered_map<TokenType, int> TokenTypeToPrecedence = {
            {TokenType::OP_MUL, 3},
            {TokenType::OP_DIV, 3},
            {TokenType::OP_MOD, 3},
            {TokenType::OP_LSHIFT, 1},
            {TokenType::OP_RSHIFT, 1},
            {TokenType::OP_BIT_AND, 0},
            {TokenType::OP_BIT_XOR, -1},
            {TokenType::OP_BIT_OR, -2},
            {TokenType::OP_ASSIGN, -3},
        };

        //TODO: FIll this shit up with all nigas except comma
        inline bool IsBinaryOperator(TokenType type)
        {
            switch (type) {
            case TokenType::OP_MUL:
            case TokenType::OP_DIV:
            case TokenType::OP_MOD:
            case TokenType::OP_BIT_AND:
            case TokenType::OP_BIT_OR:
            case TokenType::OP_BIT_XOR:
            case TokenType::OP_LSHIFT:
            case TokenType::OP_RSHIFT:
                return true;
            default:
                return false;
            }
        }

        inline bool IsRightToLeft(TokenType type)
        {
            switch (type) {
            case TokenType::OP_ASSIGN:
            case TokenType::OP_MUL_ASSIGN:
            case TokenType::OP_DIV_ASSIGN:
            case TokenType::OP_MOD_ASSIGN:
            case TokenType::OP_ADD_ASSIGN:
            case TokenType::OP_SUB_ASSIGN:
            case TokenType::OP_LSHIFT_ASSIGN:
            case TokenType::OP_RSHIFT_ASSIGN:
                return true;
            default:
                return false;
            }
        }

        //TODO: FIll shit niga up too
        enum ASTBinaryOp {
            OP_MUL,
            OP_DIV,
            OP_MOD,
            OP_ADD,
            OP_SUB,
            OP_LSHIFT,
            OP_RSHIFT,
            OP_BIT_AND,
            OP_BIT_XOR,
            OP_BIT_OR,
            OP_ASSIGN
        };

        //TODO: FIll this niga up lmaooooo
        inline static std::unordered_map<TokenType, ASTBinaryOp> TokenTypeToASTBinaryOp = {
            {TokenType::OP_MUL, OP_MUL},
            {TokenType::OP_DIV, OP_DIV},
            {TokenType::OP_MOD, OP_MOD},
            {TokenType::OP_LSHIFT, OP_LSHIFT},
            {TokenType::OP_RSHIFT, OP_RSHIFT},
            {TokenType::OP_BIT_AND, OP_BIT_AND},
            {TokenType::OP_BIT_XOR, OP_BIT_XOR},
            {TokenType::OP_BIT_OR, OP_BIT_OR},
            {TokenType::OP_ASSIGN, OP_ASSIGN}
        };

        int GetPrecedence(TokenType type) {
            if (TokenTypeToPrecedence.find(type) != TokenTypeToPrecedence.end()) {
                return TokenTypeToPrecedence[type];
            }
            assert(false && "Unknown token type");
            return -1; //TODO: ERROR?
        }

    }
}
