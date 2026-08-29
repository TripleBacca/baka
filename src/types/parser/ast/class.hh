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
            StructBodyNode* Body;

        public:
            ClassDefinitionNode(IdentifierNode* identifier, StructBodyNode* body) :
            ClassName(identifier), Body(body) {
            }
            ClassDefinitionNode(IdentifierNode* identifier, IdentifierNode* parent, StructBodyNode* body) :
            ClassName(identifier), ParentClassName(parent), Body(body) {
            }

            ~ClassDefinitionNode() override = default;

            void Print(size_t Tabs) const override {

                INDENT(Tabs);
                std::cout << "Class(" << std::endl;
                ClassName->Print(Tabs + 1);
                if (ParentClassName) {
                    INDENT(Tabs+1);
                    std::cout << "ParentClass : " << std::endl;
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
            ClassDeclarationNode(IdentifierNode* identifier) : ClassName(identifier) {
            }

            ~ClassDeclarationNode() override = default;

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
