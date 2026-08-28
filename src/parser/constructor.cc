#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ConstructorNode* Parser::ParseConstructor(types::IdentifierNode* ParentName) {
            types::IdentifierNode* ConstructorIdentifier = this->ParseIdentifier();
            if (ConstructorIdentifier->GetName() != ParentName->GetName()) {
                assert(false);
                // todo throw error
            }


            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
                assert(false);
            }

            types::FunctionParameterList* Args = this->ParseFunctionParameterList();
            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
                assert(false);
            }

            types::StatementNode* Body = this->CompoundStatement();
            auto* Node = ASTALLOC.Alloc<types::ConstructorNode>(Args, Body);

            return Node;
        }
    }
}
