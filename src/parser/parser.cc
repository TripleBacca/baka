#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/parser/ast.hh"
#include "types/token/token.hh"
#include <string_view>
#include <variant>

#define ASTALLOC base::ASTNodeArena::getInstance()

namespace baka{
namespace parser{
    bool Parser::Check(types::TokenType type) const noexcept {
        if(current >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

        return Tokens[current].TokenType_v == type;
    }

    const types::Token& Parser::Advance() {
        return Tokens[current++];
    }

    bool Parser::Match(types::TokenType type) {
        if(Check(type)) {
            Advance();
            return true;
        }
        return false;
    }

    const types::Token& Parser::Peek() const noexcept {
        if(current >= Tokens.size()) return Tokens.back();

        return Tokens[current];
    }

    const types::Token& Parser::Previous() const {
        return Tokens[current - 1];
    }



    types::ASTNode* Parser::Parse() {
        return Program();
    };

    types::ProgramNode* Parser::Program() {
        types::FunctionNode* Function = this->Function();

        types::ProgramNode* Node = ASTALLOC.Alloc<types::ProgramNode>(Function);
        return Node;
    }

    types::FunctionNode* Parser::Function() {
        this->Advance(); // return type

        if(!this->Check(types::TokenType::IDENTIFIER)) {
            // throw error
        }
        const types::Token& FunctionIdentifier = this->Advance();
        if(!std::holds_alternative<std::string_view>(FunctionIdentifier.Value)) {
            // throw error
        }


        if(!this->Match(types::TokenType::LPAREN_ROUND)) {
            // throw error
        }

        // call decl list

        if(!this->Match(types::TokenType::RPAREN_ROUND)) {
            // throw error
        }


        if(!this->Match(types::TokenType::LPAREN_CURLY)) {
            // throw error
        }
        types::ReturnStatementNode* Body = this->ReturnStatement();
        if(!this->Match(types::TokenType::RPAREN_CURLY)) {
            // throw error
        }

        types::FunctionNode* Node = ASTALLOC.Alloc<types::FunctionNode>(std::get<std::string_view>(FunctionIdentifier.Value), Body);

        return Node;
    }

    types::ReturnStatementNode* Parser::ReturnStatement() {
        if(!this->Match(types::TokenType::K_RETURN)) {
            // throw error
        }

        types::ExpressionNode* Expression = this->Expression();

        if(!this->Match(types::TokenType::SEMICOLON)) {
            // throw error
        }

        types::ReturnStatementNode* Node = ASTALLOC.Alloc<types::ReturnStatementNode>(Expression);
        return Node;
    }

    types::ExpressionNode* Parser::Expression() {
        types::ExpressionNode* Unary;
        if(this->Match(types::TokenType::LPAREN_ROUND)) {
            Unary = this->UnaryExpression();
            if(!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }
        } else {
            Unary = this->UnaryExpression();
        }


        return Unary;
    }

    types::UnaryExpressionNode* Parser::UnaryExpression() {
        if(this->Match(types::TokenType::OP_UNARY_MINUS) || this->Match(types::TokenType::OP_TILDE)) {
            types::ASTUnaryOp Op = types::TokenTypeToASTUnaryOp[this->Previous().TokenType_v];

            types::ExpressionNode* Expr = this->Expression();
            types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(Op, Expr);
            return Node;
        } else {
            types::ConstantNode* Constant = this->ConstantNode();
            types::UnaryExpressionNode* Node = ASTALLOC.Alloc<types::UnaryExpressionNode>(types::ASTUnaryOp::OP_NOOP, Constant);

            return Node;
        }

    }

    types::ConstantNode* Parser::ConstantNode() {
        types::Token Token = this->Peek();
        if(!std::holds_alternative<int>(Token.Value)) {
            // throw error
        }
        this->Advance();

        types::ConstantNode* Node = ASTALLOC.Alloc<types::ConstantNode>(std::get<int>(Token.Value));
        return Node;
    }

}
}
