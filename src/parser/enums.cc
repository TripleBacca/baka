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
            EnumName = ParseIdentifier();
            // enums cannot have fwd decl btw

            if(LookupType(EnumName->GetName())) {
                // todo throw error
                assert(false);
            }
            AddType(EnumName);
        }

        types::EnumNode* Enum = ASTALLOC.Alloc<types::EnumNode>(EnumName);

        if(!Match(types::TokenType::LPAREN_CURLY)) {
            // todo throw error
            assert(false);
        }

        do {
            types::IdentifierNode* EnumValue = ParseIdentifier();
            Enum->addEnumValue(EnumValue);

            if(!Match(types::TokenType::OP_COMMA)) {
                if (!Match(types::TokenType::RPAREN_CURLY)) {
                    // todo throw error
                    assert(false);
                }
                break;
            }

        } while(!Match(types::TokenType::RPAREN_CURLY));

        if(!Match(types::TokenType::SEMICOLON)) {
            // todo throw error
            assert(false);
        }

        return Enum;
    }

}
}
