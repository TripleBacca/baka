#pragma once
#include <cstddef>
#include <string_view>

namespace baka {
namespace heuristics {

    constexpr size_t DEFECTS_PER_KLOC = 40;
    constexpr size_t CHARS_PER_LINE = 25;

    constexpr size_t OptimisticCountDefects(const std::string_view& SourceCode) {
        size_t realDefects = ((1.0 * SourceCode.size() * DEFECTS_PER_KLOC)/(CHARS_PER_LINE * 1000)) + 1;
        return (realDefects << 1); // double cuz why not
    };
}
}
