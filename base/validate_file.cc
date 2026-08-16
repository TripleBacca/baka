#include <filesystem>

namespace baka {
    namespace base {
        bool ValidateFile(std::filesystem::path path) {
            if (!std::filesystem::exists(path)) {
                return false;
            }
            return true;
        }
    }
}
