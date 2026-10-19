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
    const size_t SourceStart = current;
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
        return AllocNodeAt<types::SizeofNode>(SourceStart, Expr);
    }
    // ate bracks

    std::optional<types::TypeNode*> Type = TryParseTypeName();

    std::variant<types::SizeofNode::SizeofTypeVariant, types::ExpressionNode*> Result;
    if(Type) {
        auto* AbstractDecl = ParseAbstractDeclarator();
        Result = types::SizeofNode::SizeofTypeVariant{Type.value(), AbstractDecl};
    } else {
        types::ExpressionNode* Expr = ParseAssignmentExpression();
        if(!Expr) {
            // "expected expression" already reported deep
            return nullptr;
        }
        if (Check(types::TokenType::OP_COMMA)) {
            std::vector<types::ExpressionNode*> Expressions{Expr};
            while (Match(types::TokenType::OP_COMMA)) {
                auto* Next = ParseAssignmentExpression();
                if (!Next) {
                    return nullptr;
                }
                Expressions.push_back(Next);
            }
            Expr = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Expressions));
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

    return AllocNodeAt<types::SizeofNode>(SourceStart, Result);
}

}
}
