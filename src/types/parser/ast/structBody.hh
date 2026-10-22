#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/typedef.hh"
namespace baka {
namespace types {
    class StructBodyNode : public ASTNode {
        std::vector<DeclarationList*> Declarations;
        std::vector<FunctionNode*> Functions; // member functions
        std::vector<TypedefNode*> Typedefs;

        std::vector<ConstructorNode*> Constructors;
        std::vector<DestructorNode*> Destructors;
        std::vector<ASTNode*> Members;

        public:
        void visitChildren(const NodeVisitor& visit) const override { for (auto* child : getMembers()) visit(child); }
            StructBodyNode() = default;

            void AddDeclaration(DeclarationList* decl) {
                Declarations.push_back(decl);
                Members.push_back(decl);
            }

            void AddFunction(FunctionNode* func) {
                Functions.push_back(func);
                Members.push_back(func);
            }

            void AddTypedef(TypedefNode* typedefNode) {
                Typedefs.push_back(typedefNode);
                Members.push_back(typedefNode);
            }

            void SetConstructor(ConstructorNode* func) {
                Constructors.push_back(func);
                Members.push_back(func);
            }

            void SetDestructor(DestructorNode* func) {
                Destructors.push_back(func);
                Members.push_back(func);
            }

            const std::vector<ASTNode*>& getMembers() const {
                return Members;
            }

            const std::vector<DeclarationList*>& getDeclarations() const {
                return Declarations;
            }

            const std::vector<FunctionNode*>& getFunctions() const {
                return Functions;
            }

            const std::vector<TypedefNode*>& getTypedefs() const {
                return Typedefs;
            }

            const std::vector<ConstructorNode*>& getConstructors() const {
                return Constructors;
            }

            const std::vector<DestructorNode*>& getDestructors() const {
                return Destructors;
            }


            bool empty() const { return Members.empty(); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "StructBodyNode(\n";
                for (auto* Member : Members) {
                    Member->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

}
}
