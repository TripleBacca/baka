#include "types/parser/ast/initializer.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
namespace parser {


    types::InitializerNode* Parser::ParseInitializer() {
        auto* IntializerNodeRet = ASTALLOC.Alloc<types::InitializerNode>();

        // struct X {
        //     int a;
        //     int b;
        // };
        // X b = {1,2,}; // trailing comma

        if(Match(types::TokenType::LPAREN_CURLY)) {
            if(Match(types::TokenType::RPAREN_CURLY)) {
                return IntializerNodeRet; // default intializer;
            }

            IntializerNodeRet->addExpression(ParseInitializer());
            while(Match(types::TokenType::OP_COMMA)) {
                if(Check(types::TokenType::RPAREN_CURLY)) break;

                IntializerNodeRet->addExpression(ParseInitializer());
            }

            if(!Match(types::TokenType::RPAREN_CURLY)) {
                ReportError("expected '}'");
                auto Sync = SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_CURLY) {
                    Advance();
                } else if (Sync == types::TokenType::SEMICOLON){
                    Advance();
                }
            }

        } else {
            auto* Expr = ParseAssignmentExpression();
            if (Expr) {
                IntializerNodeRet->addExpression(Expr);
            }
        }

        return IntializerNodeRet;
    }

}
}
