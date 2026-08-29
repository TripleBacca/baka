#include "parser.hh"
#include "utils.hh"
#include <variant>


namespace baka
{
	namespace parser
	{
		types::CompoundStatementNode* Parser::CompoundStatement()
		{
			if (!Match(types::TokenType::LPAREN_CURLY))
			{
				assert(false && "Expected '{' at the beginning of a compound statement");
			}

			std::vector<std::variant<types::StatementNode*, types::DeclarationList*>> statements;

			while (Peek().TokenType_v != types::TokenType::RPAREN_CURLY)
			{
				if (detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek()))
				{
					auto* DeclarationListNode = ParseDeclarationList();
					statements.emplace_back(DeclarationListNode);
				}
				else
				{
					auto* StatementNode = ParseStatement();
					statements.emplace_back(StatementNode);
				}
			}

			if (!Match(types::TokenType::RPAREN_CURLY))
			{
				assert(false && "Expected '}' at the end of a compound statement");
			}


			return ASTALLOC.Alloc<types::CompoundStatementNode>(std::move(statements));
		}

	}
}
