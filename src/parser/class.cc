#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
    namespace parser {
        std::variant<types::ClassDefinitionNode *, types::ClassDeclarationNode *> Parser::ParseClass() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CLASS)) {
                assert(false && "not be possible");
            }


            if (!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after 'class' keyword");
                auto Sync = SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
                return AllocNodeAt<types::ClassDeclarationNode>(SourceStart,
                    AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?")));
            }

            types::IdentifierNode *ClassIdentifier = this->ParseIdentifier();

            if (Match(types::TokenType::SEMICOLON)) {
                // this is a declaration
                auto *Node = AllocNodeAt<types::ClassDeclarationNode>(SourceStart, ClassIdentifier);
                if (!TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
                    AddType(ClassIdentifier);
                    GetParserSTE(ClassIdentifier)->SetIsStruct();
                } else if(!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    ReportError("not a class");
                }
                return Node;
            }

            // surely a definition
            if (TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
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

            if (!TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
                AddType(ClassIdentifier);
                GetParserSTE(ClassIdentifier)->SetIsStruct();
            }
            if (GetParserSTE(ClassIdentifier)->IsStruct())
                GetParserSTE(ClassIdentifier)->SetIsStructDefined();
            EnterScope();

            types::StructBodyNode *Body = nullptr;
            if (!this->Check(types::TokenType::LPAREN_CURLY)) {
                ReportError("expected '{' after struct/class declaration");
                Body = AllocNodeAt<types::StructBodyNode>(SourceStart);
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
                Node = AllocNodeAt<types::ClassDefinitionNode>(SourceStart, ClassIdentifier, ParentStructIdentifier, Body);
            } else {
                Node = AllocNodeAt<types::ClassDefinitionNode>(SourceStart, ClassIdentifier, Body);
            }

            ExitScope();

            return Node;
        }
    }
}
