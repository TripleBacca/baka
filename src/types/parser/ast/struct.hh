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
            std::string_view VariableName;

        public:
            StructDeclarationStatementNode(std::string_view dataType, std::string_view variableName) :
                DataType(dataType), VariableName(variableName) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "StructDeclaration(" << DataType << ", " << '\n';

                INDENT(Tabs + 1);
                std::cout << VariableName << '\n';

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class StructNode : public ASTNode {
            std::string_view StructName;
            std::vector<StructDeclarationStatementNode*> Body;

        public:
            StructNode(std::string_view identifier,
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
