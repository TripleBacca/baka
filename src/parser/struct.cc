#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
namespace parser {


    types::StructNode* Parser::ParseStruct() {
        //TODO add support for declarations after struct - not doing
        //TODO semicolon?

        if (!this->Match(types::TokenType::K_STRUCT)) {
            // match struct keyword
            // todo throw error
        }

        if (!this->Check(types::TokenType::IDENTIFIER)) {
            // todo throw error
        }
        types::IdentifierNode* StructIdentifier = this->ParseIdentifier();
        if(!LookupType(StructIdentifier)) {
            // todo throw error
        }

        AddType(StructIdentifier);
        EnterScope();


        if (!this->Match(types::TokenType::LPAREN_CURLY)) {
            // todo throw error
        }
        std::vector<types::DeclarationList*> Body;

        while (!this->Match(types::TokenType::RPAREN_CURLY)) {
            Body.push_back(this->ParseDeclarationList());
        }

        types::StructNode* Node = ASTALLOC.Alloc<types::StructNode>(StructIdentifier, std::move(Body));

        if(!Match(types::TokenType::SEMICOLON)) {
            // todo put error
            assert(false);
        }

        return Node;
    }

}
}
