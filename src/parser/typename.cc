#include "parser.hh"
#include "utils.hh"

namespace baka
{
	namespace parser
	{
		std::optional<types::TypeNode*> Parser::TryParseTypeName()
		{
			size_t prev = current;

			if (Check(types::TokenType::K_STATIC))
				return std::nullopt;

			types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;
			bool IsConst = false;
			if (Match(types::TokenType::K_CONST)) IsConst = true;

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			if (!isTypeName(Peek())) {
				current = prev;
				return std::nullopt;
			}

			types::IdentifierNode* TypeName = ParseTypeIdentifier();
			auto* Node = ASTALLOC.Alloc<types::TypeNode>(IsConst, Modifier, TypeName);
			return Node;
		}

	}
}