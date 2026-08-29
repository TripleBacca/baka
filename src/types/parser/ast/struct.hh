#pragma once
#include <iostream>
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/structBody.hh"
#include "utils.hh"
#include "ast_node.hh"

namespace baka {
    namespace types{

        class StructDefinitionNode : public ASTNode {
            IdentifierNode* StructName;
            IdentifierNode* ParentStructName = nullptr;
            StructBodyNode* Body;

        public:
            StructDefinitionNode(IdentifierNode* identifier, StructBodyNode* body) :
            StructName(identifier), Body(body) {
            }
            StructDefinitionNode(IdentifierNode* identifier, IdentifierNode* parent, StructBodyNode* body) :
            StructName(identifier), ParentStructName(parent), Body(body) {
            }

            ~StructDefinitionNode() override = default;

            void Print(size_t Tabs) const override {

                INDENT(Tabs);
                std::cout << "Struct(" << std::endl;
                StructName->Print(Tabs + 1);
                if (ParentStructName) {
                    INDENT(Tabs+1);
                    std::cout << "ParentStruct : " << std::endl;
                    ParentStructName->Print(Tabs+2);
                }
                Body->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class StructDeclarationNode : public ASTNode
        {
            IdentifierNode* StructName;
        public:
            StructDeclarationNode(IdentifierNode* identifier) : StructName(identifier) {
            }

            ~StructDeclarationNode() override = default;

            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "StructDeclaration(" << std::endl;
                StructName->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
