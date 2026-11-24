#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/typedef.hh"
namespace baka {
namespace types {
    // Access written for a member or base. Unspecified takes the aggregate's default
    // (private for class, public for struct and union).
    enum class AccessSpecifier { Unspecified, Public, Protected, Private };

    inline const char* AccessSpecifierName(AccessSpecifier access) {
        switch (access) {
        case AccessSpecifier::Public: return "public";
        case AccessSpecifier::Protected: return "protected";
        case AccessSpecifier::Private: return "private";
        default: return "unspecified";
        }
    }

    class StructBodyNode : public ASTNode {
        std::vector<DeclarationList*> Declarations;
        std::vector<FunctionNode*> Functions; // member functions
        std::vector<TypedefNode*> Typedefs;

        std::vector<ConstructorNode*> Constructors;
        std::vector<DestructorNode*> Destructors;
        std::vector<ASTNode*> Members;
        std::vector<AccessSpecifier> MemberAccess; // parallel to Members
        AccessSpecifier CurrentAccess = AccessSpecifier::Unspecified;

        void push(ASTNode* member) {
            Members.push_back(member);
            MemberAccess.push_back(CurrentAccess);
        }

        public:
        void visitChildren(const NodeVisitor& visit) const override { for (auto* child : getMembers()) visit(child); }
            StructBodyNode() = default;

            void AddDeclaration(DeclarationList* decl) {
                Declarations.push_back(decl);
                push(decl);
            }

            void AddFunction(FunctionNode* func) {
                Functions.push_back(func);
                push(func);
            }

            void AddTypedef(TypedefNode* typedefNode) {
                Typedefs.push_back(typedefNode);
                push(typedefNode);
            }

            void SetConstructor(ConstructorNode* func) {
                Constructors.push_back(func);
                push(func);
            }

            void SetDestructor(DestructorNode* func) {
                Destructors.push_back(func);
                push(func);
            }

            const std::vector<ASTNode*>& getMembers() const {
                return Members;
            }

            // An access label applies to every member parsed after it.
            void setCurrentAccess(AccessSpecifier access) {
                CurrentAccess = access;
            }

            AccessSpecifier getMemberAccess(size_t index) const {
                return index < MemberAccess.size() ? MemberAccess[index] : AccessSpecifier::Unspecified;
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
                for (size_t Index = 0; Index < Members.size(); ++Index) {
                    if (MemberAccess[Index] != AccessSpecifier::Unspecified) {
                        INDENT(Tabs + 1);
                        std::cout << "Access: " << AccessSpecifierName(MemberAccess[Index]) << std::endl;
                    }
                    Members[Index]->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

}
}
