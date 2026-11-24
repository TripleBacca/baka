#pragma once
#include <iostream>
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/structBody.hh"
#include "utils.hh"
#include "ast_node.hh"

namespace baka {
    namespace types{

        class ClassDefinitionNode : public ASTNode {
            IdentifierNode* ClassName;
            IdentifierNode* ParentClassName = nullptr;
            AccessSpecifier BaseAccess = AccessSpecifier::Unspecified;
            StructBodyNode* Body;

        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getName()); visit(getBaseName()); visit(getBody()); }
            ClassDefinitionNode(IdentifierNode* identifier, StructBodyNode* body) :
            ClassName(identifier), Body(body) {
            }
            ClassDefinitionNode(IdentifierNode* identifier, IdentifierNode* parent, StructBodyNode* body) :
            ClassName(identifier), ParentClassName(parent), Body(body) {
            }

            ~ClassDefinitionNode() override = default;

            IdentifierNode* getName() const {
                return ClassName;
            }

            AccessSpecifier getBaseAccess() const {
                return BaseAccess;
            }

            void setBaseAccess(AccessSpecifier access) {
                BaseAccess = access;
            }

            IdentifierNode* getBaseName() const {
                return ParentClassName;
            }

            StructBodyNode* getBody() const {
                return Body;
            }


            void Print(size_t Tabs) const override {

                INDENT(Tabs);
                std::cout << "Class(" << std::endl;
                ClassName->Print(Tabs + 1);
                if (ParentClassName) {
                    INDENT(Tabs+1);
                    std::cout << "ParentClass : ";
                    if (BaseAccess != AccessSpecifier::Unspecified) {
                        std::cout << AccessSpecifierName(BaseAccess);
                    }
                    std::cout << std::endl;
                    ParentClassName->Print(Tabs+2);
                }
                Body->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class ClassDeclarationNode : public ASTNode
        {
            IdentifierNode* ClassName;
        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getName()); }
            ClassDeclarationNode(IdentifierNode* identifier) : ClassName(identifier) {
            }

            ~ClassDeclarationNode() override = default;

            IdentifierNode* getName() const {
                return ClassName;
            }


            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "ClassDeclaration(" << std::endl;
                ClassName->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
