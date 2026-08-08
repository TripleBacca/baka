#include <cstdio>
#include <fstream>
#include <filesystem>
#include <syscall.h>

namespace baka {

    namespace base {

        class MMaapFile {
            std::ifstream file_;


            MMaapFile(const std::filesystem::path& file_) {
                FILE* file_ptr = fopen(file_.c_str(), "r");
                int fd = open(filename, O_RDWR, 0666);

            }

        }





    }
}
