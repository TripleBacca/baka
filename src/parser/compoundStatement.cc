#include "parser.hh"
#include "types/token/token.hh"
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
				ReportError("expected '{'");
			}

			EnterScope();

			std::vector<std::variant<types::StatementNode*, types::DeclarationList*, types::TypedefNode*, types::FunctionNode*>> statements;

			while (Peek().TokenType_v != types::TokenType::RPAREN_CURLY && !Check(types::TokenType::EOF_TOKEN))
			{
				if (detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek()))
			{
				if (auto PossibleFunction = TryParseFunction()) {
					statements.emplace_back(PossibleFunction.value());
				} else if (Check(types::TokenType::EOF_TOKEN) || Check(types::TokenType::RPAREN_CURLY)) {
					// TryParseFunction consumed the rest while bailing out of a broken probe.
					// Nothing left to parse as a declaration here.
				} else {
					auto* DeclarationListNode = ParseDeclarationList();
					statements.emplace_back(DeclarationListNode);
				}
			}
				else if (Check(types::TokenType::K_TYPEDEF)) {
					auto* TypedefNode = ParseTypedef();
					statements.emplace_back(TypedefNode);
				}
				else
				{
					auto* StatementNode = ParseStatement();
					statements.emplace_back(StatementNode);
				}
			}

			if (!Match(types::TokenType::RPAREN_CURLY))
			{
				ReportError("expected '}'");
			}

			ExitScope();

			return ASTALLOC.Alloc<types::CompoundStatementNode>(std::move(statements));
		}

	}
}
