#pragma once
#include <iostream>
#include <vector>
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "utils.hh"
#include "ast_node.hh"

namespace baka {
    namespace types{

        class StructNode : public ASTNode {
            IdentifierNode* StructName;
            std::vector<DeclarationList*> Body;

        public:
            StructNode(IdentifierNode* identifier, std::vector<DeclarationList*> body) : StructName(identifier), Body(std::move(body)) {
            }

            ~StructNode() = default;

            void Print(size_t Tabs = 0) const override {

                INDENT(Tabs);
                std::cout << "Struct(";
                StructName->Print();

                for (auto decl : Body) {
                    decl->Print(Tabs + 1);
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

    }
}
