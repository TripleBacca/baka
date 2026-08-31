#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
namespace parser {

    std::variant<types::StructDefinitionNode*, types::StructDeclarationNode*> Parser::ParseStruct() {
        if (!this->Match(types::TokenType::K_STRUCT)) {
            // todo throw error
            assert(false && "Expected 'struct' keyword");
        }

        if (!this->Check(types::TokenType::IDENTIFIER)) {
            ReportError("expected identifier after 'struct' keyword");
            auto Sync = SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            if (Sync == types::TokenType::SEMICOLON) {
                Advance();
            }
            return ASTALLOC.Alloc<types::StructDeclarationNode>(
                ASTALLOC.Alloc<types::IdentifierNode>(std::string_view("?")));
        }
        types::IdentifierNode* StructIdentifier = this->ParseIdentifier();
        if (Match(types::TokenType::SEMICOLON)) {
            // this is a declaration
            auto* Node = ASTALLOC.Alloc<types::StructDeclarationNode>(StructIdentifier);
            if (!LookupType(StructIdentifier)) {
                AddType(StructIdentifier);
                GetParserSTE(StructIdentifier)->SetIsStruct();

            } else {
                if(!GetParserSTE(StructIdentifier)->IsStruct()) {
                    ReportError("not a struct");
                }

            }
            return Node;
        }

        // surely a definition
        if(LookupType(StructIdentifier)) {
            // TODO throw error
            if(GetParserSTE(StructIdentifier)->IsStructDefined()) {
                ReportError("redefinition of struct");
            } else if(!GetParserSTE(StructIdentifier)->IsStruct()) {
                ReportError("not a struct");
            }
        }

        types::IdentifierNode* ParentStructIdentifier = nullptr;
        if (this->Match(types::TokenType::OP_COLON))
        {
            if (!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after ':'");
            } else {
                ParentStructIdentifier = this->ParseIdentifier();
            }
            if(ParentStructIdentifier && (!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->IsStructDefined())) {
                ReportError("parent struct/class not found in symbol table");
            }
        }

        if (!LookupType(StructIdentifier)) {
            AddType(StructIdentifier);
            GetParserSTE(StructIdentifier)->SetIsStruct();
        }
        GetParserSTE(StructIdentifier)->SetIsStructDefined();
        EnterScope();

        types::StructBodyNode* Body = nullptr;
        if (!this->Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after struct/class declaration");
            Body = ASTALLOC.Alloc<types::StructBodyNode>();
        } else {
            Body = this->ParseStructBody(StructIdentifier);
        }
        if (!Match(types::TokenType::SEMICOLON))
        {
            ReportError("expected ';' after struct/class body", true);
            SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            Match(types::TokenType::SEMICOLON);
        }

        types::StructDefinitionNode* Node = nullptr;
        if (ParentStructIdentifier) {
            Node = ASTALLOC.Alloc<types::StructDefinitionNode>(StructIdentifier, ParentStructIdentifier, Body);
        } else {
            Node = ASTALLOC.Alloc<types::StructDefinitionNode>(StructIdentifier, Body);
        }

        ExitScope();

        return Node;
    }

}
}
