#pragma once
#include "symbol_table/symbol_table.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/initializer.hh"
#include "types/parser/ast/statement.hh"
#include "types/parser/ast/structBody.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/union.hh"
#include "types/token/all.hh"
#include <span>
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/destructor.hh"
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
    struct ParserSTE
    {
        private:
            uint8_t flag = 0;

        static constexpr uint8_t IS_STRUCT = 1 << 0;
        static constexpr uint8_t IS_STRUCT_DEFINED = 1 << 1;

        public:
            bool IsStruct() const noexcept { return flag & IS_STRUCT; }
            bool IsStructDefined() const noexcept { return flag & IS_STRUCT_DEFINED; }

            void SetIsStruct() noexcept { flag |= IS_STRUCT; }
            void SetIsStructDefined() {
                if(!IsStruct()) {
                    throw std::runtime_error("Cannot set struct defined flag on non-struct type");
                }
                flag |= IS_STRUCT_DEFINED;
            }

    };

    class Parser {
        size_t current = 0;
        std::span<types::Token> Tokens;
        base::ParserSymbolTable<ParserSTE> TypeLookup;

        public:
        Parser(std::span<types::Token> Tokens) : Tokens(Tokens) {};
        types::ASTNode* Parse();

        // states needed for parser
        bool InTypedef = false;

        // for typedef redefinition exemption
        class TypedefFlagGuard {
            Parser& Self;
            bool Saved;
        public:
            TypedefFlagGuard(Parser& parser, bool NewValue) :
                Self(parser), Saved(Self.InTypedef) {
                Self.InTypedef = NewValue;
            }
            ~TypedefFlagGuard() {
                Self.InTypedef = Saved;
            }
        };


        // type lookup helpers:
        bool LookupType(types::IdentifierNode* Identifier);
        ParserSTE* GetParserSTE(types::IdentifierNode* Identifier);
        bool LookupType(const std::string_view& Name);
        void EnterScope();
        void ExitScope();
        void AddType(types::IdentifierNode* Identifier);
        bool RegisterOrReplaceType(types::IdentifierNode* Identifier);


        // helpers
        bool Check(types::TokenType type) const noexcept;
        bool Check2(types::TokenType type) const noexcept;
        const types::Token& Advance();
        bool Match(types::TokenType type);
        const types::Token& Peek() const noexcept;
        const types::Token& Previous() const;

        bool isTypeName(const types::Token& token);

        // recursive descent nodes:

        types::ProgramNode* ParseProgram();

        types::ConstructorNode* ParseConstructor(types::IdentifierNode* ParentName);
        types::DestructorNode* ParseDestructor(types::IdentifierNode* ParentName);
        std::optional<types::FunctionNode*> TryParseFunction();
        types::FunctionParameter* ParseFunctionParameter();
        types::FunctionParameterList* ParseFunctionParameterList();
        std::variant<types::StructDefinitionNode*, types::StructDeclarationNode*> ParseStruct();
        std::variant<types::ClassDefinitionNode*, types::ClassDeclarationNode*> ParseClass();
        types::StructBodyNode* ParseStructBody(types::IdentifierNode* ParentName);

        types::UnionNode* ParseUnion();
        types::UnionBodyNode* ParseUnionBody(types::IdentifierNode* ParentName);

        types::IdentifierNode* ParseTypeIdentifier();

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
        types::SwitchBlockStatementNode* SwitchBlockStatement();
        types::SelectionStatementNode* SelectionStatement();
        types::ForBlockStatementNode* ForBlockStatement();
        types::WhileBlockStatementNode* WhileBlockStatement();
        types::DoWhileBlockStatementNode* DoWhileBlockStatement();
        types::IterationStatementNode* IterationStatement();
        types::GotoLabelStatementNode* GotoLabelStatement();
        types::CaseLabelStatementNode* CaseLabelStatement();
        types::DefaultLabelStatementNode* DefaultLabelStatement();
        types::LabelStatementNode* LabelStatement();

        types::TypedefNode* ParseTypedef();

        types::ExpressionStatementNode* ParseExpressionStatement();

        types::TypeSpecifierModifier DetermineTypeSpecifierModifierType() const;
        types::StatementNodeType DetermineStatementType();
        types::EnumNode* ParseEnumDecl();

        types::ExpressionNode* Expression();
        // types::UnaryExpressionNode* UnaryExpression();
        types::ConstantNode* ParseConstantNode();


        // expressions:
        types::ExpressionNode* ParseCommaExpression();

        types::ExpressionNode* ParsePrimaryExpression();

        types::ExpressionNode* ParseFactor();
        types::TypeNode* TryParseTypeName();

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
