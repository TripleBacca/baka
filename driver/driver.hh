#pragma once


#include "defs.hh"
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
        BuildType BuildType;
        std::string SourceFilePath;

        Stage TillStage = Stage::LEX; // TODO: change this default later

        private:
            Gctx_t();

        friend Gctx;


        void Print() const;
    };


    class Gctx {
        static std::mutex mut;
        static Gctx_t gctx;

        public:
            static const Gctx_t& GetGctxRO() {
                std::lock_guard<std::mutex> lg(mut);
                return gctx;
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
    };

    void run(int argc, char* argv[]);
}

}
