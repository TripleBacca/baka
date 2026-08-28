#include "parser.hh"
#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/token/pretty.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace parser {
        types::ForBlockStatementNode* Parser::ForBlockStatement() {
            if (!this->Match(types::TokenType::K_FOR)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            std::variant<types::ExpressionNode*, types::DeclarationList*> Decl;
            if(detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek())) {
                Decl = this->ParseDeclarationList();
            } else {
                Decl = this->Expression();
                if (!Match(types::TokenType::SEMICOLON)) {
                    // todo throw error
                    assert(false);
                }
            }

            types::ExpressionNode* ComparisonExpr = Expression();
            if (!Match(types::TokenType::SEMICOLON)) {
                // throw error
                assert(false);
            }

            types::ExpressionNode* Update = Expression();

            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::SEMICOLON) && Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
                if (LookupType(Label)) {
                    // todo throw error
                    assert(false);
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
                assert(false);
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ForBlockStatementNode* Node = ASTALLOC.Alloc<types::ForBlockStatementNode>(Decl, ComparisonExpr, Update, Label, Body);
            return Node;
        }

        types::WhileBlockStatementNode* Parser::WhileBlockStatement() {
            if (!this->Match(types::TokenType::K_WHILE)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Cond = this->Expression();
            Match(types::TokenType::SEMICOLON); // allow trailing semicolon


            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            types::StatementNode* Body = this->ParseStatement();
            types::WhileBlockStatementNode* Node = ASTALLOC.Alloc<types::WhileBlockStatementNode>(Cond, Label, Body);
            return Node;
        }

        types::DoWhileBlockStatementNode* Parser::DoWhileBlockStatement() {
            if (!this->Match(types::TokenType::K_DO)) {
                // throw error
            }

            types::StatementNode* Body = this->ParseStatement();

            if (!this->Match(types::TokenType::K_WHILE)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Cond = this->Expression();
            Match(types::TokenType::SEMICOLON); // allow trailing semicolon

            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::DoWhileBlockStatementNode* Node = ASTALLOC.Alloc<types::DoWhileBlockStatementNode>(Cond, Label, Body);
            return Node;
        }

        types::IterationStatementNode* Parser::IterationStatement() {
            if (this->Check(types::TokenType::K_FOR)) {
                return ForBlockStatement();
            }
            else if (this->Check(types::TokenType::K_WHILE)) {
                return WhileBlockStatement();
            }
            else if (this->Check(types::TokenType::K_DO)) {
                return DoWhileBlockStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
