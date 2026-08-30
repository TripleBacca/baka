#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
namespace parser {

    types::IdentifierNode* Parser::ParseTypeIdentifier() {
        if(Check(types::TokenType::IDENTIFIER)) {
            types::IdentifierNode* Node =  ParseIdentifier();
            if(!LookupType(Node)) {
                ReportError("unknown type name '" + std::string(Node->GetName()) + "'");
                Node->setHasError();
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

            std::string_view ErrName = "?";
            if (std::holds_alternative<std::string_view>(Peek().Value)) {
                ErrName = std::get<std::string_view>(Peek().Value);
            }
            if (Check(types::TokenType::IDENTIFIER)) {
                ReportError("unknown type name '" + std::string(ErrName) + "'");
            } else {
                ReportError("expected identifier or '('");
            }
            if (!Check(types::TokenType::EOF_TOKEN)) {
                Advance();
            }
            auto* Errored = ASTALLOC.Alloc<types::IdentifierNode>(ErrName);
            Errored->setHasError();
            return Errored;
        }
    }

}
}
