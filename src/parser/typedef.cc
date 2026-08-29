#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/token/token.hh"
#include "utils.hh"


namespace baka {
namespace parser {

    // not supporting : const typdef int x;
    // typedef opt(const) typename decl_ident;
    // wherever declaration list is typedef also must be there
    // this eats semicolon btw
    types::TypedefNode* Parser::ParseTypedef() {
        if(!Match(types::TokenType::K_TYPEDEF)) {
            // todo throw error
            assert(false);
        }

        bool isConst = Match(types::TokenType::K_CONST);

        types::TypeSpecifierModifier Modifier = DetermineTypeSpecifierModifierType();
        if (Modifier != types::TypeSpecifierModifier::NONE) {
            this->Advance();
        }

        types::IdentifierNode* BoundTypeName = ParseTypeIdentifier();

        types::DeclarationIdentifierNode* Variable = ParseDeclarationIdentifier();
        // doesnt matter if typedef is already done on that variable name
        AddType(Variable->getIdentifier());

        if(!Match(types::TokenType::SEMICOLON)) {
            // todo throw error
            assert(false);
        }

        types::TypedefNode* Node = ASTALLOC.Alloc<types::TypedefNode>(BoundTypeName, Variable, Modifier, isConst);
        return Node;
    }
}
}
