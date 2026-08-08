#pragma once


#include <mutex>
namespace baka {

namespace driver {

    struct Gctx_t {
        std::mutex _mut; // do we even need this?
        bool DebugMode;
    };


    inline Gctx_t Gctx;
    void run(int argc, char* argv[]);
}

}
