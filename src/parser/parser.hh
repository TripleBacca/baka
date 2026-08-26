#pragma once
#include "types/parser/ast/declaration.hh"
#include "types/token/all.hh"
#include <span>
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/jumpStatement.hh"

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

        types::ProgramNode* Program();
        types::FunctionNode* Function();
        types::FunctionArgumentsNode* FunctionArguments();
        types::FunctionArgumentStatementNode* FunctionArgumentStatement();
        types::StructNode* Struct();
        types::JumpStatementNode* JumpStatement();
        types::ReturnStatementNode* ReturnStatement();
        types::GotoStatementNode* GotoStatement();
        types::BreakStatementNode* BreakStatement();
        types::ContinueStatementNode* ContinueStatement();
        // this is for declarations inside a struct, im sorry
        types::StructDeclarationStatementNode* StructDeclarationStatement();
        types::ExpressionNode* Expression();
        // types::UnaryExpressionNode* UnaryExpression();
        types::ConstantIntNode* ParseConstantNode();


        // expressions:
        types::ExpressionNode* ParseCommaExpression();

        types::ExpressionNode* ParsePrimaryExpression();

        bool IsUnaryOperator(types::TokenType type) const noexcept;
        types::ExpressionNode* ParseFactor();

        // for ++, -- , [], ->, .
        types::ExpressionNode* ParsePostfixExpression();
        types::ExpressionNode* ParseAssignmentExpression(size_t MinPrecedence = 1);
        types::IdentifierNode* ParseIdentifier();

        types::DeclarationList* ParseDeclarationList();
        types::SingleDeclarationNode* ParseSingleDeclaration();
        types::DeclarationIdentifierNode* ParseDeclarationIdentifier();
    };

}
}
