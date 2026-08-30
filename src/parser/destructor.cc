#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/statement.hh"
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
                if (!Check(types::TokenType::LPAREN_CURLY) &&
                    SkipTo({types::TokenType::LPAREN_CURLY, types::TokenType::SEMICOLON}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            } else if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected ')' after destructor parameter list");
                if (!Check(types::TokenType::LPAREN_CURLY) &&
                    SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::RPAREN_ROUND) {
                    Advance();
                }
            }

            types::StatementNode* Body = nullptr;
            if (Check(types::TokenType::LPAREN_CURLY)) {
                Body = this->CompoundStatement();
            } else {
                ReportError("expected '{' for destructor body");
                std::vector<std::variant<types::StatementNode*, types::DeclarationList*, types::TypedefNode*, types::FunctionNode*>> EmptyBody;
                Body = ASTALLOC.Alloc<types::CompoundStatementNode>(std::move(EmptyBody));
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }
            auto* Node = ASTALLOC.Alloc<types::DestructorNode>(Body);

            return Node;
        }
    }
}
