#pragma once
#include <string_view>
#include <unordered_map>
#include <iostream>

#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "

namespace baka {

namespace types {

    enum class TypeSpecifierModifier {
        NONE,
        UNSIGNED,
        STRUCT,
        CLASS,
        ENUM,
        UNION
    };

    inline std::unordered_map<TypeSpecifierModifier, std::string_view> TypeSpecifierModifierToStr = {
        {TypeSpecifierModifier::NONE, "None"},
        {TypeSpecifierModifier::UNSIGNED, "Unsigned"},
        {TypeSpecifierModifier::STRUCT, "Struct"},
        {TypeSpecifierModifier::CLASS, "Class"},
        {TypeSpecifierModifier::ENUM, "Enum"},
        {TypeSpecifierModifier::UNION, "Union"}
    };

}

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

