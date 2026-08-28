#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
namespace parser {

    std::variant<types::StructDefinitionNode*, types::StructDeclarationNode*> Parser::ParseStruct() {
        //TODO add support for declarations after struct - not doing
        //TODO semicolon?

        if (!this->Match(types::TokenType::K_STRUCT)) {
            // match struct keyword
            // todo throw error
            assert(false && "Expected 'struct' keyword");
        }

        if (!this->Check(types::TokenType::IDENTIFIER)) {
            // todo throw error
            assert(false && "Expected identifier after 'struct' keyword");
        }
        types::IdentifierNode* StructIdentifier = this->ParseIdentifier();
        if (Match(types::TokenType::SEMICOLON)) {
            // this is a declaration
            auto* Node = ASTALLOC.Alloc<types::StructDeclarationNode>(StructIdentifier);
            if (!LookupType(StructIdentifier)) {
                AddType(StructIdentifier);
            }
            return Node;
        }

        // surely a definition
        if(LookupType(StructIdentifier) && GetParserSTE(StructIdentifier)->isDefined) {
            // TODO throw error
            assert(false && "Struct already defined");
        }

        types::IdentifierNode* ParentStructIdentifier = nullptr;
        if (this->Match(types::TokenType::OP_COLON))
        {
            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // todo throw error
                assert(false);
            }
            ParentStructIdentifier = this->ParseIdentifier();
            if(!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->isDefined) {
                // todo throw error
                assert(false && "Parent struct/class not found in symbol table");
            }
        }

        if (!LookupType(StructIdentifier)) AddType(StructIdentifier);
        GetParserSTE(StructIdentifier)->isDefined = true;
        EnterScope();

        if (!this->Check(types::TokenType::LPAREN_CURLY)) {
            // todo throw error
            assert(false && "Expected '{' after struct/class declaration");
        }
        types::CompoundStatementNode* Body = this->CompoundStatement();
        if (!this->Match(types::TokenType::SEMICOLON))
        {
            // todo throw error
            assert(false && "Expected semicolon after struct/class body");
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
