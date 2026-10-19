#include "parser.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/union.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>


namespace baka
{
	namespace parser
	{
		bool Parser::TryParseLocalAggregate(types::CompoundStatementNode* Node)
		{
			if (!AtAggregateDefinition()) return false;

			switch (Peek().TokenType_v) {
			case types::TokenType::K_STRUCT: {
				auto Parsed = ParseStruct();
				if (std::holds_alternative<types::StructDefinitionNode*>(Parsed)) {
					Node->addStatement(std::get<types::StructDefinitionNode*>(Parsed));
				} else {
					Node->addStatement(std::get<types::StructDeclarationNode*>(Parsed));
				}
				return true;
			}
			case types::TokenType::K_CLASS: {
				auto Parsed = ParseClass();
				if (std::holds_alternative<types::ClassDefinitionNode*>(Parsed)) {
					Node->addStatement(std::get<types::ClassDefinitionNode*>(Parsed));
				} else {
					Node->addStatement(std::get<types::ClassDeclarationNode*>(Parsed));
				}
				return true;
			}
			case types::TokenType::K_UNION:
				Node->addStatement(ParseUnion());
				return true;
			case types::TokenType::K_ENUM:
				Node->addStatement(ParseEnumDecl());
				return true;
			default:
				return false;
			}
		}

		types::CompoundStatementNode* Parser::CompoundStatement()
		{
			const size_t SourceStart = current;
			if (!Match(types::TokenType::LPAREN_CURLY))
			{
				ReportError("expected '{'");
			}

			EnterScope();

			types::CompoundStatementNode::StatementList statements;

			while (Peek().TokenType_v != types::TokenType::RPAREN_CURLY && !Check(types::TokenType::EOF_TOKEN))
			{
				if (AtAggregateDefinition()) {
					auto* Scratch = AllocNodeAt<types::CompoundStatementNode>(SourceStart,
					    types::CompoundStatementNode::StatementList{});
					if (TryParseLocalAggregate(Scratch)) {
						for (auto& Statement : Scratch->getStatements()) statements.push_back(Statement);
						continue;
					}
				}

				if (Check(types::TokenType::K_TYPEDEF)) {
					auto* TypedefNode = ParseTypedef();
					statements.emplace_back(TypedefNode);
				} else if (AtDeclarationStart()) {
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

			return AllocNodeAt<types::CompoundStatementNode>(SourceStart, std::move(statements));
		}

	}
}
