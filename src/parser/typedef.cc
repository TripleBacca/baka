#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/token/token.hh"
#include "utils.hh"



namespace baka {
namespace parser {

    // not supporting : const typdef int x;
    // typedef opt(const) typename decl_ident;
    types::TypedefNode* Parser::ParseTypedef() {
        if(!Match(types::TokenType::K_TYPEDEF)) {
            // todo throw error
            assert(false);
        }

        bool isConst = Match(types::TokenType::K_CONST);
        types::IdentifierNode* BoundTypeName = ParseTypeIdentifier();

        types::DeclarationIdentifierNode* Variable = ParseDeclarationIdentifier();

        types::TypedefNode* Node = ASTALLOC.Alloc<types::TypedefNode>(BoundTypeName, Variable, isConst);
        return Node;
    }
}
}
