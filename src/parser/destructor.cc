#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::DestructorNode* Parser::ParseDestructor(types::IdentifierNode* ParentName) {
            if (!this->Match(types::TokenType::OP_TILDE)) {
                assert(false);
                // throw error
            }

            types::IdentifierNode* DestructorIdentifier = nullptr;
            if(!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after '~'");
            } else {
                DestructorIdentifier = this->ParseIdentifier();
            }
            if(DestructorIdentifier && DestructorIdentifier->GetName() != ParentName->GetName()) {
                ReportError("destructor name does not match the type name");
            }

            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after destructor name");
            }

            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected ')' after destructor parameter list");
                if (SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::RPAREN_ROUND) {
                    Advance();
                }
            }

            types::StatementNode* Body = this->CompoundStatement();
            auto* Node = ASTALLOC.Alloc<types::DestructorNode>(Body);

            return Node;
        }
    }
}
