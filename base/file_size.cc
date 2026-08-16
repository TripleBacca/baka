#include <cstddef>
#include <filesystem>

namespace baka {
    namespace base {
        std::size_t FileSize(std::filesystem::path path) {
            return std::filesystem::file_size(path);
        }
    }
}
