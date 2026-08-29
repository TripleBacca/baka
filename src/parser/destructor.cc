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

            types::IdentifierNode* DestructorIdentifier = this->ParseIdentifier();
            if(DestructorIdentifier->GetName() != ParentName->GetName()) {
                // throw error
                assert(false);
            }

            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                assert(false);
                // throw error
            }

            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                assert(false);
                // throw error
            }

            types::StatementNode* Body = this->CompoundStatement();
            auto* Node = ASTALLOC.Alloc<types::DestructorNode>(Body);

            return Node;
        }
    }
}
