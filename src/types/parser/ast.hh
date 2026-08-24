#pragma once

#include <iostream>
#include <ostream>
namespace baka {
namespace types {
    class ASTNode {
        public:
            ASTNode() = default;
            virtual ~ASTNode() = default;

            virtual void Print() const = 0;
    };

    class TrialNode : public ASTNode {
        public:
            TrialNode() = default;
            ~TrialNode() = default;

            void Print() const override {
                std::cout << "balls" << std::endl;
            }
    };

}
}
