#pragma once
#include <cstddef>


namespace baka {
namespace types {

class ASTNode {
public:
    ASTNode() = default;
    virtual ~ASTNode() = default;

    virtual void Print(size_t Tabs = 0) const = 0;
};

}
}
