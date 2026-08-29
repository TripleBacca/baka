#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <string_view>

#include "identifier.hh"

namespace baka {

	namespace types {

		class TypeNode : public ASTNode
		{
			bool isConst = false;
			TypeSpecifierModifier Modifier = TypeSpecifierModifier::NONE;
			IdentifierNode* TypeName;
			// No identifier for name

		public:
			TypeNode(bool isConst, TypeSpecifierModifier modifier, IdentifierNode* TypeName) :
				isConst(isConst), Modifier(modifier), TypeName(TypeName)
			{
				if(modifier == TypeSpecifierModifier::UNSIGNED && !detail::isUnsignedTypeName(TypeName->GetName())) {
					// todo throw error
					assert(false);
				}
			}

			void Print(size_t Tabs = 0) const override
			{
				INDENT(Tabs);
				std::cout << "TypeNode(" << "\n";
				INDENT(Tabs + 1);
				std::cout << "isConst: " << isConst << "\n";
				INDENT(Tabs + 1);
				std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";
				if (TypeName) {
					TypeName->Print(Tabs + 1);
				} else {
					INDENT(Tabs + 1);
					std::cout << "nullptr\n";
				}
				INDENT(Tabs);
				std::cout << ")\n";
			}

		};


	}
}
