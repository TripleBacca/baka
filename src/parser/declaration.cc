#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include <cassert>


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
		// unsigned annol
		// bare struct - not gonna support

		types::DeclarationList* Parser::ParseDeclarationList()
		{
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

			Match(types::TokenType::K_STRUCT);
			types::IdentifierNode* TypeName = ParseIdentifier(); //TODO: parse type specifier properly
			if(!LookupType(TypeName)) {
				// TODO: throw error
				assert(false && "Expected type name");
			}

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

			auto* DeclarationListNode = ASTALLOC.Alloc<types::DeclarationList>(IsStatic, IsConst,TypeName, std::move(Declarations));
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


		types::DeclarationIdentifierNode* Parser::ParseDeclarationIdentifier()
		{
			types::DeclarationIdentifierNode *Variable = ASTALLOC.Alloc<types::DeclarationIdentifierNode>();
			while (Match(types::TokenType::OP_MUL))
			{
			    Variable->appendPointer();
			}

			if (Match(types::TokenType::LPAREN_ROUND))
			{
				auto* InnerDeclaration = ParseDeclarationIdentifier();
				Match(types::TokenType::RPAREN_ROUND);
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
			}

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
