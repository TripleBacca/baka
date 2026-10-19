#pragma once
#include <string_view>
#include <cstdint>
#include <iostream>
#include <unordered_map>

#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "

namespace baka {

namespace types {

    // Before semantics the ID is zero, preserving parser-only AST output.
    // On a loop/switch/label it defines an identity; on a jump it names the
    // resolved destination. The backend maps this ID to its own basic blocks.
    inline void PrintControlTarget(uint32_t target, size_t tabs) {
        if (!target) return;
        INDENT(tabs);
        std::cout << "Target: " << target << '\n';
    }


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
