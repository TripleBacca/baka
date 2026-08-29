#include "types/parser/ast/function.hh"
#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/statement.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <optional>

// A declaration of a parameter as "function returning type" shall be adjusted to "pointer to function returning type"
// void x(int x, int foo() = 0) {
// }

namespace baka
{
	namespace parser
	{
		types::FunctionParameter* Parser::ParseFunctionParameter()
		{
			// TODO: need to test dis
			bool isStructOrClass = Match(types::TokenType::K_STRUCT); // eat
			bool isEnum = Match(types::TokenType::K_ENUM); // eat
			bool isUnion = Match(types::TokenType::K_UNION); // eat

			bool isUnsigned = Match(types::TokenType::K_UNSIGNED);

			bool IsConst = false;
			if (Match(types::TokenType::K_CONST))
			{
				IsConst = true;
			}

			if (!isTypeName(this->Peek()))
			{
				// throw error
				assert(false);
			}
			types::IdentifierNode* TypeName = ParseTypeIdentifier();


			types::SingleDeclarationNode* Node = ParseSingleDeclaration();

			// TODO: fix these disgusting constructors
			auto* Parameter = ASTALLOC.Alloc<types::FunctionParameter>(IsConst, TypeName, Node, isStructOrClass, isEnum,
			                                                           isUnion, isUnsigned);
			return Parameter;
		}

		types::FunctionParameterList* Parser::ParseFunctionParameterList()
		{
			// parses <functionparam>, <functionparam> , ...

			std::vector<types::FunctionParameter*> Decls;

			if (Check(types::TokenType::RPAREN_ROUND))
			{
				// empty parameter list
				return ASTALLOC.Alloc<types::FunctionParameterList>(Decls);
			}

			bool DefaultRunning = false;
			do
			{
				if (this->Match(types::TokenType::OP_ELLIPSIS))
				{
					break;
				}

				auto* FunctionParam = ParseFunctionParameter();

				if (FunctionParam->hasInitalizer())
				{
					DefaultRunning = true;
				}
				else if (DefaultRunning)
				{
					// default shoudl be in end
					// throw error
					assert(false);
				}

				Decls.push_back(FunctionParam);
			}
			while (this->Match(types::TokenType::OP_COMMA));

			return ASTALLOC.Alloc<types::FunctionParameterList>(Decls);
		}

		std::optional<types::FunctionNode*> Parser::TryParseFunction()
		{
			size_t prev = current;

			bool isUnsigned = false;

			bool IsStatic = false;
			bool IsConst = false;

			bool isClassOrStruct = false;
			bool isEnum = false;
			bool isUnion = false;

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

			isClassOrStruct = Match(types::TokenType::K_STRUCT);
			isEnum = Match(types::TokenType::K_ENUM);
			isUnion = Match(types::TokenType::K_UNION);

			isUnsigned = Match(types::TokenType::K_UNSIGNED);


            types::IdentifierNode* ReturnTypeBindedIdentifier = this->ParseTypeIdentifier();

			// int (*foo())();

			// TODO: declaration identifer should deal with function
			// single declaration can take a default init issok
			// in single declaration if its declared then put in symbol table and mark as not declared (not defined = true)

			types::DeclarationIdentifierNode* FunctionTypeDecl = this->ParseDeclarationIdentifier();
			if (!FunctionTypeDecl)
			{
				// throw error
				assert(false);
			}

			if (!Check(types::TokenType::LPAREN_CURLY))
			{
				current = prev;
				return std::nullopt;
			}

			// function body
			types::StatementNode* Body = this->CompoundStatement();

			return ASTALLOC.Alloc<types::FunctionNode>(ReturnTypeBindedIdentifier, FunctionTypeDecl, Body, IsStatic,
			                                           IsConst, isClassOrStruct, isEnum, isUnion, isUnsigned);
		}
	}
}
