#pragma once
#include "types/driver/defs.hh"
#include "line_index.hh"
#include "mmap_file.hh"
#include "types/exceptions.hh"
#include "types/token/token.hh"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
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
        std::vector<exceptions::CompilerError> CompilerErrors; // TODO: reserve with herustics
        std::vector<exceptions::CompilerWarning> CompilerWarnings; // TODO: reserve with herustics

        private:
            Gctx_t() = default;

        friend Gctx;


        void Print() const;
    };


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



            template<class T>
            requires std::is_invocable_v<T, Gctx_t&>
            static auto ModifyGctx(T fn) {
                std::unique_lock lg(mut);
                return fn(gctx);
            }

            static void Print();

            static bool isVerbose();
            static bool isDebug();
            static Stage TillStage();
            static std::string GetSourceFilePath();
            static std::shared_ptr<base::LineIndex> GetLineIndex();

            // external stuff:
            static void AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr);
            static void AttachLineIndex(std::shared_ptr<base::LineIndex> LineIndexPtr);

            // exception stuff:
            static void GenerateWarning(types::TokenSourceLocation TokenLoc, base::LineCtx LineCtx_v, std::string Message);
            static void GenerateError(types::TokenSourceLocation TokenLoc, base::LineCtx LineCtx_v, std::string Message);
            static bool ErrorFound();
    };

}

}
