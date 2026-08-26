#pragma once


#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include "memory/custom_arenas.hh"

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

		// parse_specifiers():
		//     is_static = False
		//     is_const = False

		//     if peek() == 'static':
		//         consume('static')
		//         is_static = True

		//     if peek() == 'const':
		//         consume('const')
		//         is_const = True

		//     base_type = parse_type_specifier()
		//     base_type.is_const = is_const   # attach to the type, not the declarator

		//     return is_static, base_type
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


			std::vector<types::SingleDeclarationNode*> Declarations;
			auto* Node = ParseSingleDeclaration();
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
			auto* DeclarationListNode = ASTALLOC.Alloc<types::DeclarationList>(IsStatic, IsConst, Declarations);
			return DeclarationListNode;
		}


		// int * ((***a[2])[4])[5];
		types::SingleDeclarationNode* Parser::ParseSingleDeclaration()
		{

		}

		types::DeclarationIdentifierNode* Parser::ParseDeclarationIdentifier()
		{
			types::DeclarationIdentifierNode *VariableName = nullptr;
			size_t PointerCount = 0;
			while (Match(types::TokenType::OP_MUL))
			{
				PointerCount++;
			}

			if (Match(types::TokenType::LPAREN_ROUND))
			{
				auto* InnerDeclaration = ParseDeclarationIdentifier();
				Match(types::TokenType::RPAREN_ROUND);
				VariableName = ASTALLOC.Alloc<types::DeclarationIdentifierNode>(PointerCount, InnerDeclaration, std::vector<types::ExpressionNode*>{});
			}
			else
			{
				if (!Check(types::TokenType::IDENTIFIER))
				{
					// TODO: throw error
					assert(false && "Expected identifier");
				}
				VariableName = ASTALLOC<types::DeclarationIdentifierNode>(ParseIdentifier(), std::vector<types::ExpressionNode*>{});
			}

			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				types::ExpressionNode* ArraySize = nullptr;
				if (!Check(types::TokenType::RPAREN_SQUARE))
				{
					ArraySize = ParseAssignmentExpression();
				}
				Match(types::TokenType::RPAREN_SQUARE);
				VariableName->ArraySizes.push_back(ArraySize);
			}
			return VariableName;
		}
	}
}