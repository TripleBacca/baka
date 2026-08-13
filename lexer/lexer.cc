#include "lexer.hh"
#include <cctype>
#include <regex>
#include <string>
#include <string_view>
#include "../types/token/all.hh"
#include "../types/exceptions.hh"
#include "base/base.hh"
#include "keywords.hh"
#include "operators.hh"

namespace baka {
namespace lexer {


std::vector<types::Token> Tokenize(std::string_view SourceCode) {
    std::vector<types::Token> tokens;

    std::vector<std::string> errors; // this is very bad TODO
    std::vector<std::string> warnings; // this is very bad TODO

    size_t LineNo = 1;
    size_t ColNo = 1;

    size_t lh = 0;



    std::regex float_regex(R"(^(([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+))([eE][+-]?([0-9]+))?[fF]$)");
    std::regex double_regex(R"(^(([0-9]+\.?[0-9]*)|([0-9]*\.?[0-9]+))([eE][+-]?([0-9]+))?$)");
    std::regex long_regex(R"(^((0b[0-1])|(0o[0-7])|(0x[0-9a-fA-F])|([0-9]+))[lL]$)");
    std::regex int_regex(R"(^((0b[0-1])|(0o[0-7])|(0x[0-9a-fA-F])|([0-9]+))$)");

    while(lh < SourceCode.size()) {
        char curr = SourceCode[lh];

        if(base::isValidKeywordNonDigit(curr)) {
            // keyword or identifier
            // deal with boolean (kword deals with this already)

            size_t startIdx = lh;
            size_t endIdx = startIdx;

            size_t peek = lh+1;
            while(peek < SourceCode.size() && base::isValidKeywordNonDigit(SourceCode[peek]) || std::isdigit(SourceCode[peek])) {
                endIdx = peek;
                peek++;
            }

            std::string_view lexeme = SourceCode.substr(startIdx, endIdx - startIdx + 1);
            if(Kw_lexeme_to_type.contains(lexeme)) {
                tokens.emplace_back(Kw_lexeme_to_type.at(lexeme), lexeme);
                lh = endIdx + 1;
                ColNo += endIdx - startIdx + 1;
            } else {
                tokens.emplace_back(types::TokenType::IDENTIFIER, lexeme);
                lh = endIdx + 1;
                ColNo += endIdx - startIdx + 1;
            }
        }
        else if(curr == '(') {
            tokens.emplace_back(types::TokenType::LPAREN_ROUND, std::string_view("("));
            lh++;
            ColNo++;
        }
        else if(curr == ')') {
            tokens.emplace_back(types::TokenType::RPAREN_ROUND, std::string_view(")"));
            lh++;
            ColNo++;
        }
        else if(curr == '[') {
            tokens.emplace_back(types::TokenType::LPAREN_SQUARE, std::string_view("["));
            lh++;
            ColNo++;
        }
        else if(curr == ']') {
            tokens.emplace_back(types::TokenType::RPAREN_SQUARE, std::string_view("]"));
            lh++;
            ColNo++;
        }
        else if(curr == '{') {
            tokens.emplace_back(types::TokenType::LPAREN_CURLY, std::string_view("{"));
            lh++;
            ColNo++;
        }
        else if(curr == '}') {
            tokens.emplace_back(types::TokenType::RPAREN_CURLY, std::string_view("}"));
            lh++;
            ColNo++;
        }
        else if (curr == ';') {
            tokens.emplace_back(types::TokenType::SEMICOLON, std::string_view(";"));
            lh++;
            ColNo++;
        }
        else if(curr == '\'') {
            // character literal

            size_t peek = lh+1;
            char ActualChar;
            bool CharSeen = false;
            size_t CharLength = 0;
            bool RQuoteSeen = false;

            while(peek < SourceCode.size()) {
                if(SourceCode[peek] == '\\') {
                    // possibly escape seq
                    peek++;
                    if(peek >= SourceCode.size()) {
                        errors.emplace_back("Invalid escape sequence");
                        break; //TODO: Is this safe????
                    }

                    char q = SourceCode[peek];
                    // q must either be a letter for a simple escape
                    // or
                    // q must be a number 0-7 for octal escape seq


                    if(base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                        ActualChar = base::SimpleEscapeSequence(q);
                        CharSeen = true;
                        CharLength++;
                    } else if(std::isdigit(q) &&  (0 <= (q - '0')) && (7 >= (q - '0'))) {
                        // octal escape sequence
                        // need to do this for \0

                        long long octal_seq = (q - '0');
                        peek++;

                        // octal escape seq is max 3 digits
                        size_t DigitsSeen = 1;

                        while(peek < SourceCode.size() &&
                              SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                              DigitsSeen < 3)
                        {
                            octal_seq *= 8;
                            octal_seq += (SourceCode[peek] - '0');
                            DigitsSeen++;
                            peek++;
                        }
                        peek--; // move it back to last scanned char

                        ActualChar = static_cast<char>(octal_seq);
                        CharSeen = true;
                        CharLength++;
                    } else {
                        // TODO: put error here, is line and column even updated????
                        errors.emplace_back("invalid octal escape sequence line: {} column: {}", LineNo, ColNo);
                        break;
                    }
                } else if(base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '\'') {
                    // normal ascii char
                    ActualChar = SourceCode[peek];
                    CharSeen = true;
                    CharLength++;
                } else if (SourceCode[peek] == '\'') {
                    // end literal here
                    RQuoteSeen = true;
                    break;
                } else {
                    // TODO: put error here
                    errors.emplace_back("invalid character in character literal");
                    break;
                }
                peek++;
            }

            size_t Oldlh = lh;
            lh = ++peek;
            ColNo += lh - Oldlh;

            if(RQuoteSeen && CharSeen) {
                tokens.emplace_back(types::TokenType::LITERAL_CHARACTER, ActualChar);

                if(CharLength > 1) {
                    // TODO: warn about multilength char
                }
            } else {
                // must be invalid char or (ended literal without any char)
                if(!CharSeen) {
                    errors.push_back("no character in character literal");
                }
                if(!RQuoteSeen) {
                    errors.push_back("character literal not terminated");
                }
                tokens.emplace_back(types::TokenType::UNKNOWN, ActualChar);
            }

        }
        else if(curr == '"') {
            //string literal

            size_t peek = lh+1;
            std::string str; // TODO: allocate this in an arena somewhere
            bool RQuoteSeen = false;

            while(peek < SourceCode.size()) {
                if(SourceCode[peek] == '\\') {
                    // escape sequence

                    peek++;
                    if(peek >= SourceCode.size()) {
                        errors.push_back("Invalid escape sequence");
                        break;
                    }

                    char q = SourceCode[peek];
                    // q is either letter for escape seq
                    // or
                    // q is 0-7 for octal escape seq

                    if(base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                        // letter escape seq
                        char ToAdd = base::SimpleEscapeSequence(q);
                        str += (ToAdd);
                    } else if(std::isdigit(q) &&  (0 <= (q - '0')) && (7 >= (q - '0'))) {
                        // octal escape sequence
                        // need to do this for \0

                        long long octal_seq = (q - '0');
                        peek++;

                        // octal escape seq has max 3 digits
                        size_t DigitsSeen = 1;

                        while(peek < SourceCode.size() &&
                              SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                              DigitsSeen < 3)
                        {
                            octal_seq *= 8;
                            octal_seq += (SourceCode[peek] - '0');
                            DigitsSeen++;
                            peek++;
                        }
                        peek--; // move it back to last scanned char

                        str += static_cast<char>(octal_seq);

                    } else {
                        // TODO: put error here
                        errors.push_back("invalid octal escape sequence");
                        break;
                    }
                } else if(base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '"') {
                    // normal ascii char
                    str += SourceCode[peek];
                } else if (SourceCode[peek] == '"') {
                    // end literal here
                    RQuoteSeen = true;
                    break;
                } else {
                    // TODO: put error here
                    errors.push_back("invalid character in charcter literal");
                    break;
                }
                peek++;
            }

            size_t Oldlh = lh;
            lh = ++peek;
            ColNo += lh - Oldlh;

            if(RQuoteSeen) {
                tokens.emplace_back(types::TokenType::LITERAL_STRING, std::move(str));
            } else {
                // must be invalid char
                if(!RQuoteSeen) {
                    errors.push_back("string literal not terminated");
                }
                tokens.emplace_back(types::TokenType::UNKNOWN, str);
            }

        }
        else if(std::isdigit(curr)) {
            // fp or int
            // mulch and regex bash


        }
        else if(curr == ' ' || curr == '\t') {
            lh++;
            ColNo++;
        }
        else if(curr == '\n') {
            lh++;
            LineNo++;
            ColNo = 1;
        }
        else if (base::isValidOperatorChar(curr)) {
            // this is operator
            // TODO: Remove comments before lexing
            if (curr == '.')
            {
                // check for ellipsis
                if (lh + 2 < SourceCode.size() && SourceCode[lh + 1] == '.' && SourceCode[lh + 2] == '.') {
                    tokens.emplace_back(types::TokenType::OP_ELLIPSIS, std::string_view(SourceCode.substr(lh, 3)));
                    lh += 3;
                    ColNo += 3;
                }
                else
                {
                    tokens.emplace_back(types::TokenType::OP_DOT, std::string_view(SourceCode.substr(lh, 1)));
                    lh++;
                    ColNo++;
                }
            }
            else
            {
                size_t startIdx = lh;
                size_t endIdx = startIdx;

                size_t peek = lh + 1;
                while (peek < SourceCode.size() && base::isValidOperatorChar(SourceCode[peek]) && lexeme_to_operator.contains(SourceCode.substr(startIdx, peek - startIdx + 1))) {
                    endIdx = peek;
                    peek++;
                }
                std::string_view lexeme = SourceCode.substr(startIdx, endIdx - startIdx + 1);
                tokens.emplace_back(lexeme_to_operator.at(lexeme), lexeme);
                lh = endIdx + 1;
                ColNo += endIdx - startIdx + 1;
            }

        }
        else {
            // TODO: put error here -> Unknown token
            tokens.emplace_back(types::TokenType::UNKNOWN, std::string_view(&curr, 1));
            lh++;
            ColNo++;
        }
    }

    return tokens;
}

} // namespace lexer
} // namespace baka
