#include "gctx.hh"
#include <iostream>


bool baka::driver::Gctx::isVerbose() {
    return Gctx::GetGctxRO().VerboseMode;
}

bool baka::driver::Gctx::isDebug() {
    return Gctx::GetGctxRO().BuildType == BuildType::DEBUG;
}

baka::driver::Stage baka::driver::Gctx::TillStage() {
    return Gctx::GetGctxRO().TillStage;
}


void baka::driver::Gctx::Print() {
    std::lock_guard<std::mutex> lg(mut);
    const Gctx_t& gctx = Gctx::GetGctxRO();

    std::cout << "Gctx::VerboseMode: " << gctx.VerboseMode << std::endl;
    std::cout << "Gctx::BuildType: " << to_string_view(gctx.BuildType) << std::endl;
    std::cout << "Gctx::UptoStage: " << to_string_view(gctx.TillStage) << std::endl;
    std::cout << "Gctx::SourceFile: " << gctx.SourceFilePath << std::endl;
}
