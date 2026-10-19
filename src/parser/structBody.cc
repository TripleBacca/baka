
#include "types/parser/ast/structBody.hh"
#include "parser.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
namespace parser {

    types::StructBodyNode* Parser::ParseStructBody(types::IdentifierNode* ParentName) {
        const size_t SourceStart = current;
        types::StructBodyNode* Node = AllocNodeAt<types::StructBodyNode>(SourceStart);

        if(!Match(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{'");
        }

        while(!Match(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN)) {

            auto isCtor = [&]() -> bool {
                if(Peek().TokenType_v == types::TokenType::IDENTIFIER && Check2(types::TokenType::LPAREN_ROUND)) {
                    const std::string_view& Name = std::get<std::string_view>(Peek().Value);
                    if(ParentName && Name == ParentName->GetName()) {
                        return true;
                    }
                }

                return false;
            };

            if (isCtor()) {
                types::ConstructorNode* Ctor = ParseConstructor(ParentName);
                Node->SetConstructor(Ctor);

            } else if(Check(types::TokenType::OP_TILDE)) {
                auto* destructorParent = ParentName;
                if (!destructorParent) {
                    ReportError("anonymous aggregates cannot declare destructors");
                    destructorParent = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?"));
                }
                types::DestructorNode* Dtor = ParseDestructor(destructorParent);
                Node->SetDestructor(Dtor);

            } else if(Check(types::TokenType::K_TYPEDEF)) {
                types::TypedefNode* Typedef = ParseTypedef();
                Node->AddTypedef(Typedef);
            } else {
                std::optional<types::FunctionNode*> Func = TryParseFunction();
                if(Func) {
                    Node->AddFunction(Func.value());
                } else {
                    types::DeclarationList* Decl = ParseDeclarationList();
                    Node->AddDeclaration(Decl);
                }
            }
        }

        return Node;
    }

}
}
