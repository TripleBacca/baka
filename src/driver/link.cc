#include "link.hh"
#include "types/exceptions.hh"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>

extern char** environ;

namespace baka::driver {
    namespace {
        namespace fs = std::filesystem;
        using exceptions::DriverError;

        // Stage the result next to the destination so rename can publish it atomically.
        // Docker reads the input on stdin and writes the artifact on stdout, so it
        // never needs access to host directories or file names.
        class StagingDirectory {
        public:
            explicit StagingDirectory(const fs::path& output) {
                auto pattern = (output.parent_path() / ".baka-link-XXXXXX").string();
                if (!mkdtemp(pattern.data())) {
                    throw DriverError("cannot stage '" + output.string() + "': " +
                                      std::error_code(errno, std::generic_category()).message());
                }
                path = std::move(pattern);
            }

            ~StagingDirectory() {
                std::error_code error;
                fs::remove_all(path, error);
            }

            fs::path path;
        };

        // Runs the requested MIPS assembly or link step in Docker. The child reads the
        // input from stdin and writes the artifact to the staging file, leaving stderr
        // available for toolchain diagnostics.
        void RunToolchain(const fs::path& input, const fs::path& staged, std::string mode) {
            std::string command[] = {
                "docker", "run", "--rm", "-i", "--pull=never", "baka-mips64el", "sh", "-ec",
                R"(if [ "$1" = link-object ]; then
                       cat > /tmp/program.o
                   else
                       cat > /tmp/program.s
                       mips64el-linux-gnuabi64-gcc -march=mips64r2 -mabi=64 -c /tmp/program.s -o /tmp/program.o
                   fi
                   if [ "$1" = assemble ]; then
                       cat /tmp/program.o
                   else
                       mips64el-linux-gnuabi64-gcc -march=mips64r2 -mabi=64 -static -no-pie /tmp/program.o -o /tmp/program
                       cat /tmp/program
                   fi)",
                "sh", mode,
            };
            char* args[sizeof(command) / sizeof(command[0]) + 1]{};
            for (size_t i = 0; i < sizeof(command) / sizeof(command[0]); ++i) {
                args[i] = command[i].data();
            }

            posix_spawn_file_actions_t actions;
            int error = posix_spawn_file_actions_init(&actions);
            if (error) {
                throw DriverError("cannot initialize MIPS toolchain process: " +
                                  std::error_code(error, std::generic_category()).message());
            }
            error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, input.c_str(), O_RDONLY, 0);
            if (!error) {
                error = posix_spawn_file_actions_addopen(
                    &actions, STDOUT_FILENO, staged.c_str(), O_WRONLY | O_CREAT | O_EXCL,
                    mode == "assemble" ? 0666 : 0777);
            }
            pid_t child;
            if (!error) {
                error = posix_spawnp(&child, args[0], &actions, nullptr, args, environ);
            }
            posix_spawn_file_actions_destroy(&actions);
            if (error) {
                throw DriverError("cannot start Docker for the MIPS toolchain: " +
                                  std::error_code(error, std::generic_category()).message());
            }

            int status;
            while (waitpid(child, &status, 0) < 0) {
                if (errno != EINTR) {
                    throw DriverError("cannot wait for the MIPS toolchain: " +
                                      std::error_code(errno, std::generic_category()).message());
                }
            }
            if (!WIFEXITED(status)) {
                throw DriverError("the MIPS toolchain was interrupted");
            }
            const int exitStatus = WEXITSTATUS(status);
            if (exitStatus != 0) {
                std::string message = "MIPS toolchain exited with status " + std::to_string(exitStatus);
                if (exitStatus == 125) {
                    message += ". Check Docker access and build the image with 'make mips-image'";
                }
                throw DriverError(std::move(message));
            }
        }

        void ProduceFile(const fs::path& input, const fs::path& output, const StagingDirectory& staging,
                         const char* mode) {
            const auto staged = staging.path / "output";
            RunToolchain(input, staged, mode);
            std::error_code error;
            fs::rename(staged, output, error);
            if (error) {
                throw DriverError("cannot write '" + output.string() + "': " + error.message());
            }
        }
    }

    void BuildMipsFromAssembly(std::string_view assembly, const fs::path& output, Stage stage) {
        const StagingDirectory staging(output);
        const auto input = staging.path / "program.s";
        std::ofstream file(input, std::ios::binary);
        file << assembly;
        file.close();
        if (!file) {
            throw DriverError("cannot stage MIPS assembly for '" + output.string() + "'");
        }
        ProduceFile(input, output, staging, stage == Stage::ASSEMBLE ? "assemble" : "assemble-and-link");
    }

    void BuildMipsFromFile(const fs::path& input, const fs::path& output, Stage stage) {
        const StagingDirectory staging(output);
        const char* mode = "assemble-and-link";
        if (stage == Stage::ASSEMBLE) {
            mode = "assemble";
        } else if (input.extension() == ".o") {
            mode = "link-object";
        }
        ProduceFile(input, output, staging, mode);
    }
}
