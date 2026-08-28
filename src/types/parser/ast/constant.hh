#pragma once
#include <cstddef>
#include <iostream>
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"

namespace baka {

    namespace types {

        class ConstantNode : public ASTNode {
            public:
                virtual ~ConstantNode() = default;
        };


        class ConstantIntNode : public ConstantNode {
            int Value;

            public:
                ConstantIntNode(int val) : Value(val) {
                }

                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(" << Value << ")" << std::endl;
                }
        };


        // todo:
    }
}
