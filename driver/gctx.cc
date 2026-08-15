#include "gctx.hh"
#include "line_index.hh"
#include <iostream>
#include <memory>
#include <vector>

std::string baka::driver::Gctx::GetSourceFilePath() {
    return Gctx::GetGctxRO().SourceFilePath;
}

bool baka::driver::Gctx::isVerbose() {
    return Gctx::GetGctxRO().VerboseMode;
}

bool baka::driver::Gctx::isDebug() {
    return Gctx::GetGctxRO().BuildType_v == BuildType::DEBUG;
}

baka::driver::Stage baka::driver::Gctx::TillStage() {
    return Gctx::GetGctxRO().TillStage;
}

std::shared_ptr<baka::base::LineIndex> baka::driver::Gctx::GetLineIndex() {
    return Gctx::GetGctxRO().LineIndexPtr;
}

void baka::driver::Gctx::Print() {
    auto gctx = Gctx::GetGctxRO();

    std::cout << "Gctx::VerboseMode: " << gctx.VerboseMode << std::endl;
    std::cout << "Gctx::BuildType: " << to_string_view(gctx.BuildType_v) << std::endl;
    std::cout << "Gctx::UptoStage: " << to_string_view(gctx.TillStage) << std::endl;
    std::cout << "Gctx::SourceFile: " << gctx.SourceFilePath << std::endl;
}


void baka::driver::Gctx::AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.MappedFilePtr = std::move(MappedFilePtr);
    });
}

void baka::driver::Gctx::AttachLineIndex(std::shared_ptr<base::LineIndex> LineIndexPtr) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.LineIndexPtr = std::move(LineIndexPtr);
    });
}


void baka::driver::Gctx::GenerateError(types::TokenSourceLocation TokenLoc, base::LineCtx LineCtx_v, std::string Message) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.CompilerErrors.emplace_back(TokenLoc, LineCtx_v, Message);
    });
}

void baka::driver::Gctx::GenerateWarning(types::TokenSourceLocation TokenLoc, base::LineCtx LineCtx_v, std::string Message) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.CompilerWarnings.emplace_back(TokenLoc, LineCtx_v, Message);
    });
}

bool baka::driver::Gctx::ErrorFound() {
    return !Gctx::GetGctxRO().CompilerErrors.empty();
}
