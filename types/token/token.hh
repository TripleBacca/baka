#pragma once
#include <string_view>


namespace types {

    enum class TokenType {
        // https://timsong-cpp.github.io/cppwp/lex.key
        K_BOOL,
        K_BREAK,
        K_CASE,
        K_CHAR,
        K_CLASS,
        K_CONST,
        K_CONTINUE,
        K_DELETE,
        K_DO,
        K_DOUBLE,
        K_ELSE,
        K_ENUM,
        K_FALSE,
        K_FLOAT,
        K_FOR,
        K_GOTO,
        K_IF,
        K_INT,
        K_LONG,
        K_MUTABLE,
        K_NAMESPACE,
        K_NEW,
        K_RETURN,
        K_SHORT,
        K_SIGNED,
        K_SIZEOF,
        K_STATIC,
        K_STRUCT,
        K_SWITCH,
        K_THIS,
        K_TRUE,
        K_TYPEDEF,
        K_UNION,
        K_UNSIGNED,
        K_USING,
        K_VOID,
        K_WHILE,

        // operators: https://timsong-cpp.github.io/cppwp/lex.operators#nt:operator-or-punctuator
        // unary operators
        OP_TILDE,
        OP_UNARY_MINUS,
        OP_DEC,
        OP_INC,
        // binary operators
        OP_MUL,
        OP_DIV,
        OP_MOD,
        OP_BIT_AND,
        OP_BIT_OR,
        OP_BIT_XOR,
        OP_LSHIFT,
        OP_RSHIFT,
        OP_LOGICAL_AND,
        OP_LOGICAL_OR,
        OP_EQ,
        OP_NEQ,
        OP_LT,
        OP_GT,
        OP_LE,
        OP_GE,
        OP_PLUS,
        OP_LOGICAL_NOT,
        // probably will not support these
        // OP_DOT_STAR, .*
        // OP_ARROW_STAR, ->*

        // assignment operators
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
        OP_COLON,
        OP_DOUBLE_COLON,
        OP_COMMA,
        OP_QUESTION,
        OP_DOT,
        OP_ARROW,
        OP_ELLIPSIS,

        // punctuators
        LPAREN_ROUND,
        RPAREN_ROUND,
        LPAREN_SQUARE,
        RPAREN_SQUARE,
        LPAREN_CURLY,
        RPAREN_CURLY,
        SEMICOLON,

        // identifiers
        IDENTIFIER,

        // literals: https://timsong-cpp.github.io/cppwp/lex.literal
        // If a token matches both user-defined-literal and another literal kind, it is treated as the latter.
        LITERAL_INTEGER,
        LITERAL_CHARACTER,
        LITERAL_FP,
        LITERAL_STRING,
        LITERAL_BOOL,
        LITERAL_PTR,
        LITERAL_NULL,
        // LITERAL_USER_DEFINED, not supporting this. its like in chrono 1s is lexed

        // header name
        HEADER_NAME, // possible needed by preprocessor

        // special:
        EOF_TOKEN,
        UNKNOWN
    };

    struct TokenSourceLocation {
        size_t LineNo;
        size_t Col;
    };

    struct Token {
        TokenType TokenType;
        std::string_view Lexeme;
    };
}
