#pragma once
#include <cstdio>
#include <filesystem>
#include <syscall.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace baka {
namespace base {

        class MappedFile {
            size_t Length;
            int fd;
            char* Data;
            public:
            MappedFile(std::filesystem::path path);
            std::string_view View() const;
            ~MappedFile();

            // i dont care about these:
            MappedFile(const MappedFile& other) = delete;
            MappedFile(const MappedFile&& other) = delete;
            MappedFile& operator=(const MappedFile& other) = delete;
            MappedFile& operator=(const MappedFile&& other) = delete;
        };
}
}
