#pragma once

#include "types/driver/defs.hh"
#include <filesystem>
#include <string_view>

namespace baka::driver {
    void BuildMipsFromAssembly(std::string_view assembly, const std::filesystem::path& output, Stage stage);
    void BuildMipsFromFile(const std::filesystem::path& input, const std::filesystem::path& output, Stage stage);
}
