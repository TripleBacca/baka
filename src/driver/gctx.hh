#pragma once
#include "types/driver/defs.hh"
#include "base/line_index.hh"
#include "mmap_file.hh"
#include "types/exceptions.hh"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace baka {
    namespace driver {
        class Gctx;

        struct Gctx_t {
            bool DebugMode = true;
            bool ShouldOptimize = true;
            bool VerboseMode = true;
            BuildType BuildType_v;
            std::string SourceFilePath;

            Stage TillStage = Stage::LEX; // TODO: change this default later

            // external stuff:
            std::shared_ptr<baka::base::MappedFile> MappedFilePtr;
            std::shared_ptr<baka::base::LineIndex> LineIndexPtr;

            // exceptions:
            std::vector<exceptions::CompilerLineError> CompilerLineErrors;
            std::vector<exceptions::CompilerLineError> CompilerLineWarnings;

            std::vector<exceptions::CompilerGlobalError> CompilerGlobalErrors;
            std::vector<exceptions::CompilerGlobalWarning> CompilerGlobalWarnings;

        private:
            Gctx_t() = default;

            friend Gctx;


            void Print() const;
        };

        constexpr size_t NO_LINE = 0;
        constexpr size_t NO_COL = 0;


        class Gctx {
            inline static std::shared_mutex mut;
            inline static Gctx_t gctx;

        public:
            static const Gctx_t& GetGctxRO() {
                return gctx;
            }

            static std::shared_lock<std::shared_mutex> GetROLock() {
                return std::shared_lock(mut);
            }


            template <class T>
                requires std::is_invocable_v<T, Gctx_t&>
            static auto ModifyGctx(T fn) {
                std::unique_lock lg(mut);
                return fn(gctx);
            }

            static void Print();

            static bool isVerbose();
            static bool isDebug();
            static Stage TillStage();
            static std::string_view GetSourceFilePath();
            static std::shared_ptr<base::LineIndex> GetLineIndex();

            // external stuff:
            static void AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr);
            static void AttachLineIndex(std::shared_ptr<base::LineIndex> LineIndexPtr);

            // exception stuff:
            static void GenerateLineWarning(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                        driver::Stage Stage);
            static void GenerateLineError(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                      driver::Stage Stage);

            static void GenerateGlobalWarning(std::string Message,
                                        driver::Stage Stage);
            static void GenerateGlobalError(std::string Message,
                                      driver::Stage Stage);

            static bool ErrorFound();

            static void ReserveDefectsVec(size_t ReserveSz) {
                ModifyGctx([&](Gctx_t& gctx) {
                    gctx.CompilerLineErrors.reserve(ReserveSz);
                    gctx.CompilerLineWarnings.reserve(ReserveSz);
                    gctx.CompilerGlobalErrors.reserve(ReserveSz);
                    gctx.CompilerGlobalWarnings.reserve(ReserveSz);
                });
            }
    };

}

}
