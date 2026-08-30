#include <cctype>

namespace baka::base {
    bool isValidStringChar(char c) {
        return std::isprint(c) || c == '\t' || c == '\v' || c == '\f' || c == '\r';
    }

    
    
    bool isSimpleEscapeSequenceChar(char c) {
        return c == '\\' || c == '\"' || c == '\'' || c == '?' || c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' ||
            c == 't' || c == 'v';
    }


    char SimpleEscapeSequence(char suffix) {
        switch (suffix) {
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case 'f':
            return '\f';
        case 'a':
            return '\a';
        case 'b':
            return '\b';

        case '\'':
            return '\'';
        case '"':
            return '"';
        case '?':
            return '?';
        case '\\':
            return '\\';
        default:
            return suffix;
        }
    }


    bool isValidKeywordNonDigit(char c) {
        // https://timsong-cpp.github.io/cppwp/lex#nt:nondigit
        return std::isalpha(c) || c == '_';
    }

    bool isValidDigit(char c, int base) {
        switch (base) {
        case 2:
            return '0' <= c && c <= '1';
        case 8:
            return '0' <= c && c <= '7';
        case 10:
            return '0' <= c && c <= '9';
        case 16:
            return ('0' <= c && c <= '9') || ('a' <= std::tolower(c) && std::tolower(c) <= 'f');
        default:
            return std::isdigit(c);
        }
    }


    bool isValidOperatorChar(char c) {
        return c == '~' || c == '-' || c == '+' || c == '*' ||
            c == '/' || c == '%' || c == '&' || c == '|' ||
            c == '^' || c == '<' || c == '>' || c == '=' ||
            c == '!' || c == ':' || c == ',' || c == '?' || c == '.';
    }
}
