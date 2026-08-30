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
                ReportError("constructor name does not match the type name");
            }


            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after constructor name");
            }

            types::FunctionParameterList* Args = this->ParseFunctionParameterList();
            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected ')' after constructor parameter list");
                if (SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::RPAREN_ROUND) {
                    Advance();
                }
            }

            types::StatementNode* Body = this->CompoundStatement();
            auto* Node = ASTALLOC.Alloc<types::ConstructorNode>(Args, Body);

            return Node;
        }
    }
}
