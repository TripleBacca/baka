#include <cstdio>
#include <fstream>
#include <filesystem>
#include <syscall.h>

namespace baka {
    namespace base {


        class MappedFile {

            int fd;


            MappedFile(std::filesystem::path path) {
                fd = open(path.c_str(), ::O_RDONLY);
            }



        }




    }
}
