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
            const types::Token& VariableName = this->Advance();

            types::FunctionArgumentStatementNode* Node = ASTALLOC.Alloc<types::FunctionArgumentStatementNode>(
                std::get<std::string_view>(DataType.Value), std::get<std::string_view>(VariableName.Value));

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
            const types::Token& FunctionIdentifier = this->Advance();
            if (!std::holds_alternative<std::string_view>(FunctionIdentifier.Value)) {
                // throw error
            }


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
            types::ReturnStatementNode* Body = this->ReturnStatement();
            if (!this->Match(types::TokenType::RPAREN_CURLY)) {
                // throw error
            }
            types::FunctionNode* Node = ASTALLOC.Alloc<types::FunctionNode>(
                std::get<std::string_view>(ReturnType.Value), std::get<std::string_view>(FunctionIdentifier.Value), Args, Body);

            return Node;
        }
    }
}
