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
			IdentifierNode* TypeName;

		};


	}
}
