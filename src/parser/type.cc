#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
namespace parser {

    baka::types::IdentifierNode* Parser::ParseTypeIdentifier() {
        if(Check(types::TokenType::IDENTIFIER)) {
            types::IdentifierNode* Node =  ParseIdentifier();
            if(!LookupType(Node)) {
                // todo error
                assert(false);
            }

            return Node;
        } else {
            if(std::holds_alternative<std::string_view>(Peek().Value)) {
                std::string_view Name = std::get<std::string_view>(Peek().Value);
                if(LookupType(Name)) {
                    Advance();
                    return ASTALLOC.Alloc<types::IdentifierNode>(Name);
                }
            }

            // todo error
            assert(false);
        }
    }

}
}
