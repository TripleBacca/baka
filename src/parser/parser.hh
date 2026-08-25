#pragma once
#include "types/parser/ast.hh"
#include "types/token/all.hh"
#include <span>
#include <cstddef>

// functions in here:
// check - check of current token is of some type
// advance - return token and move fwd
// match - advance if checks is true
// peek - returns current token
// previous - returns previously matched token



namespace baka {

namespace parser {

    class Parser {
        size_t current = 0;
        std::span<types::Token> Tokens;

        public:
        Parser(std::span<types::Token> Tokens) : Tokens(Tokens) {};
        types::ASTNode* Parse();

        // helpers
        bool Check(types::TokenType type) const noexcept;
        const types::Token& Advance();
        bool Match(types::TokenType type);
        const types::Token& Peek() const noexcept;
        const types::Token& Previous() const;

        // recursive descent nodes:
        // TODO

        types::ProgramNode* Program();
        types::FunctionNode* Function();
        types::StructNode* Struct();
        types::ReturnStatementNode* ReturnStatement();
        // this is for declarations inside a struct, im sorry
        types::StructDeclarationStatementNode* StructDeclarationStatement();
        types::ExpressionNode* Expression();
        types::UnaryExpressionNode* UnaryExpression();
        types::ConstantNode* ConstantNode();


    };

}
}
