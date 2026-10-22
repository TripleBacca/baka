#pragma once
#include <cstddef>
#include <iostream>
#include <string_view>
#include "ast_node.hh"
#include "memory/custom_strings.hh"
#include "types/parser/ast/utils.hh"

namespace baka {

    namespace types {

        class ConstantNode : public ASTNode {
            public:
        void visitChildren(const NodeVisitor& visit) const override {  }
                virtual ~ConstantNode() = default;
        };

        // int
        // string - this is alloc
        // char
        // float
        // double
        // long long
        // unsigned int
        // unsigned long long
        // bool
        // nullptr

        // this guy creates copy for storing
        template<class T>
        class ConstantTNode : public ConstantNode {
            T Value;

            public:
                ConstantTNode(T val) : Value(val) {
                }

                const T& getValue() const { return Value; }
                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(" << Value << ")" << std::endl;
                }
        };

        // this guy views into arena
        template<>
        class ConstantTNode<base::SLString> : public ConstantNode {
            std::string_view Value;

            public:
                ConstantTNode(const base::SLString& val) : Value(val) {
                }

                std::string_view getValue() const { return Value; }

                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(" << Value << ")" << std::endl;
                }
        };

        template<>
        class ConstantTNode<std::nullptr_t> : public ConstantNode {
            public:
                std::nullptr_t getValue() const { return nullptr; }

                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(nullptr)" << std::endl;
                }
        };


    }
}
