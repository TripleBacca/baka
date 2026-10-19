#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typedef.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace parser {

        // A loop suffix is ';' IDENTIFIER, immediately before the closing ')'.
        // It is never a runtime expression: `while (x; label)` and `do { } while (x; label);`.
        types::IdentifierNode* Parser::ParseLoopSuffixLabel() {
            if (!Match(types::TokenType::SEMICOLON)) return nullptr;
            if (!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected loop label identifier after ';'");
                return nullptr;
            }
            auto* Label = ParseIdentifier();
            if (Label && LookupType(Label)) {
                ReportError("identifier cannot be a type name");
            }
            return Label;
        }

        types::ForBlockStatementNode* Parser::ForBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_FOR)) {
                assert(false);
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'for'");
                Errored = true;
            }

            EnterScope();

            // init-clause ';' condition? ';' update? (';' label)? ')'
            std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> Decl;

            if (Check(types::TokenType::SEMICOLON)) {
                Advance();
            } else if (detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek())) {
                Decl = this->ParseDeclarationList();
            } else if (Check(types::TokenType::K_TYPEDEF)) {
                Decl = this->ParseTypedef();
            } else {
                auto* InitExpr = this->Expression();
                if (InitExpr == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                } else {
                    Decl = InitExpr;
                }
                if (!Match(types::TokenType::SEMICOLON)) {
                    ReportError("expected ';' in 'for' statement specifier");
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                    Match(types::TokenType::SEMICOLON);
                }
            }

            types::ExpressionNode* ComparisonExpr = nullptr;
            if (!Check(types::TokenType::SEMICOLON)) {
                ComparisonExpr = Expression();
                if (ComparisonExpr == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' in 'for' statement specifier");
                Errored = true;
                SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                Match(types::TokenType::SEMICOLON);
            }

            types::ExpressionNode* Update = nullptr;
            if (!Check(types::TokenType::RPAREN_ROUND) && !Check(types::TokenType::SEMICOLON)) {
                Update = Expression();
                if (Update == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            auto* Node = AllocNodeAt<types::ForBlockStatementNode>(SourceStart, Decl, ComparisonExpr, Update, Label, Body);
            if (Errored) {
                Node->setHasError();
            }

            ExitScope();

            return Node;
        }

        types::WhileBlockStatementNode* Parser::WhileBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_WHILE)) {
                assert(false);
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'while'");
                Errored = true;
            }

            types::ExpressionNode* Cond = this->Expression();
            if (Cond == nullptr) {
                Errored = true;
                SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();
            auto* Node = AllocNodeAt<types::WhileBlockStatementNode>(SourceStart, Cond, Label, Body);
            if (Errored) {
                Node->setHasError();
            }
            return Node;
        }

        types::DoWhileBlockStatementNode* Parser::DoWhileBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_DO)) {
                assert(false);
            }

            types::StatementNode* Body = this->ParseStatement();

            bool Errored = false;
            if (!this->Match(types::TokenType::K_WHILE)) {
                ReportError("expected 'while' after 'do' body");
                Errored = true;
                if (SkipTo({types::TokenType::K_WHILE, types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON}) != types::TokenType::K_WHILE) {
                    auto* Bail = AllocNodeAt<types::DoWhileBlockStatementNode>(SourceStart, nullptr, nullptr, Body);
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
            if (Check(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected expression");
                Errored = true;
            } else {
                Cond = this->Expression();
                if (Cond == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                }
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
                Errored = true;
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after 'do ... while'", true);
                Errored = true;
            }

            auto* Node = AllocNodeAt<types::DoWhileBlockStatementNode>(SourceStart, Cond, Label, Body);
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
                assert(false);
                return nullptr;
            }
        }
    }
}
