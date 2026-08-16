#include "mmap_file.hh"
#include <cstdio>
#include <filesystem>
#include <syscall.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "base.hh"

namespace baka {
    namespace base {
        MappedFile::MappedFile(std::filesystem::path path) : Length(FileSize(path)) {
            fd = open(path.c_str(), O_RDONLY);
            if (fd == -1) {
                throw std::runtime_error("open failed");
            }

            if (Length == 0) {
                this->Data = nullptr;
                return;
            }

            void* data = mmap(NULL, Length, PROT_READ, MAP_SHARED, fd, 0);

            if (data == MAP_FAILED) {
                close(fd);
                throw std::runtime_error("mmap failed");
            }
            this->Data = static_cast<char*>(data);
        }

        std::string_view MappedFile::View() const {
            return std::string_view(Data, Length);
        }

        MappedFile::~MappedFile() {
            if (Data != nullptr) {
                munmap(Data, Length);
            }
            close(fd);
        }
    }
}
