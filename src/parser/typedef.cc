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
        const size_t SourceStart = current;
        if(!Match(types::TokenType::K_TYPEDEF)) {
            // todo throw error
            assert(false);
        }

        bool isConst = Match(types::TokenType::K_CONST);

        types::TypeSpecifierModifier Modifier = DetermineTypeSpecifierModifier();
        if (Modifier != types::TypeSpecifierModifier::NONE) {
            this->Advance();
        }

        types::IdentifierNode* BoundTypeName = ParseTypeIdentifier();

        types::TypedefNode* Node = AllocNodeAt<types::TypedefNode>(SourceStart, BoundTypeName, Modifier, isConst);

        // redefinition exemption applies to the typedef own variable name. not function params in the typdef
        // eg: typedef int (*foo)(int a, int b) exemption should not apply to a, b
        TypedefFlagGuard InTypedefGuard(*this, true);
        do {
            types::DeclarationIdentifierNode* Variable = ParseDeclarationIdentifier();
            // doesnt matter if typedef is already done on that variable name
            if (auto* identifier = Variable->getIdentifier(); identifier &&
                !TypeLookup.LookupCurrent(identifier->GetName()).has_value())
                RegisterOrReplaceType(identifier);
            Node->AddVariable(Variable);
        } while(Match(types::TokenType::OP_COMMA));

        if (!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after typedef declaration", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }


        return Node;
    }
}
}
