#pragma once
#include "defs.hh"
#include "mmap_file.hh"
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

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

        std::shared_ptr<baka::base::MappedFile> MappedFilePtr;

        private:
            Gctx_t() = default;

        friend Gctx;


        void Print() const;
    };


    class Gctx {
        inline static std::mutex mut;
        inline static Gctx_t gctx;

        public:
            static Gctx_t GetGctxRO() {
                std::lock_guard<std::mutex> lg(mut);

                Gctx_t gctx_cpy = gctx;
                return gctx_cpy;
            }

            template<class T>
            requires std::is_invocable_v<T, Gctx_t&>
            static auto ModifyGctx(T fn) {
                std::lock_guard<std::mutex> lg(mut);
                return fn(gctx);
            }

            static void Print();

            static bool isVerbose();
            static bool isDebug();
            static Stage TillStage();
            static std::string GetSourceFilePath();
            static void AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr);
    };

}

}
