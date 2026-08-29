#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <variant>


namespace baka
{
	namespace parser
	{
		// static int a,*b,*c[10]
		//
		//
		// declaration        := 'static'? 'const'? type-specifier declarator-list ';'
		// type-specifier      := 'int' | 'char' | ... | 'struct' IDENTIFIER
		// declarator-list     := declarator (',' declarator)*
		// declarator          := '*'* IDENTIFIER ('[' expr? ']')* ('=' initializer)?
		// initializer         := expr | '{' expr (',' expr)* '}'


		// TODO: stuff left to support
		// function ptr

		// bare struct - not gonna support


		struct X {};
		unsigned  x = 1;
		// char
		// int

		types::TypeSpecifierModifier Parser::DetermineTypeSpecifierModifierType() const {
			switch (Peek().TokenType_v) {
			case types::TokenType::K_UNSIGNED:
				return types::TypeSpecifierModifier::UNSIGNED;
			case types::TokenType::K_STRUCT:
				return types::TypeSpecifierModifier::STRUCT;
			case types::TokenType::K_CLASS:
				return types::TypeSpecifierModifier::CLASS;
			case types::TokenType::K_ENUM:
				return types::TypeSpecifierModifier::ENUM;
			case types::TokenType::K_UNION:
				return types::TypeSpecifierModifier::UNION;
			default:
				return types::TypeSpecifierModifier::NONE;
			}
		}

		// wherever declaration list is typedef also must be there
		types::DeclarationList* Parser::ParseDeclarationList()
		{
		    types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;

			bool IsStatic = false;
			bool IsConst = false;

			while (Check(types::TokenType::K_STATIC) || Check(types::TokenType::K_CONST))
			{
				if (Match(types::TokenType::K_STATIC))
				{
					IsStatic = true;
				}
				if (Match(types::TokenType::K_CONST))
				{
					IsConst = true;
				}
			}

			Modifier = DetermineTypeSpecifierModifierType();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			types::IdentifierNode* TypeName = ParseTypeIdentifier();

			std::vector<types::SingleDeclarationNode*> Declarations;
			auto* Node = ParseSingleDeclaration();
			if(Node == nullptr) {
			    // idk if this faulure case is ever met
				// todo throw error
				assert(false && "Expected single declaration");
			}

			Declarations.push_back(Node);

			while (Match(types::TokenType::OP_COMMA))
			{
				auto* NextNode = ParseSingleDeclaration();
				Declarations.push_back(NextNode);
			}

			if (!Match(types::TokenType::SEMICOLON))
			{
				// TODO: throw error
				assert(false && "Expected semicolon at the end of declaration list");
			}

			auto* DeclarationListNode = ASTALLOC.Alloc<types::DeclarationList>(IsStatic, IsConst, Modifier, TypeName, std::move(Declarations));
			return DeclarationListNode;
		}

		// int * ((***a[2])[4])[5];
		// int a = {1}, b;
		types::SingleDeclarationNode* Parser::ParseSingleDeclaration()
		{
			auto* DeclarationIdentifer = ParseDeclarationIdentifier();
			if(DeclarationIdentifer == nullptr) {
				// todo throw error
				assert(false && "Expected declaration identifier");
			}

			types::ExpressionNode* Initialization = nullptr;
			if (Match(types::TokenType::OP_ASSIGN))
			{
				Initialization = ParseInitializer();
				// Intializer is either : assignment_expr, { intializer, ... }
			}

			auto* Node = ASTALLOC.Alloc<types::SingleDeclarationNode>(DeclarationIdentifer, Initialization);
			return Node;
		}


		// TODO : write function declaration parser

		types::DeclarationIdentifierNode* Parser::ParseDeclarationIdentifier()
		{
			types::DeclarationIdentifierNode *Variable = ASTALLOC.Alloc<types::DeclarationIdentifierNode>();
			while (Match(types::TokenType::OP_MUL))
			{
			    Variable->appendPointer();
			}

			// not supporting const pointer
			// int* const ptr - not supported

			if (Match(types::TokenType::LPAREN_ROUND))
			{
				auto* InnerDeclaration = ParseDeclarationIdentifier();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					// TODO: throw error
					assert(false && "Expected ')'");
				}

				// check for function
				if(Match(types::TokenType::LPAREN_ROUND)) {
					auto* ParameterList = ParseFunctionParameterList();
					if(!Match(types::TokenType::RPAREN_ROUND))
					{
						// TODO: throw error
						assert(false && "Expected ')'");
					}

					// if(!InnerDeclaration->hasPointerAtAnyLevel())
					// {
					//     // todo throw error
					// 	assert(false && "Function pointer declarator requires at least one '*' inside the parentheses");
					// }

					Variable->setFunctionParameters(ParameterList);

					// bind inner declaration to function ptr;
					// cases:
					// if inner decl = function ptr, bind to its return type
					// if inner decl = identifier, bind to it
					// if inner decl = function definition, bind to its return type - NOT POSSIBLE
					// if inner decl = pointer type,
					//

					// int (*(*foo[2])())();
				}

				Variable->setInnerDeclaration(InnerDeclaration);
			}
			else
			{
				if (!Check(types::TokenType::IDENTIFIER))
				{
					// TODO: throw error
					assert(false && "Expected identifier");
				}

				auto* Identifier = ParseIdentifier();
				if(LookupType(Identifier)) {
				    // todo throw error
					assert(false && "Identifier already type");
				}

				Variable->setInnerDeclaration(Identifier);

				// check for function ( copypastad code )
				if(Match(types::TokenType::LPAREN_ROUND)) {
					auto* ParameterList = ParseFunctionParameterList();
					if(!Match(types::TokenType::RPAREN_ROUND))
					{
						// TODO: throw error
						assert(false && "Expected ')'");
					}
					Variable->setFunctionParameters(ParameterList);
				}

			}

			// int (*foo[20])()[];
			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				types::ExpressionNode* ArraySize = ParseAssignmentExpression();
				if(ArraySize == nullptr) {
				    // todo throw error empty array expr
					assert(false && "Expected array size");
				}

				Variable->appendArraySize(ArraySize);

				if (!Match(types::TokenType::RPAREN_SQUARE))
				{
					// TODO: throw error
					assert(false && "Expected ']'");
				}
			}


			return Variable;
		}
	}
}
