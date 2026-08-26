#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/parser/ast/expression.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

// TODO: deal with dtypddefs by mainting symbol table

namespace baka {
    namespace parser {
        bool Parser::Check(types::TokenType type) const noexcept {
            if (current >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

            return Tokens[current].TokenType_v == type;
        }

        const types::Token& Parser::Advance() {
            return Tokens[current++];
        }

        bool Parser::Match(types::TokenType type) {
            if (Check(type)) {
                Advance();
                return true;
            }
            return false;
        }

        const types::Token& Parser::Peek() const noexcept {
            if (current >= Tokens.size()) return Tokens.back();

            return Tokens[current];
        }

        const types::Token& Parser::Previous() const {
            return Tokens[current - 1];
        }


        types::ASTNode* Parser::Parse() {
            return ParseDeclarationList();
        };

        types::ProgramNode* Parser::Program() {
            // std::vector<types::ASTNode*> body;
            // while (!Check(types::TokenType::EOF_TOKEN)) {
            //     if (Check(types::TokenType::K_STRUCT)) {
            //         body.push_back(this->Struct());
            //     }
            //     else {
            //         body.push_back(this->Function());
            //     }
            // }

            // types::ProgramNode* Node = ASTALLOC.Alloc<types::ProgramNode>(body);
            // return Node;
        }

        types::ExpressionNode* Parser::Expression() {
            // todo: does this have to be different
            return ParseCommaExpression();
        }

        types::ConstantIntNode* Parser::ParseConstantNode() {
            types::Token Token = this->Peek();
            if (!std::holds_alternative<int>(Token.Value)) {
                // throw error
            }
            this->Advance();

            types::ConstantIntNode* Node = ASTALLOC.Alloc<types::ConstantIntNode>(std::get<int>(Token.Value));
            return Node;
        }

        bool Parser::LookupType(types::IdentifierNode* Identifier) {
            return TypeLookup.Lookup(Identifier->GetName()).has_value();
        }

        void Parser::EnterScope() {
            TypeLookup.EnterNewScope();
        }

        void Parser::ExitScope() {
            TypeLookup.ExitScope();
        }

        void Parser::AddType(types::IdentifierNode* Identifier) {
            TypeLookup.AddEntry(Identifier->GetName(), ParserSTE{});
        }
    }
}
