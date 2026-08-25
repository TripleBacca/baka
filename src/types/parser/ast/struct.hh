#pragma once
#include <iostream>
#include <string_view>
#include <vector>
#include "utils.hh"
#include "ast_node.hh"
#include "statement.hh"

namespace baka {
    namespace types{
        class StructDeclarationStatementNode : public StatementNode {
            std::string_view DataType;
            IdentiferNode* VariableName;

        public:
            StructDeclarationStatementNode(std::string_view dataType, IdentiferNode* variableName) :
                DataType(dataType), VariableName(variableName) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "StructDeclaration(" << DataType << ", " << '\n';

                VariableName->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class StructNode : public ASTNode {
            IdentiferNode* StructName;
            std::vector<StructDeclarationStatementNode*> Body;

        public:
            StructNode(IdentiferNode* identifier,
                       std::vector<StructDeclarationStatementNode*> body) : StructName(identifier),
                                                                            Body(std::move(body)) {
            }

            ~StructNode() = default;

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Struct(" << StructName << ", " << '\n';

                for (auto decl : Body) {
                    decl->Print(Tabs + 1);
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

    }
}
