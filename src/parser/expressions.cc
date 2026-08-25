
#include "parser.hh"
#include "parser.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifer.hh"
#include "types/parser/ast/utils.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include "base/memory/custom_arenas.hh"


namespace baka {

namespace parser {

// parse_factor():
//     if next token is unop:
//         consume unop
//         return Unary(op, parse_factor())
//     else:
//         return parse_postfix_exp()
//
// parse_postfix_exp():
//     expr = parse_primary_exp()      // int, identifier, "(" expr ")"
//     while next token is '++', '--', '.', '->', '[':
//         ... wrap expr accordingly ...
//     return expr
//
// parse_primary_exp():
//     // just the int/identifier/paren cases, factored out of the old parse_factor

//
//   parse_expr         (binary ops: + - * / etc, via precedence climbing)
//     → parse_factor      (unary ops: - ! ~)
//         → parse_postfix_exp   (postfix: ++ -- . -> [ ( )
//             → parse_primary_exp   (atoms: constants, identifiers, "(...)")


    types::ExpressionNode* Parser::ParsePrimaryExpression() {
        auto& ConstantOrIdentifier = Peek();

        if(Match(types::TokenType::LPAREN_ROUND)) {
            types::ExpressionNode* Node = ParseExpression();
            if(!Match(types::TokenType::RPAREN_ROUND)) {
                // TODO: throw erorr
            }
            return Node;
        }

        if (detail::isConstantToken(ConstantOrIdentifier.TokenType_v)) {
            auto& Constant = ConstantOrIdentifier;
            Advance();

            if (std::holds_alternative<int>(Constant.Value)) {
                types::ConstantNode* ConstantNode = ASTALLOC.Alloc<types::ConstantIntNode>(std::get<int>(Constant.Value));
                types::PrimaryExpressionNode* Node = ASTALLOC.Alloc<types::PrimaryExpressionNode>(ConstantNode);
                return Node;
            } else {
                assert(false && "havent added other constant types");
            }


        } else if(ConstantOrIdentifier.TokenType_v == types::TokenType::IDENTIFIER) {
            auto& Identifier = ConstantOrIdentifier;
            Advance();

            assert(std::holds_alternative<std::string_view>(Identifier.Value));
            types::IdentiferNode* IdentifierNode = this->Identifier();

            types::PrimaryExpressionNode* PrimaryExpressionNode = ASTALLOC.Alloc<types::PrimaryExpressionNode>(IdentifierNode);

            return PrimaryExpressionNode;
        } else {
            // TODO: throw error
        }


    }


    types::ExpressionNode* Parser::ParseFactor() {
        auto& CurrentToken = Peek();

        if (detail::IsUnaryOperator(CurrentToken.TokenType_v)) {
            types::ASTUnaryOp UnaryOp = types::TokenTypeToASTUnaryOp.at(CurrentToken.TokenType_v);
            Advance();

            types::ExpressionNode* Expr = Parser::ParseFactor();

            return ASTALLOC.Alloc<types::FactorNode>(UnaryOp, Expr);
        } else  {
            return ParsePostfixExpression();
        }
    }

    // for ++, -- , [], ->, .
    types::PostfixExpressionNode* Parser::ParsePostfixExpression() {

    }


}
}
