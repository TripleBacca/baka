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

        types::IdentifierNode* UnionName = nullptr;
        if(Check(types::TokenType::IDENTIFIER)) {
            UnionName = ParseIdentifier();
            if(LookupType(UnionName)) {
                ReportError("redefinition of existing type");
            }
            AddType(UnionName);
        } else if(!Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected identifier or '{' after 'union' keyword");
        }

        EnterScope();

        if(!Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after union name");
        }
        types::UnionBodyNode* Body = ParseUnionBody(UnionName);
        if(!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after union body", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        types::UnionNode* Node = ASTALLOC.Alloc<types::UnionNode>(Body);

        ExitScope();

        return Node;
    }

}
}
