#pragma once
#include <cstddef>


namespace baka {
namespace types {

class ASTNode {
    bool hasError_ = false;
public:
    ASTNode() = default;
    virtual ~ASTNode() = default;

    virtual void Print(size_t Tabs = 0) const = 0;

    virtual void setHasError() {
        hasError_ = true;
    }

    bool hasError() const {
        return hasError_;
    }

};
}
}
