#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <vector>

namespace baka {
    namespace types {
        class ProgramNode : public ASTNode {
            std::vector<ASTNode*> Body;
            // function decl
            // declaration list
            // union decl
            // class decl
            // enum decl
            // typedef

        public:
        void visitChildren(const NodeVisitor& visit) const override { for (auto* child : getBody()) visit(child); }
            ProgramNode(std::vector<ASTNode*> body) : Body(std::move(body)) {
            }

            ProgramNode() = default;

            void addNode(ASTNode* node) {
                Body.push_back(node);
            }

            const std::vector<ASTNode*>& getBody() const {
                return Body;
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
