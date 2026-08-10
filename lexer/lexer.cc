#include "lexer.hh"
#include <cctype>
#include <exception>
#include "../types/token/all.hh"
#include "../types/exceptions.hh"
#include "base/base.hh"

namespace baka {
namespace lexer {


std::vector<types::Token> Tokenize(std::string_view SourceCode) {
    std::vector<types::Token> tokens;

    std::vector<std::string> errors; // this is very bad TODO


    size_t lh = 0;
    while(lh < SourceCode.size()) {

        char curr = SourceCode[lh];
        if(std::isalpha(curr)) {
            // identifier or keyword
            // deal with boolean

        } else if(curr == '(') {
            tokens.push_back({types::TokenType::LPAREN_ROUND, "("});
        } else if(curr == ')') {
            tokens.push_back({types::TokenType::RPAREN_ROUND, ")"});
        } else if(curr == '[') {
            tokens.push_back({types::TokenType::LPAREN_SQUARE, "["});
        } else if(curr == ']') {
            tokens.push_back({types::TokenType::RPAREN_SQUARE, "]"});
        } else if(curr == '{') {
            tokens.push_back({types::TokenType::LPAREN_CURLY, "{"});
        } else if(curr == '}') {
            tokens.push_back({types::TokenType::RPAREN_CURLY, "}"});
        } else if (curr == ';') {
            tokens.push_back({types::TokenType::SEMICOLON, ";"});
        } else if(curr == '\'') {
            size_t peek = lh+1;
            char ActualChar;
            bool CharSeen = false;
            bool lQuoteSeen = false;


            while(peek < SourceCode.size()) {
                if(SourceCode[peek] == '\\') {
                    peek++;
                    // need to escape whatevers after
                    // cannot end on slash
                    if(peek >= SourceCode.size()) {
                        errors.push_back("Invalid escape sequence");
                        break;
                    }

                    if(!base::isValidStringChar(SourceCode[peek])) {
                        errors.push_back("Invalid string character after escape");
                        // TODO: move till space or semicolon
                        break;
                    }

                    // check if valid escape sequence
                    if(!base::isValidStringChar(SourceCode[peek])) {
                        errors.push_back("Invalid string character after escape");
                        // TODO: move till space or semicolon
                        break;
                    }

                    // pre check if emding string
                    if(!CharSeen) {
                        ActualChar = base::SimpleEscapeSequence(SourceCode[peek]);
                        CharSeen = true;
                    }
                    }


                }
            }

            if(peek < SourceCode.size()) {
                ActualChar = SourceCode[peek];
            } else {
                ActualChar = '\0';
            }
        } else if(curr == '"') {
            //string literal
        } else if(std::isdigit(curr)) {
            // fp or int
            // mulch and regex bash
        } else {
            // this is operator
        }
    }

    return tokens;
}

} // namespace lexer
} // namespace baka
