#pragma once
#include "types/token/all.hh"
#include <span>
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/labelStatement.hh"
#include "types/parser/ast/jumpStatement.hh"
#include "types/parser/ast/selectionStatement.hh"
#include "types/parser/ast/iterationStatement.hh"

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
        bool Check2(types::TokenType type) const noexcept;
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
        types::StatementNode* Statement();
        types::JumpStatementNode* JumpStatement();
        types::ReturnStatementNode* ReturnStatement();
        types::GotoStatementNode* GotoStatement();
        types::BreakStatementNode* BreakStatement();
        types::ContinueStatementNode* ContinueStatement();
        types::IfSuperBlockStatementNode* IfSuperBlockStatement();
        types::IfSubBlockStatementNode* IfSubBlockStatement();
        types::ElseIfSubBlockStatementNode* ElseIfSubBlockStatement();
        types::ElseSubBlockStatementNode* ElseSubBlockStatement();
        types::SelectionStatementNode* SelectionStatement();
        types::ForBlockStatementNode* ForBlockStatement();
        types::WhileBlockStatementNode* WhileBlockStatement();
        types::DoWhileBlockStatementNode* DoWhileBlockStatement();
        types::IterationStatementNode* IterationStatement();
        types::GotoLabelStatementNode* GotoLabelStatementNode();
        types::CaseLabelStatementNode* CaseLabelStatementNode();
        types::DefaultLabelStatementNode* DefaultLabelStatementNode();

        types::LabelStatementNode* LabelStatement();


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
        types::IdentifierNode* Identifier();
    };

}
}
