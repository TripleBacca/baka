#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/function.hh"
namespace baka {
namespace types {
    class StructBodyNode : public ASTNode {
        std::vector<DeclarationList*> Declarations;
        std::vector<FunctionNode*> Functions; // member functions

        ConstructorNode* Constructor = nullptr; // TODO: do we generate defaults later?
        DestructorNode* Destructor = nullptr;

        public:
            StructBodyNode() = default;

            void AddDeclaration(DeclarationList* decl) {
                Declarations.push_back(decl);
            }

            void AddFunction(FunctionNode* func) {
                Functions.push_back(func);
            }

            void SetConstructor(ConstructorNode* func) {
                Constructor = func;
            }

            void SetDestructor(DestructorNode* func) {
                Destructor = func;
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "StructBodyNode(:\n";
                for (const auto& decl : Declarations) {
                    decl->Print(Tabs + 1);
                }
                for (const auto& func : Functions) {
                    func->Print(Tabs + 1);
                }
                if (Constructor) {
                    Constructor->Print(Tabs + 1);
                }
                if (Destructor) {
                    Destructor->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;;
            }
    };

}
}
