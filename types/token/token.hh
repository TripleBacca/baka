#pragma once
#include <string_view>


namespace types {

    enum class TokenType {
        // https://timsong-cpp.github.io/cppwp/lex.key
        K_BOOL,         // keyword: 'bool'
        K_BREAK,        // keyword: 'break'
        K_CASE,         // keyword: 'case'
        K_CHAR,         // keyword: 'char'
        K_CLASS,        // keyword: 'class'
        K_CONST,        // keyword: 'const'
        K_CONTINUE,     // keyword: 'continue'
        K_DELETE,       // keyword: 'delete'
        K_DO,           // keyword: 'do'
        K_DOUBLE,       // keyword: 'double'
        K_ELSE,         // keyword: 'else'
        K_ENUM,         // keyword: 'enum'
        K_FALSE,        // literal: 'false'
        K_FLOAT,        // keyword: 'float'
        K_FOR,          // keyword: 'for'
        K_GOTO,         // keyword: 'goto'
        K_IF,           // keyword: 'if'
        K_INT,          // keyword: 'int'
        K_LONG,         // keyword: 'long'
        K_MUTABLE,      // keyword: 'mutable'
        K_NAMESPACE,    // keyword: 'namespace'
        K_NEW,          // keyword: 'new'
        K_RETURN,       // keyword: 'return'
        K_SHORT,        // keyword: 'short'
        K_SIGNED,       // keyword: 'signed'
        K_SIZEOF,       // keyword: 'sizeof'
        K_STATIC,       // keyword: 'static'
        K_STRUCT,       // keyword: 'struct'
        K_SWITCH,       // keyword: 'switch'
        K_THIS,         // keyword: 'this'
        K_TRUE,         // literal: 'true'
        K_TYPEDEF,      // keyword: 'typedef'
        K_UNION,        // keyword: 'union'
        K_UNSIGNED,     // keyword: 'unsigned'
        K_USING,        // keyword: 'using'
        K_VOID,         // keyword: 'void'
        K_WHILE,        // keyword: 'while'

        // operators: https://timsong-cpp.github.io/cppwp/lex.operators#nt:operator-or-punctuator
        // unary operators
        OP_TILDE,       // '~' bitwise NOT                          SINGLE
        OP_MINUS,       // '-' unary minus (also binary minus)
        OP_DEC,         // '--' decrement
        OP_INC,         // '++' increment
        // binary operators
        OP_MUL,         // '*' multiplication
        OP_DIV,         // '/' division
        OP_MOD,         // '%' modulus
        OP_BIT_AND,     // '&' bitwise AND
        OP_BIT_OR,      // '|' bitwise OR
        OP_BIT_XOR,     // '^' bitwise XOR
        OP_LSHIFT,      // '<<' left shift
        OP_RSHIFT,      // '>>' right shift
        OP_LOGICAL_AND, // '&&' logical AND
        OP_LOGICAL_OR,  // '||' logical OR
        OP_EQ,          // '==' equality
        OP_NEQ,         // '!=' not equal
        OP_LT,          // '<' less-than
        OP_GT,          // '>' greater-than
        OP_LE,          // '<=' less-or-equal
        OP_GE,          // '>=' greater-or-equal
        OP_PLUS,        // '+' unary plus / binary addition
        OP_LOGICAL_NOT, // '!' logical NOT
        // probably will not support these
        // OP_DOT_STAR, .*
        // OP_ARROW_STAR, ->*

        // assignment operators
        OP_ASSIGN,         // '=' assignment
        OP_ADD_ASSIGN,     // '+=' add and assign
        OP_SUB_ASSIGN,     // '-=' subtract and assign
        OP_MUL_ASSIGN,     // '*=' multiply and assign
        OP_DIV_ASSIGN,     // '/=' divide and assign
        OP_MOD_ASSIGN,     // '%=' modulus and assign
        OP_AND_ASSIGN,     // '&=' bitwise AND and assign
        OP_OR_ASSIGN,      // '|=' bitwise OR and assign
        OP_XOR_ASSIGN,     // '^=' bitwise XOR and assign
        OP_LSHIFT_ASSIGN,  // '<<=' left shift and assign
        OP_RSHIFT_ASSIGN,  // '>>=' right shift and assign
        OP_COLON,          // ':' colon
        OP_DOUBLE_COLON,   // '::' scope resolution
        OP_COMMA,          // ',' comma
        OP_QUESTION,       // '?' ternary question
        OP_DOT,            // '.' member access
        OP_ARROW,          // '->' pointer member access
        OP_ELLIPSIS,       // '...' variadic/ellipsis

        // punctuators
        LPAREN_ROUND,   // '('
        RPAREN_ROUND,   // ')'
        LPAREN_SQUARE,  // '['
        RPAREN_SQUARE,  // ']'
        LPAREN_CURLY,   // '{'
        RPAREN_CURLY,   // '}'
        SEMICOLON,      // ';'

        // identifiers
        IDENTIFIER,     // user-defined identifier (names)

        // literals: https://timsong-cpp.github.io/cppwp/lex.literal
        // If a token matches both user-defined-literal and another literal kind, it is treated as the latter.
        LITERAL_INTEGER,   // integer literal (decimal/hex/oct/bin)
        LITERAL_CHARACTER, // character literal: 'a' or '\n'
        LITERAL_FP,        // floating-point literal
        LITERAL_STRING,    // string literal: "..."
        LITERAL_BOOL,      // boolean literal (true/false)
        LITERAL_PTR,       // pointer literal: nullptr
        // LITERAL_USER_DEFINED, not supporting this. its like in chrono 1s is lexed

        // header name
        //HEADER_NAME, // possible needed by preprocessor

        // special:
        EOF_TOKEN,       // end-of-file marker
        UNKNOWN          // unknown or unrecognized token
    };

    struct TokenSourceLocation {
        std::string_view FilePath;
        size_t LineNo;
        size_t Col;
    };

    struct Token {
        TokenType TokenType_v;
        std::string_view Lexeme;
    };
}
