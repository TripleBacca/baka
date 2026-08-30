
#include "parser.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/operators.hh"
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/this.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>
#include <variant>


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
//     -> parse_factor      (unary ops: - ! ~)
//         -> parse_postfix_exp   (postfix: ++ -- . -> [ ( )
//             -> parse_primary_exp   (atoms: constants, identifiers, "(...)")
//         -> try_parse_typename (on finding "(")


    types::ExpressionNode* Parser::ParsePrimaryExpression() {
        auto& ConstantOrIdentifierOrThis = Peek();


        if(Match(types::TokenType::LPAREN_ROUND)) {
            types::ExpressionNode* Node = ParseCommaExpression();
            if(!Match(types::TokenType::RPAREN_ROUND)) {
                // TODO: throw erorr
            }
            return Node;
        }

        if (detail::isConstantToken(ConstantOrIdentifierOrThis.TokenType_v)) {
            auto& Constant = ConstantOrIdentifierOrThis;

            auto* ConstantNode =  ParseConstantNode();
            Advance();

            return ASTALLOC.Alloc<types::PrimaryExpressionNode>(ConstantNode);

        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::IDENTIFIER) {
            auto& Identifier = ConstantOrIdentifierOrThis;
            Advance();

            assert(std::holds_alternative<std::string_view>(Identifier.Value));
            auto* IdentifierNode = ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(Identifier.Value));

            auto* PrimaryExpressionNode = ASTALLOC.Alloc<types::PrimaryExpressionNode>(IdentifierNode);

            return PrimaryExpressionNode;
        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::K_THIS) {
            Advance();

            auto* ThisNode = ASTALLOC.Alloc<types::ThisNode>();
            auto* PrimaryExpressionNode = ASTALLOC.Alloc<types::PrimaryExpressionNode>(ThisNode);
            return PrimaryExpressionNode;
        } else {
            // TODO: throw error
            assert(false);
        }

        return nullptr;
    }


    types::ExpressionNode* Parser::ParseFactor() {

        if (Check(types::TokenType::LPAREN_ROUND))
        {
            // try parsing contents as a type name for c style cast
            size_t prev = current;
            Advance();

            auto TypeNameOpt = TryParseTypeName();
            if (TypeNameOpt.has_value())
            {
                auto Declarator = ParseAbstractDeclarator();
                if (!Match(types::TokenType::RPAREN_ROUND))
                {
                    //todo throw error
                    assert(false && "Expected ')' after type name in cast");
                }
                auto CastExpression = ParseFactor();
                return ASTALLOC.Alloc<types::CastFactorNode>(TypeNameOpt.value(), Declarator, CastExpression);
            }

            current = prev;
        }

        if (detail::IsUnaryOperator(Peek().TokenType_v)) {
            types::ASTUnaryOp UnaryOp = types::TokenTypeToASTUnaryOp[Peek().TokenType_v];

            if(UnaryOp == types::ASTUnaryOp::OP_SIZEOF) {
                return ParseSizeofExpression();
            } else {
                Advance();
                types::ExpressionNode* Expr = ParseFactor();
                return ASTALLOC.Alloc<types::FactorNode>(UnaryOp, Expr);
            }
        }

        return ParsePostfixExpression();
    }

    // for ++, -- , [], ->, .
    types::ExpressionNode* Parser::ParsePostfixExpression() {
        types::ExpressionNode* Left = ParsePrimaryExpression(); // can be int, (expr), constants, identifier
        // primary expr because cannot be something like 'a-- --' or 'a--[]' - need be brakcetted

        auto isPostfixExprOp = [](types::TokenType TokenType_v) -> bool {
            switch(TokenType_v) {
                case types::TokenType::OP_INC:
                case types::TokenType::OP_DEC:
                case types::TokenType::OP_DOT:
                case types::TokenType::OP_ARROW:
                case types::TokenType::LPAREN_SQUARE:
                case types::TokenType::LPAREN_ROUND:
                    return true;
                default:
                    return false;
            }
        };

        while(isPostfixExprOp(Peek().TokenType_v)) {
            types::TokenType Op = Advance().TokenType_v;

            if(Op == types::TokenType::LPAREN_SQUARE) {

                types::ExpressionNode* Index = ParseCommaExpression();
                Left = ASTALLOC.Alloc<types::IndexPostfixExpr>(Left, Index);

                if(!Match(types::TokenType::RPAREN_SQUARE)) {
                    // todo throw error
                }
            } else if(Op == types::TokenType::OP_INC || Op == types::TokenType::OP_DEC) {


                Left = ASTALLOC.Alloc<types::UnaryPostfixExpr>(Left, types::TokenTypeToASTUnaryOp.at(Op));

            } else if(Op == types::TokenType::OP_DOT) {
                // (expr).Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    // todo throw error
                }

                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                auto* IdentifierNode = ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(Identifier.Value));

                Left = ASTALLOC.Alloc<types::MemberPostfixExpr>(Left, IdentifierNode);

            } else if(Op == types::TokenType::OP_ARROW) {
                // (expr)->Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    // todo throw error
                }
                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                types::IdentifierNode* IdentifierNode = ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(Identifier.Value));

                Left = ASTALLOC.Alloc<types::ArrowPostfixExpr>(Left, IdentifierNode);
            } else {
                // Identifier(expr) expr will have commas
                // (2+2)(1,2) will allow this

                types::ExpressionNode* ArgsList = nullptr;
                if (!Check(types::TokenType::RPAREN_ROUND)) {
                    ArgsList = ParseCommaExpression();
                } else {
                    ArgsList = ASTALLOC.Alloc<types::CommaExpressionNode>(std::vector<types::ExpressionNode*>{});
                }

                if(!Match(types::TokenType::RPAREN_ROUND)) {
                    // todo throw error
                }

                Left = ASTALLOC.Alloc<types::FunctionCallPostfixExpr>(Left, ArgsList);
            }
        }

        return Left;
    }

    types::ExpressionNode* Parser::ParseAssignmentExpression(size_t MinPrecedence)
    {
        auto* LeftFactor = ParseFactor();
        auto Op = Peek().TokenType_v;
        while (IsBinaryOperator(Op) && GetPrecedence(Op) >= MinPrecedence && Op != types::TokenType::OP_COMMA) {
            Advance();
            if (IsRightToLeft(Op))
            {
                if (Op == types::TokenType::OP_QUESTION) {
                    auto* ThenExpr = ParseAssignmentExpression();
                    if (!Match(types::TokenType::OP_COLON)) {
                        // todo throw error
                    }

                    auto* ElseExpr = ParseAssignmentExpression();

                    LeftFactor = ASTALLOC.Alloc<types::TernaryOpNode>(LeftFactor, ThenExpr, ElseExpr);
                } else {
                    auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op));
                    LeftFactor = ASTALLOC.Alloc<types::BinaryExpressionNode>(types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
                }
            }
            else
            {
                auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op) + 1);
                LeftFactor = ASTALLOC.Alloc<types::BinaryExpressionNode>(types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
            }
            Op = Peek().TokenType_v;
        }

        return LeftFactor;
    }

    types::ExpressionNode* Parser::ParseCommaExpression()
    {
        std::vector<types::ExpressionNode*> Args;
        auto* LeftFactor = ParseAssignmentExpression();

        Args.push_back(LeftFactor);
        while (Match(types::TokenType::OP_COMMA)) {
            auto* RightFactor = ParseAssignmentExpression();
            Args.push_back(RightFactor);
        }
        return ASTALLOC.Alloc<types::CommaExpressionNode>(std::move(Args));
    }

}
}
