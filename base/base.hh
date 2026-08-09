#pragma once
#include "mmap_file.hh"
#include <filesystem>


namespace baka {
namespace base {

    bool ValidateFile(std::filesystem::path path);
    size_t FileSize(std::filesystem::path path);

}
}
