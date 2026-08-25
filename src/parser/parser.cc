#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/parser/ast.hh"
#include "types/token/token.hh"
#include <string_view>
#include <variant>

// TODO: deal with dtypddefs by mainting symbol table
//
#define ASTALLOC base::ASTNodeArena::getInstance()

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

        types::FunctionNode* Parser::Function() {
            const types::Token& ReturnType = this->Advance();

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            const types::Token& FunctionIdentifier = this->Advance();
            if (!std::holds_alternative<std::string_view>(FunctionIdentifier.Value)) {
                // throw error
            }


            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            // call decl list

            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }


            if (!this->Match(types::TokenType::LPAREN_CURLY)) {
                // throw error
            }
            types::ReturnStatementNode* Body = this->ReturnStatement();
            if (!this->Match(types::TokenType::RPAREN_CURLY)) {
                // throw error
            }
            types::FunctionNode* Node = ASTALLOC.Alloc<types::FunctionNode>(
                std::get<std::string_view>(ReturnType.Value), std::get<std::string_view>(FunctionIdentifier.Value),
                Body);

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
            const types::Token& StructIdentifier = this->Advance();
            if (!std::holds_alternative<std::string_view>(StructIdentifier.Value)) {
                // throw error
            }


            if (!this->Match(types::TokenType::LPAREN_CURLY)) {
                // throw error
            }
            std::vector<types::StructDeclarationStatementNode*> Body;

            while (!this->Match(types::TokenType::RPAREN_CURLY)) {
                Body.push_back(this->StructDeclarationStatement());
            }

            types::StructNode* Node = ASTALLOC.Alloc<types::StructNode>(
                std::get<std::string_view>(StructIdentifier.Value), Body);

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
            const types::Token& DataType = this->Advance();
            const types::Token& VariableName = this->Advance();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                // throw error
            }

            types::StructDeclarationStatementNode* Node = ASTALLOC.Alloc<types::StructDeclarationStatementNode>(
                std::get<std::string_view>(DataType.Value), std::get<std::string_view>(VariableName.Value));

            return Node;
        }

        types::ExpressionNode* Parser::Expression() {
            types::ExpressionNode* Unary;
            if (this->Match(types::TokenType::LPAREN_ROUND)) {
                Unary = this->UnaryExpression();
                if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                    // throw error
                }
            }
            else {
                Unary = this->UnaryExpression();
            }


            return Unary;
        }

        types::UnaryExpressionNode* Parser::UnaryExpression() {
            if (this->Match(types::TokenType::OP_UNARY_MINUS) || this->Match(types::TokenType::OP_TILDE)) {
                types::ASTUnaryOp Op = types::TokenTypeToASTUnaryOp[this->Previous().TokenType_v];

                types::ExpressionNode* Expr = this->Expression();
                types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(Op, Expr);
                return Node;
            }
            else {
                types::ConstantNode* Constant = this->ConstantNode();
                types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(
                    types::ASTUnaryOp::OP_NOOP, Constant);

                return Node;
            }
        }

        types::ConstantNode* Parser::ConstantNode() {
            types::Token Token = this->Peek();
            if (!std::holds_alternative<int>(Token.Value)) {
                // throw error
            }
            this->Advance();

            types::ConstantNode* Node = ASTALLOC.Alloc<types::ConstantNode>(std::get<int>(Token.Value));
            return Node;
        }
    }
}
