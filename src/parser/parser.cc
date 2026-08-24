#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/parser/ast.hh"
#include "types/token/token.hh"

#define ASTALLOC base::ASTNodeArena::getInstance()

namespace baka{
namespace parser{
    bool Parser::Check(types::TokenType type) const noexcept {
        if(current >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

        return Tokens[current].TokenType_v == type;
    }

    const types::Token& Parser::Advance() {
        return Tokens[current++];
    }

    bool Parser::Match(types::TokenType type) {
        if(Check(type)) {
            Advance();
            return true;
        }
        return false;
    }

    const types::Token& Parser::Peek() const noexcept {
        if(current >= Tokens.size()) return Tokens.back();

        return Tokens[current];
    }

    const types::Token& Parser::Previous() const {
        return Tokens[current - 1];
    }

    types::ASTNode* Parser::Parse() {
        types::ASTNode* node = ASTALLOC.Alloc<types::TrialNode>();
        return node;
    }
}
}
