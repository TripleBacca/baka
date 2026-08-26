#include "types/parser/ast/function.hh"
#include "parser.hh"
#include "memory/custom_arenas.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::FunctionParameter* Parser::ParseFunctionParameter() {

            Match(types::TokenType::K_STRUCT); // eat

            bool IsConst = false;

            if(Match(types::TokenType::K_CONST)) {
                IsConst = true;
            }

            if (!detail::IsTypeOrIdentifier(this->Peek().TokenType_v)) {
                // throw error
                assert(false);
            }
            types::IdentifierNode* TypeName = ParseIdentifier();
            if(!LookupType(TypeName)) {
                // throw error
                assert(false);
            }

            types::SingleDeclarationNode* Node = ParseSingleDeclaration();

            types::FunctionParameter* Parameter = ASTALLOC.Alloc<types::FunctionParameter>(IsConst, TypeName, Node);
            return Parameter;
        }

        types::FunctionParameterList* Parser::ParseFunctionParameterList() {
            // parses <functionparam>, <functionparam> , ...

            std::vector<types::FunctionParameter*> Decls;

            if(Check(types::TokenType::RPAREN_ROUND)) {
                // empty parameter list
                return ASTALLOC.Alloc<types::FunctionParameterList>(Decls);
            }

            bool DefaultRunning = false;
            do {
                if(this->Match(types::TokenType::OP_ELLIPSIS)) {
                    break;
                }

                auto* FunctionParam = ParseFunctionParameter();

                if(FunctionParam->hasInitalizer()) {
                    DefaultRunning = true;
                } else if (DefaultRunning) {
                    // default shoudl be in end
                    // throw error
                    assert(false);
                }

                Decls.push_back(FunctionParam);

            } while (this->Match(types::TokenType::OP_COMMA));

            return ASTALLOC.Alloc<types::FunctionParameterList>(Decls);
        }

        types::FunctionNode* Parser::Function() {
            if (!detail::IsTypeOrIdentifier(this->Peek().TokenType_v)) {
                // throw error
                assert(false);
            }
            types::IdentifierNode* ReturnTypeIdentifier = this->ParseIdentifier();
            if(!LookupType(ReturnTypeIdentifier)) {
                // throw error
                assert(false);
            }
            types::IdentifierNode* FunctionIdentifier = this->ParseIdentifier();

            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::FunctionParameterList* Args = this->ParseFunctionParameterList();
            if(!this->Match(types::TokenType::RPAREN_ROUND)) {
                // throw error
            }

            // TODO: do statement
            types::JumpStatementNode* Body = this->JumpStatement();
            types::FunctionNode* Node = ASTALLOC.Alloc<types::FunctionNode>(ReturnTypeIdentifier, FunctionIdentifier, Args, Body);

            return Node;
        }
    }
}
