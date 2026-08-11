#include "lexer.hh"
#include <cctype>
#include <exception>
#include <iostream>
#include <string>
#include "../types/token/all.hh"
#include "../types/exceptions.hh"
#include "base/base.hh"

namespace baka {
namespace lexer {


std::vector<types::Token> Tokenize(std::string_view SourceCode) {
    std::vector<types::Token> tokens;

    std::vector<std::string> errors; // this is very bad TODO

    size_t LineNo = 1;
    size_t ColNo = 1;

    size_t lh = 0;
    while(lh < SourceCode.size()) {
        char curr = SourceCode[lh];

        if(std::isalpha(curr)) {
            // identifier or keyword
            // deal with boolean

        } else if(curr == '(') {
            tokens.emplace_back(types::TokenType::LPAREN_ROUND, std::string_view("("));
            lh++;
            ColNo++;
        } else if(curr == ')') {
            tokens.emplace_back(types::TokenType::RPAREN_ROUND, std::string_view(")"));
            lh++;
            ColNo++;
        } else if(curr == '[') {
            tokens.emplace_back(types::TokenType::LPAREN_SQUARE, std::string_view("["));
            lh++;
            ColNo++;
        } else if(curr == ']') {
            tokens.emplace_back(types::TokenType::RPAREN_SQUARE, std::string_view("]"));
            lh++;
            ColNo++;
        } else if(curr == '{') {
            tokens.emplace_back(types::TokenType::LPAREN_CURLY, std::string_view("{"));
            lh++;
            ColNo++;
        } else if(curr == '}') {
            tokens.emplace_back(types::TokenType::RPAREN_CURLY, std::string_view("}"));
            lh++;
            ColNo++;
        } else if (curr == ';') {
            tokens.emplace_back(types::TokenType::SEMICOLON, std::string_view(";"));
            lh++;
            ColNo++;
        } else if(curr == '\'') {
            size_t peek = lh+1;
            char ActualChar;
            bool CharSeen = false;
            bool RQuoteSeen = false;

            while(peek < SourceCode.size()) {
                if(SourceCode[peek] == '\\') {
                    peek++;
                    // need to escape whatevers after
                    // cannot end on slash
                    if(peek >= SourceCode.size()) {
                        errors.push_back("Invalid escape sequence");
                        break;
                    }

                    char q = SourceCode[peek];

                    if(base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                        ActualChar = base::SimpleEscapeSequence(q);
                        CharSeen = true;
                    } else if(std::isdigit(q) &&  (0 <= (q - '0')) && (7 >= (q - '0'))) {
                        // octal escape sequence
                        // need to do this for \0

                        long long octal_seq = (q - '0');
                        peek++;
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
                    } else {
                        errors.push_back("invalid octal escape sequence");
                        break;
                        // TODO: put error here

                    }
                } else if(base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '\'') {
                    ActualChar = SourceCode[peek];
                    CharSeen = true;
                } else if (SourceCode[peek] == '\'') {
                    RQuoteSeen = true;
                    break;
                } else {
                    errors.push_back("invalid character in charcter literal");
                    break;
                    // TODO: put error here
                }
                peek++;
            }

            size_t Oldlh = lh;
            lh = ++peek;
            ColNo += lh - Oldlh;

            if(RQuoteSeen && CharSeen) {
                tokens.emplace_back(types::TokenType::LITERAL_CHARACTER, ActualChar);
            } else {
                // if breaked and no rquote then only case is error
                //
                // either r quote not seen
                // or char not seen
                if(!CharSeen) {
                    errors.push_back("no character in character literal");
                }
                if(!RQuoteSeen) {
                    errors.push_back("character literal not terminated");
                }
                tokens.emplace_back(types::TokenType::UNKNOWN, ActualChar);
            }

        } else if(curr == '"') {
            //string literal

            size_t peek = lh+1;
            std::string str; // TODO: allocate this in an arena somewhere
            bool RQuoteSeen = false;

            while(peek < SourceCode.size()) {
                if(SourceCode[peek] == '\\') {
                    peek++;
                    // need to escape whatevers after
                    // cannot end on slash
                    if(peek >= SourceCode.size()) {
                        errors.push_back("Invalid escape sequence");
                        break;
                    }

                    char q = SourceCode[peek];

                    if(base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                        char ToAdd = base::SimpleEscapeSequence(q);
                        str += (ToAdd);
                    } else if(std::isdigit(q) &&  (0 <= (q - '0')) && (7 >= (q - '0'))) {
                        // octal escape sequence
                        // need to do this for \0

                        long long octal_seq = (q - '0');
                        peek++;
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
                        errors.push_back("invalid octal escape sequence");
                        break;
                        // TODO: put error here

                    }
                } else if(base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '"') {
                    str += SourceCode[peek];
                } else if (SourceCode[peek] == '"') {
                    RQuoteSeen = true;
                    break;
                } else {
                    errors.push_back("invalid character in charcter literal");
                    break;
                    // TODO: put error here
                }
                peek++;
            }

            size_t Oldlh = lh;
            lh = ++peek;
            ColNo += lh - Oldlh;

            if(RQuoteSeen) {
                tokens.emplace_back(types::TokenType::LITERAL_STRING, std::move(str));
            } else {
                // if breaked and no rquote then only case is error
                //
                // either r quote not seen or invalid char
                if(!RQuoteSeen) {
                    errors.push_back("string literal not terminated");
                }
                tokens.emplace_back(types::TokenType::UNKNOWN, str);
            }

        } else if(std::isdigit(curr)) {
            // fp or int
            // mulch and regex bash
        } else if(curr == ' ' || curr == '\t') {
            lh++;
            ColNo++;
        } else if(curr == '\n') {
            lh++;
            LineNo++;
            ColNo = 1;
        } else {
            // this is operator
        }
    }

    return tokens;
}

} // namespace lexer
} // namespace baka
