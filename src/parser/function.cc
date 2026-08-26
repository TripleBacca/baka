#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
    namespace parser {
        types::FunctionArgumentStatementNode* Parser::FunctionArgumentStatement() {
            //TODO add support for default arguments
            //TODO add support for struct enum etc declarations
            //TODO add support for unsigned const etc declarations
            const types::Token& DataType = this->Advance();

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            types::IdentifierNode* VariableName = ParseIdentifier();

            types::FunctionArgumentStatementNode* Node = ASTALLOC.Alloc<types::FunctionArgumentStatementNode>(
                std::get<std::string_view>(DataType.Value), VariableName);

            return Node;
        }

        types::FunctionArgumentsNode* Parser::FunctionArguments() {
            std::vector<types::FunctionArgumentStatementNode*> Decls;
            bool HasEllipsis = false;
            bool ReachedEnd = false;
            while (!Check(types::TokenType::RPAREN_ROUND) && !ReachedEnd) {
                if (Match(types::TokenType::OP_ELLIPSIS)) {
                    HasEllipsis = true;
                    ReachedEnd = true;
                }
                else {
                    Decls.push_back(this->FunctionArgumentStatement());
                    if (!Match(types::TokenType::OP_COMMA)) {
                        ReachedEnd = true;
                    }
                }
            }
            if (!Check(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            return ASTALLOC.Alloc<types::FunctionArgumentsNode>(Decls, HasEllipsis);
        }

        types::FunctionNode* Parser::Function() {
            const types::Token& ReturnType = this->Advance();

            if (!this->Check(types::TokenType::IDENTIFIER)) {
                // throw error
            }
            types::IdentifierNode* FunctionIdentifier = this->ParseIdentifier();


            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::FunctionArgumentsNode* Args = this->FunctionArguments();

            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }


            if (!this->Match(types::TokenType::LPAREN_CURLY)) {
                // throw error
            }
            types::JumpStatementNode* Body = this->JumpStatement();
            if (!this->Match(types::TokenType::RPAREN_CURLY)) {
                // throw error
            }
            types::FunctionNode* Node = ASTALLOC.Alloc<types::FunctionNode>(
                std::get<std::string_view>(ReturnType.Value), FunctionIdentifier, Args, Body);

            return Node;
        }
    }
}
