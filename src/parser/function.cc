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
		types::FunctionParameter* Parser::ParseFunctionParameter() {
			const size_t SourceStart = current;

			TypedefFlagGuard ScopedInTypedef(*this, false);

			types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;

			bool IsConst = false;
			if (Match(types::TokenType::K_CONST)) {
				IsConst = true;
			}

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			if (!isTypeName(this->Peek())) {
				ReportError("expected type name");
				auto* ErroredParam = AllocNodeAt<types::FunctionParameter>(SourceStart, IsConst, Modifier, nullptr, nullptr);
				ErroredParam->setHasError();
				return ErroredParam;
			}
			types::IdentifierNode* TypeName = ParseTypeIdentifier();


			types::SingleDeclarationNode* Node = ParseSingleDeclaration();

			// TODO: fix these disgusting constructors

			auto* Parameter = AllocNodeAt<types::FunctionParameter>(SourceStart, IsConst, Modifier, TypeName, Node);
			return Parameter;
		}

		types::FunctionParameterList* Parser::ParseFunctionParameterList()
		{
			const size_t SourceStart = current;
			// parses <functionparam>, <functionparam> , ...

			std::vector<types::FunctionParameter*> Decls;
			bool Variadic = false;

			if (Check(types::TokenType::RPAREN_ROUND))
			{
				// empty parameter list
				return AllocNodeAt<types::FunctionParameterList>(SourceStart, Decls, Variadic);
			}

			bool DefaultRunning = false;
			do
			{
				if (this->Match(types::TokenType::OP_ELLIPSIS))
				{
					Variadic = true;
					break;
				}

				auto* FunctionParam = ParseFunctionParameter();

				if (FunctionParam->hasError())
				{
					// the errored param consumed nothing; resync to the next separator so the
					// enclosing declarator still sees a balanced parenthesized list
					SkipTo({types::TokenType::OP_COMMA, types::TokenType::RPAREN_ROUND});
				}

				if (FunctionParam->hasInitalizer())
				{
					DefaultRunning = true;
				}
				else if (DefaultRunning)
				{
					// default should be at the end
					ReportError("default argument must be at the end of the parameter list");
				}

				Decls.push_back(FunctionParam);
			}
			while (this->Match(types::TokenType::OP_COMMA));

			return AllocNodeAt<types::FunctionParameterList>(SourceStart, Decls, Variadic);
		}

		std::optional<types::FunctionNode*> Parser::TryParseFunction()
		{
			const size_t SourceStart = current;
			size_t prev = current;
			size_t ErrorsOnEntry = ReportedErrorCount;

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

			if (!isTypeName(this->Peek()))
			{
				// not a plausible function/declaration start (e.g. a stray 'if' at top level)
				std::string_view Name = "?";
				if (std::holds_alternative<std::string_view>(Peek().Value)) {
					Name = std::get<std::string_view>(Peek().Value);
				}
				if (Check(types::TokenType::IDENTIFIER)) {
					ReportError("unknown type name '" + std::string(Name) + "'");
				} else {
					ReportError("expected identifier or '('");
				}
				if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
					Advance();
				}
				return std::nullopt;
			}

            types::IdentifierNode* ReturnTypeBindedIdentifier = this->ParseTypeIdentifier();

			// int (*foo())();

			// TODO: declaration identifer should deal with function
			// single declaration can take a default init issok
			// in single declaration if its declared then put in symbol table and mark as not declared (not defined = true)

			types::DeclarationIdentifierNode* FunctionTypeDecl = this->ParseDeclarationIdentifier();
			if (!FunctionTypeDecl)
			{
				ReportError("expected declaration identifier after return type");
				return std::nullopt;
			}

			if (!Check(types::TokenType::LPAREN_CURLY))
			{
				if (ReportedErrorCount != ErrorsOnEntry)
				{
					// the probe already reported an error; rolling back would re-parse the
					// broken declaration and report it again. resync to the end instead.
					if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
						Advance();
					}
					return std::nullopt;
				}
				current = prev;
				return std::nullopt;
			}

			// function body
			types::StatementNode* Body = this->CompoundStatement();

			return AllocNodeAt<types::FunctionNode>(SourceStart, ReturnTypeBindedIdentifier, FunctionTypeDecl, Body, IsStatic,
			                                           IsConst, Modifier);
		}
	}
}
