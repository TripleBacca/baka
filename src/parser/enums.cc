#include "types/parser/ast/enums.hh"
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>

enum {
    BALLS,
    EAT
};

namespace baka {
namespace parser {

    types::EnumNode* Parser::ParseEnumDecl() {
        if(!Match(types::TokenType::K_ENUM)) {
            // todo throw error
            assert(false);
        }

        types::IdentifierNode* EnumName = nullptr;
        if(!Check(types::TokenType::LPAREN_CURLY)) { // supporting anonymous enums
            if(!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier");
            } else {
                EnumName = ParseIdentifier();
                // enums cannot have fwd decl btw
                if(LookupType(EnumName->GetName())) {
                    ReportError("redefinition of existing type");
                }
                AddType(EnumName);
            }
        }

        types::EnumNode* Enum = ASTALLOC.Alloc<types::EnumNode>(EnumName);

        if(!Match(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after enum name");
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
            return Enum;
        }

        do {
            if(!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier in enumeration");
                SkipTo({types::TokenType::OP_COMMA, types::TokenType::RPAREN_CURLY});
            } else {
                types::IdentifierNode* EnumValue = ParseIdentifier();
                Enum->addEnumValue(EnumValue);
            }

            if(!Match(types::TokenType::OP_COMMA)) {
                if (!Match(types::TokenType::RPAREN_CURLY)) {
                    ReportError("expected '}' or ',' in enumeration");
                    SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                }
                break;
            }

        } while(!Match(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN));

        if(!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after enum definition", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        return Enum;
    }

}
}
