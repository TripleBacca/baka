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

		types::TypeSpecifierModifier Parser::DetermineTypeSpecifierModifier() const {
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
					if (IsStatic) {
						ReportError("duplicate 'static' specifier");
					}
					IsStatic = true;
				}
				if (Match(types::TokenType::K_CONST))
				{
					if (IsConst) {
						ReportError("duplicate 'const' specifier");
					}
					IsConst = true;
				}
			}

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			types::IdentifierNode* TypeName = ParseTypeIdentifier();

			std::vector<types::SingleDeclarationNode*> Declarations;
			auto* Node = ParseSingleDeclaration();
			if(Node == nullptr) {
				ReportError("expected single declaration");
				Node = ASTALLOC.Alloc<types::SingleDeclarationNode>(
				    ASTALLOC.Alloc<types::DeclarationIdentifierNode>(), nullptr);
				Node->setHasError();
			}

			Declarations.push_back(Node);

			while (Match(types::TokenType::OP_COMMA))
			{
				auto* NextNode = ParseSingleDeclaration();
				Declarations.push_back(NextNode);
			}

			if (!Match(types::TokenType::SEMICOLON) && !Check(types::TokenType::EOF_TOKEN))
			{
				// e.g. "int a b = 2;" — a second identifier where ',' or ';' was expected
				ReportError("expected ',' or ';' in declaration");
				if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
					Advance();
				}
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
				ReportError("expected declaration identifier");
				DeclarationIdentifer = ASTALLOC.Alloc<types::DeclarationIdentifierNode>();
				DeclarationIdentifer->setHasError();
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
		// AllowRedefine : if true allows redefining the same identifier (useful for typedef that lets u redefine the same name)
		// false by default
		types::DeclarationIdentifierNode* Parser::ParseDeclarationIdentifier()
		{
			auto* Variable = ASTALLOC.Alloc<types::DeclarationIdentifierNode>();
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
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}


				Variable->setInnerDeclaration(InnerDeclaration);
			}
			else
			{
				if (!Check(types::TokenType::IDENTIFIER))
				{
					std::string_view ErrName = "?";
					if (std::holds_alternative<std::string_view>(Peek().Value)) {
						ErrName = std::get<std::string_view>(Peek().Value);
					}
					ReportError("expected identifier");
					if (!Check(types::TokenType::EOF_TOKEN)) {
						Advance();
					}
					auto* Errored = ASTALLOC.Alloc<types::IdentifierNode>(ErrName);
					Errored->setHasError();
					Variable->setInnerDeclaration(Errored);
				}
				else
				{
					auto* Identifier = ParseIdentifier();
					if(!InTypedef && LookupType(Identifier)) {
						ReportError("identifier cannot be a type name");
					}

					Variable->setInnerDeclaration(Identifier);
				}

			}

			// check for function
			// bind inner declaration to function ptr;
			// cases:
			// if inner decl = function ptr, bind to its return type
			// if inner decl = identifier, bind to it
			// if inner decl = function definition, bind to its return type - NOT POSSIBLE
			// if inner decl = pointer type,
			//

			// int (*(*foo[2])())();
			if(Match(types::TokenType::LPAREN_ROUND)) {
				auto* ParameterList = ParseFunctionParameterList();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}
				Variable->setFunctionParameters(ParameterList);
			}

			// int (*foo[20])()[];
			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				types::ExpressionNode* ArraySize = ParseAssignmentExpression();
				if (ArraySize == nullptr) {
				    // "expected expression" already reported deep; resync to the ']'
					SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				} else {
					Variable->appendArraySize(ArraySize);
					if (!Match(types::TokenType::RPAREN_SQUARE))
					{
						ReportError("expected ']'");
						SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
					}
				}
				Match(types::TokenType::RPAREN_SQUARE);
			}


			return Variable;
		}

		types::DeclarationIdentifierNode* Parser::ParseAbstractDeclarator()
		{
			auto* Variable = ASTALLOC.Alloc<types::DeclarationIdentifierNode>();
			while (Match(types::TokenType::OP_MUL))
			{
			    Variable->appendPointer();
			}
			// not supporting const pointer
			// int* const ptr - not supported

			if (Check(types::TokenType::LPAREN_ROUND) && Check2(types::TokenType::OP_MUL))
			{
				Advance();
				auto* InnerDeclaration = ParseAbstractDeclarator();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}

				Variable->setInnerDeclaration(InnerDeclaration);
			}

			if(Match(types::TokenType::LPAREN_ROUND)) {
				auto* ParameterList = ParseFunctionParameterList();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}
				Variable->setFunctionParameters(ParameterList);
			}


			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				types::ExpressionNode* ArraySize = ParseAssignmentExpression();
				if (ArraySize == nullptr) {
				    // "expected expression" already reported deep; resync to the ']'
					SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				} else {
					Variable->appendArraySize(ArraySize);
					if (!Match(types::TokenType::RPAREN_SQUARE))
					{
						ReportError("expected ']'");
						SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
					}
				}
				Match(types::TokenType::RPAREN_SQUARE);
			}

			return Variable;
		}
	}
}
