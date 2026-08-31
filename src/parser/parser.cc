#include "parser.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/program.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include "base/line_index.hh"
#include "driver/gctx.hh"
#include "types/driver/defs.hh"
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <optional>
#include <variant>

// TODO: deal with dtypddefs by mainting symbol table

namespace baka {
    namespace parser {
        bool Parser::Check(types::TokenType type) const noexcept {
            if (current >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

            return Tokens[current].TokenType_v == type;
        }

        bool Parser::Check2(types::TokenType type) const noexcept {
            if (current + 1 >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

            return Tokens[current + 1].TokenType_v == type;
        }

        const types::Token& Parser::Advance() {
            return Tokens[current++];
        }

        bool Parser::Match(types::TokenType type) {
            if (Check(type)) {
                Advance();
                return true;
            }
            return false;
        }

        const types::Token& Parser::Peek() const noexcept {
            if (current >= Tokens.size()) return Tokens.back();

            return Tokens[current];
        }

        const types::Token& Parser::Previous() const {
            return Tokens[current - 1];
        }


        types::ASTNode* Parser::Parse() {
            return ParseProgram();
        };

        types::ProgramNode* Parser::ParseProgram() {
            auto* ProgramNode = ASTALLOC.Alloc<types::ProgramNode>();

            while (!Check(types::TokenType::EOF_TOKEN)) {
                // function decl
                // declaration list
                // union decl
                // class decl
                // enum decl
                // typdef

                if (Check(types::TokenType::K_STRUCT)) {
                    auto Struct = this->ParseStruct();
                    if (std::holds_alternative<types::StructDefinitionNode*>(Struct)) {
                        ProgramNode->addNode(std::get<types::StructDefinitionNode*>(Struct));
                    } else {
                        ProgramNode->addNode(std::get<types::StructDeclarationNode*>(Struct));
                    }
                } else if (Check(types::TokenType::K_CLASS)) {
                    auto Class = this->ParseClass();
                    if (std::holds_alternative<types::ClassDefinitionNode*>(Class)) {
                        ProgramNode->addNode(std::get<types::ClassDefinitionNode*>(Class));
                    } else {
                        ProgramNode->addNode(std::get<types::ClassDeclarationNode*>(Class));
                    }
                } else if (Check(types::TokenType::K_UNION)) {
                    auto Union = this->ParseUnion();
                    ProgramNode->addNode(Union);
                } else if (Check(types::TokenType::K_ENUM)) {
                    auto Enum = this->ParseEnumDecl();
                    ProgramNode->addNode(Enum);
                } else if (Check(types::TokenType::K_TYPEDEF)) {
                    auto Typedef = this->ParseTypedef();
                    ProgramNode->addNode(Typedef);
                } else {
                    auto PossibleFunction = TryParseFunction();

                    if(PossibleFunction) {
                        ProgramNode->addNode(PossibleFunction.value());
                    } else {
                        // try parse function could have munched entire file in err path
                        if(!Check(types::TokenType::EOF_TOKEN)) {
                            auto* Node = ParseDeclarationList();
                            ProgramNode->addNode(Node);
                        }
                    }
                }
            }

            return ProgramNode;
        }

        types::ExpressionNode* Parser::Expression() {
            // todo: does this have to be different
            return ParseCommaExpression();
        }

        types::ConstantNode* Parser::ParseConstantNode() {
            const types::Token& Token = this->Peek();

            // int
            // string
            // char
            // float
            // double
            // long long
            // unsigned int
            // unsigned long long

            #define CONSTANT_NODE(T) else if(std::holds_alternative<T>(Token.Value)) do { \
                auto* cn = ASTALLOC.Alloc<types::ConstantTNode<T>>(std::get<T>(Token.Value)); \
                return cn; } while(0)

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
            else if (Check(types::TokenType::K_TRUE)) {
                auto* cn = ASTALLOC.Alloc<types::ConstantTNode<bool>>(true);
                return cn;
            } else if (Check(types::TokenType::K_FALSE)) {
                auto* cn = ASTALLOC.Alloc<types::ConstantTNode<bool>>(false);
                return cn;
            } else if (Check(types::TokenType::K_NULLPTR)) {
                auto* cn = ASTALLOC.Alloc<types::ConstantTNode<std::nullptr_t>>();
                return cn;
            }

            ReportError("expected constant");
            auto* cn = ASTALLOC.Alloc<types::ConstantTNode<int>>(0);
            cn->setHasError();
            return cn;
        }

        bool Parser::LookupType(types::IdentifierNode* Identifier) {
            return TypeLookup.Lookup(Identifier->GetName()).has_value();
        }

        bool Parser::LookupType(const std::string_view& IdentifierName) {
            return TypeLookup.Lookup(IdentifierName).has_value();
        }

        ParserSTE* Parser::GetParserSTE(types::IdentifierNode* Identifier) {
            auto* Entry = TypeLookup.GetEntry(Identifier->GetName());
            if (!Entry) {
                ReportError("unknown type name '" + std::string(Identifier->GetName()) + "'");
            }
            return Entry;
        }

        void Parser::EnterScope() {
            TypeLookup.EnterNewScope();
        }

        void Parser::ExitScope() {
            TypeLookup.ExitScope();
        }

        void Parser::AddType(types::IdentifierNode* Identifier) {
            TypeLookup.AddEntry(Identifier->GetName(), ParserSTE{});
        }

        bool Parser::isTypeName(const types::Token& token) {
            if(!std::holds_alternative<std::string_view>(token.Value))
                return false;

            return LookupType(std::get<std::string_view>(token.Value));
        }

        bool Parser::RegisterOrReplaceType(types::IdentifierNode* Identifier) {
            return TypeLookup.RegisterOrReplaceType(Identifier->GetName(), ParserSTE{});
        }

        void Parser::ReportError(std::string Message, bool ReportPreviousToken) {
            ReportedErrorCount++;

            if (TokenSourceLocations.empty()) return;

            size_t idx = current;
            if(ReportPreviousToken) idx--;

            idx = std::clamp(idx, 0uz, TokenSourceLocations.size() - 1);


            const auto& Loc = TokenSourceLocations[idx];
            base::LineCtx LineCtx_v{Loc.LineNo - 1, driver::Gctx::GetLineIndex().get()};
            driver::Gctx::GenerateLineError(Loc.LineNo, Loc.Col, LineCtx_v, std::move(Message), driver::Stage::PARSE);
        }

        types::TokenType Parser::SkipTo(std::initializer_list<types::TokenType> TokenList) {

            while (!Check(types::TokenType::EOF_TOKEN)) {
                for (auto Sync : TokenList) {
                    if (Check(Sync)) return Sync;
                }

                if (Check(types::TokenType::LPAREN_CURLY)) {
                    int Depth = 0;
                    do {
                        if (Check(types::TokenType::LPAREN_CURLY)) {
                            Depth++;
                        } else if (Check(types::TokenType::RPAREN_CURLY)) {
                            if (--Depth <= 0) {
                                Advance();
                                break;
                            }
                        }
                        Advance();
                    } while (!Check(types::TokenType::EOF_TOKEN));
                    continue;
                }

                Advance();
            }
            return types::TokenType::EOF_TOKEN;
        }

    }
}
