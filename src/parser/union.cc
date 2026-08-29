#include "types/parser/ast/union.hh"
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"


namespace baka {
namespace parser {

    types::UnionBodyNode* Parser::ParseUnionBody(types::IdentifierNode* ParentName) {
        // todo: does this have to be diff?
        return this->ParseStructBody(ParentName);
    }

    types::UnionNode* Parser::ParseUnion() {
        if(!Match(types::TokenType::K_UNION)) {
            // todo throw error;
            assert(false);
        }

        types::IdentifierNode* UnionName = ParseIdentifier();
        if(LookupType(UnionName)) {
            // todo throw error;
            assert(false);
        }
        AddType(UnionName);

        EnterScope();

        if(!Check(types::TokenType::LPAREN_CURLY)) {
            // todo throw error;
            assert(false && "expected LPAREN_CURLY");
        }
        types::UnionBodyNode* Body = ParseUnionBody(UnionName);
        if(!Match(types::TokenType::SEMICOLON)) {
            // todo throw error;
            assert(false && "expected SEMICOLON");
        }

        types::UnionNode* Node = ASTALLOC.Alloc<types::UnionNode>(Body);

        ExitScope();

        return Node;
    }

}
}
