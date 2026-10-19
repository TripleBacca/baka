
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
        const size_t SourceStart = current;
        auto& ConstantOrIdentifierOrThis = Peek();


        if(Match(types::TokenType::LPAREN_ROUND)) {
            types::ExpressionNode* Node = ParseCommaExpression();
            if(!Match(types::TokenType::RPAREN_ROUND)) {
                if (Node) ReportError("expected ')'");
            }
            return Node;
        }

        if (detail::isConstantToken(ConstantOrIdentifierOrThis.TokenType_v)) {
            auto& Constant = ConstantOrIdentifierOrThis;

            auto* ConstantNode =  ParseConstantNode();
            Advance();

            return AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, ConstantNode);

        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::IDENTIFIER) {
            auto& Identifier = ConstantOrIdentifierOrThis;
            Advance();

            assert(std::holds_alternative<std::string_view>(Identifier.Value));
            auto* IdentifierNode = AllocNodeAt<types::IdentifierNode>(SourceStart, std::get<std::string_view>(Identifier.Value));

            auto* PrimaryExpressionNode = AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, IdentifierNode);

            return PrimaryExpressionNode;
        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::K_THIS) {
            Advance();

            auto* ThisNode = AllocNodeAt<types::ThisNode>(SourceStart);
            auto* PrimaryExpressionNode = AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, ThisNode);
            return PrimaryExpressionNode;
        } else {
            ReportError("expected expression");
            return nullptr;
        }

        return nullptr;
    }


    types::ExpressionNode* Parser::ParseFactor() {
        const size_t SourceStart = current;

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
                    ReportError("expected ')'");
                }
                auto CastExpression = ParseFactor();
                return AllocNodeAt<types::CastFactorNode>(SourceStart, TypeNameOpt.value(), Declarator, CastExpression);
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
                if (Expr == nullptr) return nullptr;
                return AllocNodeAt<types::FactorNode>(SourceStart, UnaryOp, Expr);
            }
        }

        return ParsePostfixExpression();
    }

    // for ++, -- , [], ->, .
    types::ExpressionNode* Parser::ParsePostfixExpression() {
        const size_t SourceStart = current;
        types::ExpressionNode* Left = ParsePrimaryExpression(); // can be int, (expr), constants, identifier
        if (Left == nullptr) return nullptr;
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
            const size_t OperatorStart = current;
            types::TokenType Op = Advance().TokenType_v;

            if(Op == types::TokenType::LPAREN_SQUARE) {

                types::ExpressionNode* Index = ParseCommaExpression();
                if (Index == nullptr) return nullptr;
                Left = AllocNodeAt<types::IndexPostfixExpr>(OperatorStart, Left, Index);

                if(!Match(types::TokenType::RPAREN_SQUARE)) {
                    ReportError("expected ']'");
                }
            } else if(Op == types::TokenType::OP_INC || Op == types::TokenType::OP_DEC) {


                Left = AllocNodeAt<types::UnaryPostfixExpr>(OperatorStart, Left, types::TokenTypeToASTUnaryOp.at(Op));

            } else if(Op == types::TokenType::OP_DOT) {
                // (expr).Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after '.'");
                    return nullptr;
                }

                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                auto* IdentifierNode = AllocNodeAt<types::IdentifierNode>(current - 1, std::get<std::string_view>(Identifier.Value));

                Left = AllocNodeAt<types::MemberPostfixExpr>(OperatorStart, Left, IdentifierNode);

            } else if(Op == types::TokenType::OP_ARROW) {
                // (expr)->Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after '->'");
                    return nullptr;
                }
                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                types::IdentifierNode* IdentifierNode = AllocNodeAt<types::IdentifierNode>(current - 1, std::get<std::string_view>(Identifier.Value));

                Left = AllocNodeAt<types::ArrowPostfixExpr>(OperatorStart, Left, IdentifierNode);
            } else {
                // Identifier(expr) expr will have commas
                // (2+2)(1,2) will allow this

                types::ExpressionNode* ArgsList = nullptr;
                if (!Check(types::TokenType::RPAREN_ROUND)) {
                    ArgsList = ParseCommaExpression();
                    if (ArgsList == nullptr) return nullptr;
                } else {
                    ArgsList = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::vector<types::ExpressionNode*>{});
                }

                if(!Match(types::TokenType::RPAREN_ROUND)) {
                    ReportError("expected ')'");
                }

                Left = AllocNodeAt<types::FunctionCallPostfixExpr>(OperatorStart, Left, ArgsList);
            }
        }

        return Left;
    }

    types::ExpressionNode* Parser::ParseAssignmentExpression(size_t MinPrecedence)
    {
        const size_t SourceStart = current;
        auto* LeftFactor = ParseFactor();
        if (LeftFactor == nullptr) return nullptr;

        auto Op = Peek().TokenType_v;
        while (IsBinaryOperator(Op) && GetPrecedence(Op) >= MinPrecedence && Op != types::TokenType::OP_COMMA) {
            const size_t OperatorStart = current;
            Advance();
            if (IsRightToLeft(Op))
            {
                if (Op == types::TokenType::OP_QUESTION) {
                    // The middle operand is a full expression; an unparenthesized comma belongs to it.
                    auto* ThenExpr = ParseAssignmentExpression();
                    if (ThenExpr && Check(types::TokenType::OP_COMMA)) {
                        std::vector<types::ExpressionNode*> Middle{ThenExpr};
                        while (Match(types::TokenType::OP_COMMA)) {
                            auto* Next = ParseAssignmentExpression();
                            if (!Next) return nullptr;
                            Middle.push_back(Next);
                        }
                        ThenExpr = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Middle));
                    }
                    if (ThenExpr == nullptr) return nullptr;
                    if (!Match(types::TokenType::OP_COLON)) {
                        ReportError("expected ':' in ternary expression");
                    }

                    auto* ElseExpr = ParseAssignmentExpression();
                    if (ElseExpr == nullptr) return nullptr;

                    LeftFactor = AllocNodeAt<types::TernaryOpNode>(OperatorStart, LeftFactor, ThenExpr, ElseExpr);
                } else {
                    auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op));
                    if (RightFactor == nullptr) return nullptr;
                    LeftFactor = AllocNodeAt<types::BinaryExpressionNode>(OperatorStart, types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
                }
            }
            else
            {
                auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op) + 1);
                if (RightFactor == nullptr) return nullptr;
                LeftFactor = AllocNodeAt<types::BinaryExpressionNode>(OperatorStart, types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
            }
            Op = Peek().TokenType_v;
        }

        return LeftFactor;
    }

    types::ExpressionNode* Parser::ParseCommaExpression()
    {
        const size_t SourceStart = current;
        std::vector<types::ExpressionNode*> Args;
        auto* LeftFactor = ParseAssignmentExpression();
        if (LeftFactor == nullptr) return nullptr;

        Args.push_back(LeftFactor);
        while (Match(types::TokenType::OP_COMMA)) {
            auto* RightFactor = ParseAssignmentExpression();
            if (RightFactor == nullptr) return nullptr;
            Args.push_back(RightFactor);
        }
        return AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Args));
    }

}
}
