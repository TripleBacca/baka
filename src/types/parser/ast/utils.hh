#pragma once
#include <string_view>
#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "



namespace baka {
namespace detail {

    inline bool isUnsignedTypeName(std::string_view TypeName) {
        if(TypeName == "long") return true;
        if(TypeName == "char") return true;
        if(TypeName == "int") return true;
        if(TypeName == "short") return true;
        return false;
    }
}
}
