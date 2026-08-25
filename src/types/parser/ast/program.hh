#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <vector>

namespace baka {
    namespace types {
        class ProgramNode : public ASTNode {
            std::vector<ASTNode*> Body;
            // TODO: should have a vector of declarations

        public:
            ProgramNode(std::vector<ASTNode*> body) : Body(std::move(body)) {
            }

            ~ProgramNode() = default;

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Program(" << '\n';
                for (const auto tlBlock : Body) {
                    tlBlock->Print(Tabs + 1);
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
