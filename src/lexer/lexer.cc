#include "lexer.hh"
#include <cctype>
#include <string>
#include <string_view>
#include <charconv>
#include <climits>
#include "base/base.hh"
#include "memory/custom_strings.hh"
#include "types/driver/defs.hh"
#include "driver/gctx.hh"
#include "keywords.hh"
#include "operators.hh"
#include "types/token/token.hh"


#define LEX_ERROR(STR, IDX) driver::Gctx::GenerateLineError(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)
#define LEX_WARN(STR, IDX) driver::Gctx::GenerateLineWarning(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)

namespace baka {
    namespace lexer {
        std::vector<types::Token> Tokenize(std::string_view SourceCode) {
            std::vector<types::Token> tokens;

            size_t LineNo = 1;
            size_t ColNo = 1;

            size_t lh = 0;
            auto LineIndex = driver::Gctx::GetLineIndex();
            LineIndex->startLine(0);

            while (lh < SourceCode.size()) {
                char curr = SourceCode[lh];

                if (base::isValidKeywordNonDigit(curr)) {
                    // keyword or identifier
                    // deal with boolean (kword deals with this already)

                    size_t startIdx = lh;
                    size_t endIdx = startIdx;

                    size_t peek = lh + 1;
                    while (peek < SourceCode.size() &&
                        (base::isValidKeywordNonDigit(SourceCode[peek]) || std::isdigit(SourceCode[peek]))) {
                        endIdx = peek;
                        peek++;
                    }

                    std::string_view lexeme = SourceCode.substr(startIdx, endIdx - startIdx + 1);
                    if (Kw_lexeme_to_type.contains(lexeme)) {
                        tokens.emplace_back(Kw_lexeme_to_type.at(lexeme), lexeme);
                    }
                    else {
                        tokens.emplace_back(types::TokenType::IDENTIFIER, lexeme);
                    }
                    lh = endIdx + 1;
                    ColNo += endIdx - startIdx + 1;
                }
                else if (curr == '@') {
                    tokens.emplace_back(types::TokenType::AT, std::string_view("@"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '(') {
                    tokens.emplace_back(types::TokenType::LPAREN_ROUND, std::string_view("("));
                    lh++;
                    ColNo++;
                }
                else if (curr == ')') {
                    tokens.emplace_back(types::TokenType::RPAREN_ROUND, std::string_view(")"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '[') {
                    tokens.emplace_back(types::TokenType::LPAREN_SQUARE, std::string_view("["));
                    lh++;
                    ColNo++;
                }
                else if (curr == ']') {
                    tokens.emplace_back(types::TokenType::RPAREN_SQUARE, std::string_view("]"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '{') {
                    tokens.emplace_back(types::TokenType::LPAREN_CURLY, std::string_view("{"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '}') {
                    tokens.emplace_back(types::TokenType::RPAREN_CURLY, std::string_view("}"));
                    lh++;
                    ColNo++;
                }
                else if (curr == ';') {
                    tokens.emplace_back(types::TokenType::SEMICOLON, std::string_view(";"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '\'') {
                    // character literal

                    size_t orgLh = lh; // need for col no calcs for error

                    size_t peek = lh + 1;
                    char ActualChar = '\0';
                    bool CharSeen = false;
                    size_t CharLength = 0;
                    bool RQuoteSeen = false;

                    while (peek < SourceCode.size()) {
                        if (SourceCode[peek] == '\\') {
                            // possibly escape seq
                            peek++;
                            if (peek >= SourceCode.size()) {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                break;
                            }

                            char q = SourceCode[peek];
                            // q must either be a letter for a simple escape
                            // or
                            // q must be a number 0-7 for octal escape seq


                            if (base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                                ActualChar = base::SimpleEscapeSequence(q);
                                CharSeen = true;
                                CharLength++;
                            }
                            else if (std::isdigit(q) && (0 <= (q - '0')) && (7 >= (q - '0'))) {
                                // octal escape sequence
                                // need to do this for \0

                                long long octal_seq = (q - '0');
                                peek++;

                                // octal escape seq is max 3 digits
                                size_t DigitsSeen = 1;

                                while (peek < SourceCode.size() &&
                                    SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                                    DigitsSeen < 3) {
                                    octal_seq *= 8;
                                    octal_seq += (SourceCode[peek] - '0');
                                    DigitsSeen++;
                                    peek++;
                                }
                                peek--; // move it back to last scanned char

                                ActualChar = static_cast<char>(octal_seq);
                                CharSeen = true;
                                CharLength++;
                            }
                            else {
                                // TODO: is line and column even updated????
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                // break; dont break here and search for closing quote
                            }
                        }
                        else if (base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '\'') {
                            // normal ascii char
                            ActualChar = SourceCode[peek];
                            CharSeen = true;
                            CharLength++;
                        }
                        else if (SourceCode[peek] == '\'') {
                            // end literal here
                            RQuoteSeen = true;
                            break;
                        }
                        else {
                            LEX_ERROR("Invalid character in character literal", ColNo + peek - orgLh);
                            // break; no break here to scan for end quote
                        }
                        peek++;
                    }

                    size_t Oldlh = lh;
                    size_t orgColNo = ColNo;
                    lh = ++peek;
                    ColNo += lh - Oldlh;

                    if (RQuoteSeen && CharSeen) {
                        tokens.emplace_back(types::TokenType::LITERAL_CHARACTER, ActualChar);

                        if (CharLength > 1) {
                            LEX_WARN(
                                "Multilength character literal will always chose the last character as its representation.",
                                ColNo-1);
                        }
                    }
                    else {
                        if (!CharSeen) {
                            LEX_ERROR("no character in character literal", ColNo-1);
                        }
                        if (!RQuoteSeen) {
                            LEX_ERROR("character literal not terminated", orgColNo);
                        }
                        tokens.emplace_back(types::TokenType::UNKNOWN, ActualChar);
                    }
                }
                else if (curr == '"') {
                    //string literal

                    size_t orgLh = lh; // need for col no calcs for error

                    size_t peek = lh + 1;

                    base::SLString str; // arena alloc

                    bool RQuoteSeen = false;

                    while (peek < SourceCode.size()) {
                        if (SourceCode[peek] == '\\') {
                            // escape sequence

                            peek++;
                            if (peek >= SourceCode.size()) {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                break;
                            }

                            char q = SourceCode[peek];
                            // q is either letter for escape seq
                            // or
                            // q is 0-7 for octal escape seq

                            if (base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                                // letter escape seq
                                char ToAdd = base::SimpleEscapeSequence(q);
                                str += (ToAdd);
                            }
                            else if (std::isdigit(q) && (0 <= (q - '0')) && (7 >= (q - '0'))) {
                                // octal escape sequence
                                // need to do this for \0

                                long long octal_seq = (q - '0');
                                peek++;

                                // octal escape seq has max 3 digits
                                size_t DigitsSeen = 1;

                                while (peek < SourceCode.size() &&
                                    SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                                    DigitsSeen < 3) {
                                    octal_seq *= 8;
                                    octal_seq += (SourceCode[peek] - '0');
                                    DigitsSeen++;
                                    peek++;
                                }
                                peek--; // move it back to last scanned char

                                str += static_cast<char>(octal_seq);
                            }
                            else {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                // break; dont break here and search for closing quote
                            }
                        }
                        else if (base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '"') {
                            // normal ascii char
                            str += SourceCode[peek];
                        }
                        else if (SourceCode[peek] == '"') {
                            // end literal here
                            RQuoteSeen = true;
                            break;
                        }
                        else {
                            LEX_ERROR("Invalid character in string literal", ColNo + peek - orgLh);
                            // break; no break here to scan for end quote
                        }
                        peek++;
                    }

                    size_t Oldlh = lh;
                    size_t orgColNo = ColNo;
                    lh = ++peek;
                    ColNo += lh - Oldlh;

                    if (RQuoteSeen) {
                        tokens.emplace_back(types::TokenType::LITERAL_STRING, std::move(str));
                    }
                    else {
                        // must be invalid char
                        LEX_ERROR("string literal not terminated", orgColNo);
                        tokens.emplace_back(types::TokenType::UNKNOWN, std::move(str));
                    }
                }
                else if (std::isdigit(curr) ||
                    (curr == '.' &&
                        (lh + 1 < SourceCode.size()) && std::isdigit(SourceCode[lh + 1]))) {
                    types::NumericLiteralType numType = types::NumericLiteralType::INT;

                    //need this stuff for error reporting
                    size_t orgLh = lh;
                    size_t orgColNo = ColNo;

                    size_t peek = lh + 1;
                    size_t startIdx = lh;

                    bool dotSeen = false;
                    bool eSeen = false;
                    bool uSeen = false;
                    bool lSeen = false;
                    bool fSeen = false;

                    bool chatIsThisOctLarp = false;
                    size_t larpLoc = -1;
                    int nextDigitBase = 10;
                    int numBase = 10;


                    if (curr == '0' && (lh + 1 < SourceCode.size())) {
                        if (std::tolower(SourceCode[lh + 1]) == 'b') {
                            nextDigitBase = 2;
                            numBase = 2;
                            peek++;
                        }
                        else if (std::tolower(SourceCode[lh + 1]) == 'x') {
                            nextDigitBase = 16;
                            numBase = 16;
                            peek++;
                        }
                        else {
                            nextDigitBase = 8;
                            numBase = 8;
                        }
                    }

                    if (curr == '.') {
                        dotSeen = true;
                    }

                    while (peek < SourceCode.size()) {
                        if (base::isValidDigit(SourceCode[peek], nextDigitBase)) {
                            peek++;
                        }
                        else if (nextDigitBase == 8 && std::isdigit(SourceCode[peek])) {
                            chatIsThisOctLarp = true;
                            larpLoc = peek;
                            nextDigitBase = 10;
                        }
                        else if (SourceCode[peek] == '.' && (nextDigitBase == 8 || nextDigitBase == 10 || nextDigitBase
                                == 16) && !dotSeen
                            && !eSeen) {
                            if (nextDigitBase == 8) {
                                nextDigitBase = 10;
                            }
                            dotSeen = true;
                            peek++;
                        }
                        else if (((std::tolower(SourceCode[peek]) == 'p' && nextDigitBase == 16) ||
                                (std::tolower(SourceCode[peek]) == 'e' && nextDigitBase == 10)) && peek + 1 < SourceCode
                            .
                            size() &&
                            ((!eSeen && std::isdigit(SourceCode[peek + 1])) || (!eSeen && (SourceCode[peek + 1] == '-'
                                || SourceCode[peek + 1] == '+') && (peek + 2 <
                                SourceCode.size()) && std::isdigit(SourceCode[peek + 2])))) {
                            eSeen = true;
                            nextDigitBase = 10;
                            if (base::isValidDigit(SourceCode[peek + 1], nextDigitBase)) {
                                peek++;
                            }
                            else {
                                peek += 2;
                            }
                        }
                        else if (std::tolower(SourceCode[peek]) == 'u' && !dotSeen && !eSeen && !uSeen && !fSeen) {
                            uSeen = true;
                            peek++;
                        }
                        else if (std::tolower(SourceCode[peek]) == 'l' && !dotSeen && !eSeen && !lSeen && !fSeen) {
                            lSeen = true;
                            peek++;
                        }
                        else if (std::tolower(SourceCode[peek]) == 'f' && (dotSeen || eSeen) && !lSeen && !fSeen) {
                            fSeen = true;
                            peek++;
                        }
                        else {
                            break;
                        }
                    }


                    lh += peek - startIdx;
                    ColNo += peek - startIdx;
                    if (dotSeen || eSeen) {
                        if (fSeen) {
                            numType = types::NumericLiteralType::FLOAT;
                        }
                        else {
                            numType = types::NumericLiteralType::DOUBLE;
                        }
                    }
                    else {
                        if (chatIsThisOctLarp) {
                            LEX_ERROR(std::string("Invalid digit '") + SourceCode[larpLoc] + "' in octal number",
                                      orgColNo + (larpLoc - orgLh));
                        }
                        if (uSeen) {
                            if (lSeen) {
                                numType = types::NumericLiteralType::UNSIGNED_LONG;
                            }
                            else {
                                numType = types::NumericLiteralType::UNSIGNED_INT;
                            }
                        }
                        else {
                            if (lSeen) {
                                numType = types::NumericLiteralType::LONG;
                            }
                            else {
                                numType = types::NumericLiteralType::INT;
                            }
                        }
                    }

                    switch (numType) {
                    case types::NumericLiteralType::INT:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > INT_MAX) {
                                    LEX_ERROR("Integer literal overflows int", orgColNo);
                                }
                                else {
                                    tokens.emplace_back(types::TokenType::LITERAL_INTEGER, static_cast<int>(x));
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::LONG:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > LLONG_MAX) {
                                    LEX_ERROR("Integer literal overflows long", orgColNo);
                                }
                                else {
                                    tokens.emplace_back(types::TokenType::LITERAL_INTEGER, static_cast<long long>(x));
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::UNSIGNED_INT:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > UINT_MAX) {
                                    LEX_ERROR("Integer literal overflows unsigned int", orgColNo);
                                }
                                else {
                                    tokens.emplace_back(types::TokenType::LITERAL_INTEGER,
                                                        static_cast<unsigned int>(x));
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::UNSIGNED_LONG:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                tokens.emplace_back(types::TokenType::LITERAL_INTEGER, x);
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::FLOAT:
                        {
                            std::chars_format fmtBase = std::chars_format::general;
                            if (numBase == 16) {
                                startIdx += 2;
                                fmtBase = std::chars_format::hex;
                            }
                            double x = 0;
                            auto [ptr, ec] =
                                std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x, fmtBase);
                            if (ec == std::errc()) {
                                tokens.emplace_back(types::TokenType::LITERAL_FP, static_cast<float>(x));
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::DOUBLE:
                        {
                            std::chars_format fmtBase = std::chars_format::general;
                            if (numBase == 16) {
                                startIdx += 2;
                                fmtBase = std::chars_format::hex;
                            }
                            double x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             fmtBase);
                            if (ec == std::errc()) {
                                tokens.emplace_back(types::TokenType::LITERAL_FP, x);
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    }
                }
                else if (curr == ' ' || curr == '\t') {
                    lh++;
                    ColNo++;
                }
                else if (curr == '\n') {
                    LineNo++;

                    LineIndex->endLine(lh);
                    lh++;
                    LineIndex->startLine(lh);

                    ColNo = 1;
                }
                else if(SourceCode[lh] == '/' && (lh + 1 < SourceCode.size()) && SourceCode[lh+1] == '/') {
                    // skip comment
                    while (lh < SourceCode.size() && SourceCode[lh] != '\n') {
                        lh++;
                        ColNo++;
                    }
                }
                else if(SourceCode[lh] == '/' && (lh + 1 < SourceCode.size()) && SourceCode[lh + 1] == '*') {
                    // skip block comment
                    lh+=2;
                    ColNo+=2;

                    size_t orgLineNo = LineNo;
                    size_t orgColNo = ColNo-1;
                    auto orgLine = LineIndex->CurrentLine();

                    bool blockEndFound = false;
                    while (lh < SourceCode.size()) {
                        if(SourceCode[lh] == '*' && (lh + 1) < SourceCode.size() && SourceCode[lh + 1] == '/') {
                            blockEndFound = true;
                            ColNo += 2;
                            lh += 2;
                            break;
                        }

                        if(SourceCode[lh] == '\n') {
                            LineNo++;

                            LineIndex->endLine(lh);
                            lh++;
                            LineIndex->startLine(lh);

                            ColNo = 1;
                        } else {
                            lh++;
                            ColNo++;
                        }
                    }
                    if(!blockEndFound) {
                        driver::Gctx::GenerateLineError(orgLineNo, orgColNo, orgLine, "Block comment not terminated" , driver::Stage::LEX);
                    }
                }
                else if (base::isValidOperatorChar(curr)) {
                    // this is operator
                    if (curr == '.') {
                        // check for ellipsis
                        if (lh + 2 < SourceCode.size() && SourceCode[lh + 1] == '.' && SourceCode[lh + 2] == '.') {
                            tokens.emplace_back(types::TokenType::OP_ELLIPSIS,
                                                std::string_view(SourceCode.substr(lh, 3)));
                            lh += 3;
                            ColNo += 3;
                        }
                        else {
                            tokens.emplace_back(types::TokenType::OP_DOT, std::string_view(SourceCode.substr(lh, 1)));
                            lh++;
                            ColNo++;
                        }
                    }
                    else {
                        size_t startIdx = lh;
                        size_t endIdx = startIdx;

                        size_t peek = lh + 1;
                        while (peek < SourceCode.size() && base::isValidOperatorChar(SourceCode[peek]) &&
                            lexeme_to_operator.contains(SourceCode.substr(startIdx, peek - startIdx + 1))) {
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
                    LEX_ERROR("Unknown token", ColNo);

                    tokens.emplace_back(types::TokenType::UNKNOWN, std::string_view(SourceCode.data() + lh, 1));
                    lh++;
                    ColNo++;
                }
            }
            LineIndex->endLine(lh);
            tokens.emplace_back(types::TokenType::EOF_TOKEN, std::string_view());

            return tokens;
        }
    } // namespace lexer
} // namespace baka
