#pragma once
#include "types/token/token.hh"
#include "precedence_climbing.hh"
#include <string_view>
#include <unordered_map>


namespace baka {
    namespace types {
        enum class ASTUnaryOp {
            OP_UNARY_MINUS,
            OP_UNARY_PLUS,
            OP_TILDE,
            OP_LOGICAL_NOT,
            OP_INC,
            OP_DEC,
            OP_DEREF,
            OP_ADDR_OF,
            OP_SIZEOF,
            OP_NOOP
        };


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
            OP_LOGICAL_AND,
            OP_LOGICAL_OR,
            OP_EQ,
            OP_NEQ,
            OP_LT,
            OP_GT,
            OP_LE,
            OP_GE,
            OP_ASSIGN,
            OP_ADD_ASSIGN,
            OP_SUB_ASSIGN,
            OP_MUL_ASSIGN,
            OP_DIV_ASSIGN,
            OP_MOD_ASSIGN,
            OP_AND_ASSIGN,
            OP_OR_ASSIGN,
            OP_XOR_ASSIGN,
            OP_LSHIFT_ASSIGN,
            OP_RSHIFT_ASSIGN,
            OP_COMMA,
            OP_DOT,
            OP_ARROW
        };


        inline std::string_view ASTUnaryOpToString(ASTUnaryOp op) {
            switch (op) {
                case ASTUnaryOp::OP_UNARY_MINUS: return "OP_UNARY_MINUS";
                case ASTUnaryOp::OP_UNARY_PLUS: return "OP_UNARY_PLUS";
                case ASTUnaryOp::OP_TILDE: return "OP_TILDE";
                case ASTUnaryOp::OP_LOGICAL_NOT: return "OP_LOGICAL_NOT";
                case ASTUnaryOp::OP_INC: return "OP_INC";
                case ASTUnaryOp::OP_DEC: return "OP_DEC";
                case ASTUnaryOp::OP_DEREF: return "OP_DEREF";
                case ASTUnaryOp::OP_ADDR_OF: return "OP_ADDR_OF";
                case ASTUnaryOp::OP_SIZEOF: return "OP_SIZEOF";
                case ASTUnaryOp::OP_NOOP: return "OP_NOOP";
            }
            return "";
        }

        inline std::string_view ASTBinaryOpToString(ASTBinaryOp op) {
            switch (op) {
                case OP_MUL: return "OP_MUL";
                case OP_DIV: return "OP_DIV";
                case OP_MOD: return "OP_MOD";
                case OP_ADD: return "OP_ADD";
                case OP_SUB: return "OP_SUB";
                case OP_LSHIFT: return "OP_LSHIFT";
                case OP_RSHIFT: return "OP_RSHIFT";
                case OP_BIT_AND: return "OP_BIT_AND";
                case OP_BIT_XOR: return "OP_BIT_XOR";
                case OP_BIT_OR: return "OP_BIT_OR";
                case OP_LOGICAL_AND: return "OP_LOGICAL_AND";
                case OP_LOGICAL_OR: return "OP_LOGICAL_OR";
                case OP_EQ: return "OP_EQ";
                case OP_NEQ: return "OP_NEQ";
                case OP_LT: return "OP_LT";
                case OP_GT: return "OP_GT";
                case OP_LE: return "OP_LE";
                case OP_GE: return "OP_GE";
                case OP_ASSIGN: return "OP_ASSIGN";
                case OP_ADD_ASSIGN: return "OP_ADD_ASSIGN";
                case OP_SUB_ASSIGN: return "OP_SUB_ASSIGN";
                case OP_MUL_ASSIGN: return "OP_MUL_ASSIGN";
                case OP_DIV_ASSIGN: return "OP_DIV_ASSIGN";
                case OP_MOD_ASSIGN: return "OP_MOD_ASSIGN";
                case OP_AND_ASSIGN: return "OP_AND_ASSIGN";
                case OP_OR_ASSIGN: return "OP_OR_ASSIGN";
                case OP_XOR_ASSIGN: return "OP_XOR_ASSIGN";
                case OP_LSHIFT_ASSIGN: return "OP_LSHIFT_ASSIGN";
                case OP_RSHIFT_ASSIGN: return "OP_RSHIFT_ASSIGN";
                case OP_COMMA: return "OP_COMMA";
                case OP_DOT: return "OP_DOT";
                case OP_ARROW: return "OP_ARROW";
                default: return "UNKNOWN";
            }
        }


        inline static std::unordered_map<TokenType, ASTUnaryOp> TokenTypeToASTUnaryOp = {
            {TokenType::OP_UNARY_MINUS, ASTUnaryOp::OP_UNARY_MINUS},
            {TokenType::OP_PLUS, ASTUnaryOp::OP_UNARY_PLUS},
            {TokenType::OP_TILDE, ASTUnaryOp::OP_TILDE},
            {TokenType::OP_LOGICAL_NOT, ASTUnaryOp::OP_LOGICAL_NOT},
            {TokenType::OP_INC, ASTUnaryOp::OP_INC},
            {TokenType::OP_DEC, ASTUnaryOp::OP_DEC},
            {TokenType::OP_MUL, ASTUnaryOp::OP_DEREF},
            {TokenType::OP_BIT_AND, ASTUnaryOp::OP_ADDR_OF},
            {TokenType::K_SIZEOF, ASTUnaryOp::OP_SIZEOF},
        };


