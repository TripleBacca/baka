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
                // todo throw error
                SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                assert(false && "Expected identifier after 'class' keyword");
            }

            types::IdentifierNode *ClassIdentifier = this->ParseIdentifier();

            if (Match(types::TokenType::SEMICOLON)) {
                // this is a declaration
                auto *Node = ASTALLOC.Alloc<types::ClassDeclarationNode>(ClassIdentifier);
                if (!LookupType(ClassIdentifier)) {
                    AddType(ClassIdentifier);
                    GetParserSTE(ClassIdentifier)->SetIsStruct();
                } else if(!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    // TODO throw error
                    SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                    assert(false && "Not a class");
                }
                return Node;
            }

            // surely a definition
            if (LookupType(ClassIdentifier)) {
                // TODO throw error
                if (GetParserSTE(ClassIdentifier)->IsStructDefined()) {
                    assert(false && "Struct already defined");
                } else if (!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    assert(false && "Not a struct");
                }
            }

            types::IdentifierNode *ParentStructIdentifier = nullptr;
            if (this->Match(types::TokenType::OP_COLON)) {
                if (!this->Check(types::TokenType::IDENTIFIER)) {
                    // todo throw error
                    assert(false);
                }
                ParentStructIdentifier = this->ParseIdentifier();
                if (!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->IsStructDefined()) {
                    // todo throw error
                    assert(false && "Parent struct/class not found in symbol table");
                }
            }

            if (!LookupType(ClassIdentifier)) {
                AddType(ClassIdentifier);
                GetParserSTE(ClassIdentifier)->SetIsStruct();
            }
            GetParserSTE(ClassIdentifier)->SetIsStructDefined();
            EnterScope();

            if (!this->Check(types::TokenType::LPAREN_CURLY)) {
                // todo throw error
                assert(false && "Expected '{' after struct/class declaration");
            }
            types::StructBodyNode *Body = this->ParseStructBody(ClassIdentifier);
            if (!this->Match(types::TokenType::SEMICOLON)) {
                // todo throw error
                assert(false && "Expected semicolon after struct/class body");
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
