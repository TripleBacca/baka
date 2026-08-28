
#include "memory/custom_strings.hh"
#include "parser.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/ternary.hh"
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




    types::ExpressionNode* Parser::ParsePrimaryExpression() {
        auto& ConstantOrIdentifier = Peek();

        if(Match(types::TokenType::LPAREN_ROUND)) {
            // TODO: can be a c style cast do later after symbol talbe
            types::ExpressionNode* Node = ParseCommaExpression();
            if(!Match(types::TokenType::RPAREN_ROUND)) {
                // TODO: throw erorr
            }
            return Node;
        }

        if (detail::isConstantToken(ConstantOrIdentifier.TokenType_v)) {
            auto& Constant = ConstantOrIdentifier;
            Advance();

            // int
            // string
            // char
            // float
            // double
            // long long
            // unsigned int
            // unsigned long long

            #define CONSTANT_NODE(T) else if(std::holds_alternative<T>(Constant.Value)) do { \
                auto* cn = ASTALLOC.Alloc<types::ConstantTNode<T>>(std::get<T>(Constant.Value)); \
                auto* Node = ASTALLOC.Alloc<types::PrimaryExpressionNode>(cn); \
                return Node; } while(0)

            if(false) {}
            CONSTANT_NODE(int);
            CONSTANT_NODE(base::SLString);
            CONSTANT_NODE(char);
            CONSTANT_NODE(std::string);
            CONSTANT_NODE(float);
            CONSTANT_NODE(double);
            CONSTANT_NODE(long long);
            CONSTANT_NODE(unsigned int);
            CONSTANT_NODE(unsigned long long);


        } else if(ConstantOrIdentifier.TokenType_v == types::TokenType::IDENTIFIER) {
            auto& Identifier = ConstantOrIdentifier;
            Advance();

            assert(std::holds_alternative<std::string_view>(Identifier.Value));
            types::IdentifierNode* IdentifierNode = ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(Identifier.Value));

            types::PrimaryExpressionNode* PrimaryExpressionNode = ASTALLOC.Alloc<types::PrimaryExpressionNode>(IdentifierNode);

            return PrimaryExpressionNode;
        } else {
            // TODO: throw error
        }

        return nullptr;
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
                types::IdentifierNode* IdentifierNode = ASTALLOC.Alloc<types::IdentifierNode>(std::get<std::string_view>(Identifier.Value));

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
