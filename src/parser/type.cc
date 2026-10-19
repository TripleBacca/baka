#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
namespace parser {

    types::IdentifierNode* Parser::ParseTypeIdentifier() {
        const size_t SourceStart = current;
        // Central validation also covers B-owned parameter/function parsers. Keep
        // invalid modifiers out of legacy AST constructors that still assert.
        if (current > 0 && Tokens[current-1].TokenType_v == types::TokenType::K_UNSIGNED &&
            std::holds_alternative<std::string_view>(Peek().Value) &&
            !baka::detail::isUnsignedTypeName(std::get<std::string_view>(Peek().Value))) {
            ReportError("unsigned requires char, short, int, or long");
            if (!Check(types::TokenType::EOF_TOKEN)) Advance();
            auto* invalid = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("int"));
            invalid->setHasError();
            return invalid;
        }
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
                    return AllocNodeAt<types::IdentifierNode>(SourceStart, Name);
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
            auto* Errored = AllocNodeAt<types::IdentifierNode>(SourceStart, ErrName);
            Errored->setHasError();
            return Errored;
        }
    }

}
}
