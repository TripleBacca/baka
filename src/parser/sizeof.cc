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
            // todo throw error
            assert(false);
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
            // todo throw error
            assert(false);
        }
        Result = Expr;
    }

    if(!Match(types::TokenType::RPAREN_ROUND)) {
        // todo throw error
        assert(false);
    }

    return ASTALLOC.Alloc<types::SizeofNode>(Result);
}

}
}