        inline bool IsBinaryOperator(TokenType type) {
            switch (type) {
                case TokenType::OP_MUL:
                case TokenType::OP_DIV:
                case TokenType::OP_MOD:
                case TokenType::OP_PLUS:
                case TokenType::OP_UNARY_MINUS: // '-' is binary subtraction too
                case TokenType::OP_BIT_AND:
                case TokenType::OP_BIT_OR:
                case TokenType::OP_BIT_XOR:
                case TokenType::OP_LSHIFT:
                case TokenType::OP_RSHIFT:
                case TokenType::OP_EQ:
                case TokenType::OP_NEQ:
                case TokenType::OP_LT:
                case TokenType::OP_GT:
                case TokenType::OP_LE:
                case TokenType::OP_GE:
                case TokenType::OP_LOGICAL_AND:
                case TokenType::OP_LOGICAL_OR:
                case TokenType::OP_ASSIGN:
                case TokenType::OP_ADD_ASSIGN:
                case TokenType::OP_SUB_ASSIGN:
                case TokenType::OP_MUL_ASSIGN:
                case TokenType::OP_DIV_ASSIGN:
                case TokenType::OP_MOD_ASSIGN:
                case TokenType::OP_AND_ASSIGN:
                case TokenType::OP_OR_ASSIGN:
                case TokenType::OP_XOR_ASSIGN:
                case TokenType::OP_LSHIFT_ASSIGN:
                case TokenType::OP_RSHIFT_ASSIGN:
                case TokenType::OP_COMMA:
                case TokenType::OP_DOT:
                case TokenType::OP_ARROW:
                case TokenType::OP_QUESTION:
                    return true;
                default:
                    return false;
            }
        }

        inline bool IsRightToLeft(TokenType type) {
            switch (type) {
                case TokenType::OP_QUESTION:
                case TokenType::OP_ASSIGN:
                case TokenType::OP_MUL_ASSIGN:
                case TokenType::OP_DIV_ASSIGN:
                case TokenType::OP_MOD_ASSIGN:
                case TokenType::OP_ADD_ASSIGN:
                case TokenType::OP_SUB_ASSIGN:
                case TokenType::OP_LSHIFT_ASSIGN:
                case TokenType::OP_RSHIFT_ASSIGN:
                case TokenType::OP_OR_ASSIGN:
                case TokenType::OP_XOR_ASSIGN:
                case TokenType::OP_AND_ASSIGN:
                    return true;
                default:
                    return false;
            }
        }


        inline static std::unordered_map<TokenType, ASTBinaryOp> TokenTypeToASTBinaryOp = {
            {TokenType::OP_MUL, OP_MUL},
            {TokenType::OP_DIV, OP_DIV},
            {TokenType::OP_MOD, OP_MOD},
            {TokenType::OP_PLUS, OP_ADD},
            {TokenType::OP_UNARY_MINUS, OP_SUB},
            {TokenType::OP_LSHIFT, OP_LSHIFT},
            {TokenType::OP_RSHIFT, OP_RSHIFT},
            {TokenType::OP_BIT_AND, OP_BIT_AND},
            {TokenType::OP_BIT_XOR, OP_BIT_XOR},
            {TokenType::OP_BIT_OR, OP_BIT_OR},
            {TokenType::OP_LOGICAL_AND, OP_LOGICAL_AND},
            {TokenType::OP_LOGICAL_OR, OP_LOGICAL_OR},
            {TokenType::OP_EQ, OP_EQ},
            {TokenType::OP_NEQ, OP_NEQ},
            {TokenType::OP_LT, OP_LT},
            {TokenType::OP_GT, OP_GT},
            {TokenType::OP_LE, OP_LE},
            {TokenType::OP_GE, OP_GE},
            {TokenType::OP_ASSIGN, OP_ASSIGN},
            {TokenType::OP_ADD_ASSIGN, OP_ADD_ASSIGN},
            {TokenType::OP_SUB_ASSIGN, OP_SUB_ASSIGN},
            {TokenType::OP_MUL_ASSIGN, OP_MUL_ASSIGN},
            {TokenType::OP_DIV_ASSIGN, OP_DIV_ASSIGN},
            {TokenType::OP_MOD_ASSIGN, OP_MOD_ASSIGN},
            {TokenType::OP_AND_ASSIGN, OP_AND_ASSIGN},
            {TokenType::OP_OR_ASSIGN, OP_OR_ASSIGN},
            {TokenType::OP_XOR_ASSIGN, OP_XOR_ASSIGN},
            {TokenType::OP_LSHIFT_ASSIGN, OP_LSHIFT_ASSIGN},
            {TokenType::OP_RSHIFT_ASSIGN, OP_RSHIFT_ASSIGN},
            {TokenType::OP_COMMA, OP_COMMA},
            {TokenType::OP_DOT, OP_DOT},
            {TokenType::OP_ARROW, OP_ARROW}
        };

        inline int GetPrecedence(TokenType type) {
            if (TokenTypeToPrecedence.find(type) != TokenTypeToPrecedence.end()) {
                return TokenTypeToPrecedence[type];
            }
            return -1;
        }
    }
}
