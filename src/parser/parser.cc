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
            return Program();
        };

        types::ProgramNode* Parser::Program() {
            std::vector<types::ASTNode*> body;
            while (!Check(types::TokenType::EOF_TOKEN)) {
                if (Check(types::TokenType::K_STRUCT)) {
                    body.push_back(this->Struct());
                }
                else {
                    body.push_back(this->Function());
                }
            }

            types::ProgramNode* Node = ASTALLOC.Alloc<types::ProgramNode>(body);
            return Node;
        }

        types::StructNode* Parser::Struct() {
            //TODO add support for declarations after struct
            //TODO semicolon?
            if (!this->Match(types::TokenType::K_STRUCT)) {
                // match struct keyword
                // throw error
            }

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }

            types::IdentiferNode* StructIdentifier = ASTALLOC.Alloc<types::IdentiferNode>(std::get<std::string_view>(this->Advance().Value));


            if (!this->Match(types::TokenType::LPAREN_CURLY)) {
                // throw error
            }
            std::vector<types::StructDeclarationStatementNode*> Body;

            while (!this->Match(types::TokenType::RPAREN_CURLY)) {
                Body.push_back(this->StructDeclarationStatement());
            }

            types::StructNode* Node = ASTALLOC.Alloc<types::StructNode>(StructIdentifier, Body);

            return Node;
        }

        types::ReturnStatementNode* Parser::ReturnStatement() {
            if (!this->Match(types::TokenType::K_RETURN)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::ReturnStatementNode* Node = ASTALLOC.Alloc<types::ReturnStatementNode>(Expression);
            return Node;
        }

        types::StructDeclarationStatementNode* Parser::StructDeclarationStatement() {
            //TODO add support for inline initialisation
            //TODO add support for comma separated declarations
            //TODO add support for struct enum etc declarations
            //TODO add support for unsigned const etc declarations
            const types::Token& DataType = this->Advance();

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            types::IdentiferNode* VariableName = ASTALLOC.Alloc<types::IdentiferNode>(std::get<std::string_view>(this->Advance().Value));

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::StructDeclarationStatementNode* Node = ASTALLOC.Alloc<types::StructDeclarationStatementNode>(
                std::get<std::string_view>(DataType.Value), VariableName);

            return Node;
        }

        types::ExpressionNode* Parser::Expression() {
            //TODO:
            // types::ExpressionNode* Unary;
            // if (this->Match(types::TokenType::LPAREN_ROUND)) {
            //     Unary = this->UnaryExpression();
            //     if (!this->Match(types::TokenType::RPAREN_ROUND)) {
            //         // throw error
            //     }
            // }
            // else {
            //     Unary = this->UnaryExpression();
            // }


            // return Unary;
        }

        // types::UnaryExpressionNode* Parser::UnaryExpression() {
        //     if (this->Match(types::TokenType::OP_UNARY_MINUS) || this->Match(types::TokenType::OP_TILDE)) {
        //         types::ASTUnaryOp Op = types::TokenTypeToASTUnaryOp[this->Previous().TokenType_v];

        //         types::ExpressionNode* Expr = this->Expression();
        //         types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(Op, Expr);
        //         return Node;
        //     }
        //     else {
        //         types::ConstantIntNode* Constant = this->ParseConstantNode();
        //         types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(
        //             types::ASTUnaryOp::OP_NOOP, Constant);

        //         return Node;
        //     }
        // }


        types::ConstantIntNode* Parser::ParseConstantNode() {
            types::Token Token = this->Peek();
            if (!std::holds_alternative<int>(Token.Value)) {
                // throw error
            }
            this->Advance();

            types::ConstantIntNode* Node = ASTALLOC.Alloc<types::ConstantIntNode>(std::get<int>(Token.Value));
            return Node;
        }


    }
}
