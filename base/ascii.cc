

#include <cctype>
namespace baka {
    namespace base {

        bool isValidStringChar(char c) {
            return std::isprint(c) || c == '\t' || c == '\v' || c == '\f' || c == '\r';
        }

        bool isSimpleEscapeSequenceChar(char c) {
            return c == '\\' || c == '\"' || c == '\'' || c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' || c == 'v';
        }


        char SimpleEscapeSequence(char suffix) {
            switch(suffix) {
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
    }
}
