#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
    namespace parser {
        std::variant<types::ClassDefinitionNode *, types::ClassDeclarationNode *> Parser::ParseClass() {
            if (!this->Match(types::TokenType::K_CLASS)) {
                assert(false && "not be possible");
            }


            if (!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after 'class' keyword");
                auto Sync = SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
                return ASTALLOC.Alloc<types::ClassDeclarationNode>(
                    ASTALLOC.Alloc<types::IdentifierNode>(std::string_view("?")));
            }

            types::IdentifierNode *ClassIdentifier = this->ParseIdentifier();

            if (Match(types::TokenType::SEMICOLON)) {
                // this is a declaration
                auto *Node = ASTALLOC.Alloc<types::ClassDeclarationNode>(ClassIdentifier);
                if (!LookupType(ClassIdentifier)) {
                    AddType(ClassIdentifier);
                    GetParserSTE(ClassIdentifier)->SetIsStruct();
                } else if(!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    ReportError("not a class");
                }
                return Node;
            }

            // surely a definition
            if (LookupType(ClassIdentifier)) {
                // TODO throw error
                if (GetParserSTE(ClassIdentifier)->IsStructDefined()) {
                    ReportError("redefinition of class");
                } else if (!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    ReportError("not a class");
                }
            }

            types::IdentifierNode *ParentStructIdentifier = nullptr;
            if (this->Match(types::TokenType::OP_COLON)) {
                if (!this->Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after ':'");
                } else {
                    ParentStructIdentifier = this->ParseIdentifier();
                }
                if (ParentStructIdentifier && (!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->IsStructDefined())) {
                    ReportError("parent struct/class not found in symbol table");
                }
            }

            if (!LookupType(ClassIdentifier)) {
                AddType(ClassIdentifier);
                GetParserSTE(ClassIdentifier)->SetIsStruct();
            }
            GetParserSTE(ClassIdentifier)->SetIsStructDefined();
            EnterScope();

            types::StructBodyNode *Body = nullptr;
            if (!this->Check(types::TokenType::LPAREN_CURLY)) {
                ReportError("expected '{' after struct/class declaration");
                Body = ASTALLOC.Alloc<types::StructBodyNode>();
            } else {
                Body = this->ParseStructBody(ClassIdentifier);
            }
            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after struct/class body", true);
                SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
                Match(types::TokenType::SEMICOLON);
            }

            types::ClassDefinitionNode *Node = nullptr;
            if (ParentStructIdentifier) {
                Node = ASTALLOC.Alloc<types::ClassDefinitionNode>(ClassIdentifier, ParentStructIdentifier, Body);
            } else {
                Node = ASTALLOC.Alloc<types::ClassDefinitionNode>(ClassIdentifier, Body);
            }

            ExitScope();

            return Node;
        }
    }
}
