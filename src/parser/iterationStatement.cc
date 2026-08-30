#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typedef.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace parser {
        types::ForBlockStatementNode* Parser::ForBlockStatement() {
            if (!this->Match(types::TokenType::K_FOR)) {
                assert(false);
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'for'");
                Errored = true;
            }

            EnterScope();

            std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> Decl;

            if(detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek())) {
                Decl = this->ParseDeclarationList();
            } else if (Check(types::TokenType::K_TYPEDEF)) {
                Decl = this->ParseTypedef();
            } else if (Match(types::TokenType::SEMICOLON)) {
                // empty init clause
            } else {
                auto* InitExpr = this->Expression();
                Decl = InitExpr;
                if (InitExpr == nullptr) {
                    // "expected expression" already reported deep; resync to the ';'
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                } else if (!Match(types::TokenType::SEMICOLON)) {
                    ReportError("expected ';' in 'for' statement specifier");
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
                Match(types::TokenType::SEMICOLON);
            }

            types::ExpressionNode* ComparisonExpr = nullptr;
            if (!Match(types::TokenType::SEMICOLON)) {
                ComparisonExpr = Expression();
                if (ComparisonExpr == nullptr) {
                    // "expected expression" already reported deep; resync to the ';'
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                } else if (!Match(types::TokenType::SEMICOLON)) {
                    ReportError("expected ';' in 'for' statement specifier");
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
                Match(types::TokenType::SEMICOLON);
            }

            types::ExpressionNode* Update = nullptr;
            if (!Check(types::TokenType::RPAREN_ROUND)) {
                if (!Check(types::TokenType::SEMICOLON)) {
                    Update = Expression();
                    if (Update == nullptr) {
                        // "expected expression" already reported deep; resync to the ')'
                        Errored = true;
                        SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
                    }
                }
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored && Match(types::TokenType::SEMICOLON) && Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
                if (LookupType(Label)) {
                    ReportError("identifier cannot be a type name");
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
                // leave current be; ParseStatement consumes whatever follows (e.g. the '{' body)
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ForBlockStatementNode* Node = ASTALLOC.Alloc<types::ForBlockStatementNode>(Decl, ComparisonExpr, Update, Label, Body);
            if (Errored) {
                Node->setHasError();
            }

            ExitScope();

            return Node;
        }

        types::WhileBlockStatementNode* Parser::WhileBlockStatement() {
            if (!this->Match(types::TokenType::K_WHILE)) {
                assert(false);
                // throw error
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'while'");
                Errored = true;
            }

            types::ExpressionNode* Cond = this->Expression();
            if (Cond == nullptr) {
                // "expected expression" already reported deep; resync to the ')'
                Errored = true;
                SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
            }
            Match(types::TokenType::SEMICOLON); // allow trailing semicolon


            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
                if (LookupType(Label)) {
                    ReportError("identifier cannot be a type name");
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Cond != nullptr && !Errored) {
                    ReportError("expected ')'");
                }
                // leave current be; ParseStatement consumes whatever follows (e.g. the '{' body)
            }

            types::StatementNode* Body = this->ParseStatement();
            types::WhileBlockStatementNode* Node = ASTALLOC.Alloc<types::WhileBlockStatementNode>(Cond, Label, Body);
            if (Cond == nullptr || Errored) {
                Node->setHasError();
            }
            return Node;
        }

        // fix up the nullptr returning thing, return a sentinel instead


        types::DoWhileBlockStatementNode* Parser::DoWhileBlockStatement() {
            if (!this->Match(types::TokenType::K_DO)) {
                assert(false);
                // throw error
            }

            types::StatementNode* Body = this->ParseStatement();

            bool Errored = false;
            if (!this->Match(types::TokenType::K_WHILE)) {
                ReportError("expected 'while' after 'do' body");
                Errored = true;
                if (SkipTo({types::TokenType::K_WHILE, types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON}) != types::TokenType::K_WHILE) {
                    // couldn't find a 'while' to resync to; bail out of the tail
                    types::DoWhileBlockStatementNode* Bail = ASTALLOC.Alloc<types::DoWhileBlockStatementNode>(nullptr, nullptr, Body);
                    Bail->setHasError();
                    return Bail;
                }
                this->Match(types::TokenType::K_WHILE);
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'while'");
                Errored = true;
            }

            types::ExpressionNode* Cond = nullptr;
            if (!Check(types::TokenType::RPAREN_ROUND)) {
                Cond = this->Expression();
                if (Cond == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                }
            }
            Match(types::TokenType::SEMICOLON); // allow trailing semicolon

            types::IdentifierNode* Label = nullptr;
            if (Match(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Cond != nullptr && !Errored) {
                    ReportError("expected ')'");
                }
                Errored = true;
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after 'do ... while'");
                Errored = true;
            }

            types::DoWhileBlockStatementNode* Node = ASTALLOC.Alloc<types::DoWhileBlockStatementNode>(Cond, Label, Body);
            if (Errored) {
                Node->setHasError();
            }
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
                assert(false);
                return nullptr;
            }
        }
    }
}
