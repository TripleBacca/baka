#pragma once
#include <cassert>
#include <ostream>
#include <string_view>
#include <unordered_map>
#include "token.hh"


namespace {

    inline std::unordered_map<types::TokenType, std::string_view> TokenToStr = {
        {types::TokenType::K_BOOL,"K_BOOL"},
        {types::TokenType::K_BREAK,"K_BREAK"},
        {types::TokenType::K_CASE,"K_CASE"},
        {types::TokenType::K_CHAR,"K_CHAR"},
        {types::TokenType::K_CLASS,"K_CLASS"},
        {types::TokenType::K_CONST,"K_CONST"},
        {types::TokenType::K_CONTINUE,"K_CONTINUE"},
        {types::TokenType::K_DELETE,"K_DELETE"},
        {types::TokenType::K_DO,"K_DO"},
        {types::TokenType::K_DOUBLE,"K_DOUBLE"},
        {types::TokenType::K_ELSE,"K_ELSE"},
        {types::TokenType::K_ENUM,"K_ENUM"},
        {types::TokenType::K_FALSE,"K_FALSE"},
        {types::TokenType::K_FLOAT,"K_FLOAT"},
        {types::TokenType::K_FOR,"K_FOR"},
        {types::TokenType::K_GOTO,"K_GOTO"},
        {types::TokenType::K_IF,"K_IF"},
        {types::TokenType::K_INT,"K_INT"},
        {types::TokenType::K_LONG,"K_LONG"},
        {types::TokenType::K_NAMESPACE,"K_NAMESPACE"},
        {types::TokenType::K_NEW,"K_NEW"},
        {types::TokenType::K_RETURN,"K_RETURN"},
        {types::TokenType::K_SHORT,"K_SHORT"},
        {types::TokenType::K_SIGNED,"K_SIGNED"},
        {types::TokenType::K_SIZEOF,"K_SIZEOF"},
        {types::TokenType::K_STATIC,"K_STATIC"},
        {types::TokenType::K_STRUCT,"K_STRUCT"},
        {types::TokenType::K_SWITCH,"K_SWITCH"},
        {types::TokenType::K_THIS,"K_THIS"},
        {types::TokenType::K_TRUE,"K_TRUE"},
        {types::TokenType::K_TYPEDEF,"K_TYPEDEF"},
        {types::TokenType::K_UNION,"K_UNION"},
        {types::TokenType::K_UNSIGNED,"K_UNSIGNED"},
        {types::TokenType::K_USING,"K_USING"},
        {types::TokenType::K_VOID,"K_VOID"},
        {types::TokenType::K_WHILE,"K_WHILE"},
        {types::TokenType::OP_TILDE,"OP_TILDE"},
        {types::TokenType::OP_UNARY_MINUS,"OP_UNARY_MINUS"},
        {types::TokenType::OP_DEC,"OP_DEC"},
        {types::TokenType::OP_INC,"OP_INC"},
        {types::TokenType::OP_MUL,"OP_MUL"},
        {types::TokenType::OP_DIV,"OP_DIV"},
        {types::TokenType::OP_MOD,"OP_MOD"},
        {types::TokenType::OP_BIT_AND,"OP_BIT_AND"},
        {types::TokenType::OP_BIT_OR,"OP_BIT_OR"},
        {types::TokenType::OP_BIT_XOR,"OP_BIT_XOR"},
        {types::TokenType::OP_LSHIFT,"OP_LSHIFT"},
        {types::TokenType::OP_RSHIFT,"OP_RSHIFT"},
        {types::TokenType::OP_LOGICAL_AND,"OP_LOGICAL_AND"},
        {types::TokenType::OP_LOGICAL_OR,"OP_LOGICAL_OR"},
        {types::TokenType::OP_EQ,"OP_EQ"},
        {types::TokenType::OP_NEQ,"OP_NEQ"},
        {types::TokenType::OP_LT,"OP_LT"},
        {types::TokenType::OP_GT,"OP_GT"},
        {types::TokenType::OP_LE,"OP_LE"},
        {types::TokenType::OP_GE,"OP_GE"},
        {types::TokenType::OP_PLUS,"OP_PLUS"},
        {types::TokenType::OP_LOGICAL_NOT,"OP_LOGICAL_NOT"},
        {types::TokenType::OP_ASSIGN,"OP_ASSIGN"},
        {types::TokenType::OP_ADD_ASSIGN,"OP_ADD_ASSIGN"},
        {types::TokenType::OP_SUB_ASSIGN,"OP_SUB_ASSIGN"},
        {types::TokenType::OP_MUL_ASSIGN,"OP_MUL_ASSIGN"},
        {types::TokenType::OP_DIV_ASSIGN,"OP_DIV_ASSIGN"},
        {types::TokenType::OP_MOD_ASSIGN,"OP_MOD_ASSIGN"},
        {types::TokenType::OP_AND_ASSIGN,"OP_AND_ASSIGN"},
        {types::TokenType::OP_OR_ASSIGN,"OP_OR_ASSIGN"},
        {types::TokenType::OP_XOR_ASSIGN,"OP_XOR_ASSIGN"},
        {types::TokenType::OP_LSHIFT_ASSIGN,"OP_LSHIFT_ASSIGN"},
        {types::TokenType::OP_RSHIFT_ASSIGN,"OP_RSHIFT_ASSIGN"},
        {types::TokenType::OP_COLON,"OP_COLON"},
        {types::TokenType::OP_DOUBLE_COLON,"OP_DOUBLE_COLON"},
        {types::TokenType::OP_COMMA,"OP_COMMA"},
        {types::TokenType::OP_QUESTION,"OP_QUESTION"},
        {types::TokenType::OP_DOT,"OP_DOT"},
        {types::TokenType::OP_ARROW,"OP_ARROW"},
        {types::TokenType::OP_ELLIPSIS,"OP_ELLIPSIS"},
        {types::TokenType::LPAREN_ROUND,"LPAREN_ROUND"},
        {types::TokenType::RPAREN_ROUND,"RPAREN_ROUND"},
        {types::TokenType::LPAREN_SQUARE,"LPAREN_SQUARE"},
        {types::TokenType::RPAREN_SQUARE,"RPAREN_SQUARE"},
        {types::TokenType::LPAREN_CURLY,"LPAREN_CURLY"},
        {types::TokenType::RPAREN_CURLY,"RPAREN_CURLY"},
        {types::TokenType::SEMICOLON,"SEMICOLON"},
        {types::TokenType::IDENTIFIER,"IDENTIFIER"},
        {types::TokenType::LITERAL_INTEGER,"LITERAL_INTEGER"},
        {types::TokenType::LITERAL_CHARACTER,"LITERAL_CHARACTER"},
        {types::TokenType::LITERAL_FP,"LITERAL_FP"},
        {types::TokenType::LITERAL_STRING,"LITERAL_STRING"},
        {types::TokenType::LITERAL_BOOL,"LITERAL_BOOL"},
        {types::TokenType::LITERAL_PTR,"LITERAL_PTR"},
        {types::TokenType::LITERAL_NULL,"LITERAL_NULL"},
        {types::TokenType::HEADER_NAME,"HEADER_NAME"}, // possible needed by preprocessor
        {types::TokenType::EOF_TOKEN,"EOF_TOKEN"},
        {types::TokenType::UNKNOWN,"UNKNOWN"},
    };

}

inline std::string_view TokenTypeToStr(types::TokenType tokenType) {
    return TokenToStr.at(tokenType);
}

inline std::ostream& operator<<(std::ostream& os, const types::Token& token) {
    switch(token.Value.index()) {
        case 0:
            os << "'" << std::get<char>(token.Value) << "'";
            break;
        case 1:
            os << std::get<std::string_view>(token.Value);
            break;
        case 2:
            os << '"' << std::get<std::string>(token.Value) << '"';
            break;
        default:
            assert(false && "what the helly?");
            break;
    }

    os << " (TokenType: " << TokenTypeToStr(token.TokenType_v) << ")";
    return os;
}
