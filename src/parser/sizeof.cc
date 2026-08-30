#include "types/parser/ast/sizeof.hh"
#include "parser.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typenode.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <optional>
#include <variant>



namespace baka {
namespace parser {

types::ExpressionNode* Parser::ParseSizeofExpression() {
    if(!Match(types::TokenType::K_SIZEOF)) {
        assert(false);
        // todo throw error
    }

    if(!Match(types::TokenType::LPAREN_ROUND)) {

        types::ExpressionNode* Expr = ParseFactor();
        if(!Expr) {
            // "expected expression" already reported deep
            return nullptr;
        }
        return ASTALLOC.Alloc<types::SizeofNode>(Expr);
    }
    // ate bracks

    std::optional<types::TypeNode*> Type = TryParseTypeName();

    std::variant<types::SizeofNode::SizeofTypeVariant, types::ExpressionNode*> Result;
    if(Type) {
        auto* AbstractDecl = ParseAbstractDeclarator();
        Result = types::SizeofNode::SizeofTypeVariant{Type.value(), AbstractDecl};
    } else {
        types::ExpressionNode* Expr = Expression();
        if(!Expr) {
            // "expected expression" already reported deep
            return nullptr;
        }
        Result = Expr;
    }

    if(!Match(types::TokenType::RPAREN_ROUND)) {
        ReportError("expected ')'");
        auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
        if (Sync != types::TokenType::EOF_TOKEN) {
            Advance();
        }
    }

    return ASTALLOC.Alloc<types::SizeofNode>(Result);
}

}
}
