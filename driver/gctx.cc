#include "gctx.hh"
#include "line_index.hh"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::string baka::driver::Gctx::GetSourceFilePath() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().SourceFilePath;
}

bool baka::driver::Gctx::isVerbose() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().VerboseMode;
}

bool baka::driver::Gctx::isDebug() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().BuildType_v == BuildType::DEBUG;
}

baka::driver::Stage baka::driver::Gctx::TillStage() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().TillStage;
}

std::shared_ptr<baka::base::LineIndex> baka::driver::Gctx::GetLineIndex() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().LineIndexPtr;
}

void baka::driver::Gctx::Print() {
    auto lg = Gctx::GetROLock();
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
    auto lg = Gctx::GetROLock();
    return !Gctx::GetGctxRO().CompilerErrors.empty();
}
