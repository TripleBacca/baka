#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::DestructorNode* Parser::ParseDestructor() {
            if (!this->Match(types::TokenType::OP_TILDE)) {
                // throw error
            }

            types::IdentifierNode* DestructorIdentifier = this->ParseIdentifier();

            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            // TODO: do statement
            types::StatementNode* Body = this->ParseStatement();
            auto* Node = ASTALLOC.Alloc<types::DestructorNode>(DestructorIdentifier, Body);

            return Node;
        }
    }
}
