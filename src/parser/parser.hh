#pragma once
#include "symbol_table/symbol_table.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/initializer.hh"
#include "types/parser/ast/statement.hh"
#include "types/token/all.hh"
#include <span>
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/labelStatement.hh"
#include "types/parser/ast/jump.hh"
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
    struct ParserSTE {};

    class Parser {
        size_t current = 0;
        std::span<types::Token> Tokens;
        base::ParserSymbolTable<ParserSTE> TypeLookup;

        public:
        Parser(std::span<types::Token> Tokens) : Tokens(Tokens) {};
        types::ASTNode* Parse();

        // type lookup helpers:
        bool LookupType(types::IdentifierNode* Identifier);
        bool LookupType(std::string_view Name);
        void EnterScope();
        void ExitScope();
        void AddType(types::IdentifierNode* Identifier);

        // helpers
        bool Check(types::TokenType type) const noexcept;
        bool Check2(types::TokenType type) const noexcept;
        const types::Token& Advance();
        bool Match(types::TokenType type);
        const types::Token& Peek() const noexcept;
        const types::Token& Previous() const;

        // recursive descent nodes:

        types::ProgramNode* ParseProgram();

        types::FunctionNode* ParseFunction();
        types::FunctionParameter* ParseFunctionParameter();
        types::FunctionParameterList* ParseFunctionParameterList();
        types::StructNode* ParseStruct();

        types::StatementNode* ParseStatement();
        types::CompoundStatementNode* CompoundStatement();

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
        types::GotoLabelStatementNode* GotoLabelStatement();
        types::CaseLabelStatementNode* CaseLabelStatement();
        types::DefaultLabelStatementNode* DefaultLabelStatement();
        types::LabelStatementNode* LabelStatement();
        types::EnumNode* ParseEnumDecl();

        // this is for declarations inside a struct, im sorry
        types::StructNode* StructDeclarationStatement();
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
        types::InitializerNode* ParseInitializer(); // { intializer , intializer .... } or just 'assingmentexpr'

    };

}
}
