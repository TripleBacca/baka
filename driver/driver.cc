#include "driver.hh"
#include "../types/exceptions.hh"
#include "defs.hh"
#include <iostream>

// cli args cases:
// --lex
// --parse
// --codegen
//
// --build_type
// --verbose
// passed by tf: '{self.cc} {options} {source_file}'

void baka::driver::run(int argc, char **argv) {
    if(argc == 1) {
        throw baka::exceptions::DriverError("no input file");
    }
    size_t idx = 1;

    // TODO: maybe change these defaults later
    Stage UptoStage = Stage::LEX;
    BuildType BuildType = BuildType::DEBUG;
    std::string SourceFile;
    bool VerboseMode = true;
    bool SourceFound = false;

    while(idx < argc) {
        std::string_view curr = argv[idx];
        if(curr == "--lex") {
            UptoStage = Stage::LEX;
        } else if (curr == "--parse") {
            UptoStage = Stage::PARSE;
        } else if (curr == "--codegen") {
            UptoStage = Stage::CODEGEN;
        } else if (curr == "--built_type") {
            idx++;
            if(idx < argc) {
                std::string_view type = argv[idx];

                if (type == "release") {
                    BuildType = BuildType::RELEASE;
                } else if (type == "debug") {
                    BuildType = BuildType::DEBUG;
                } else {
                    throw baka::exceptions::DriverError("invalid built type for --build_type");
                }
            } else {
                throw baka::exceptions::DriverError("no built type specified for --build_type");
            }
        } else if (curr == "--verbose") {
            VerboseMode = true;
        } else {
            if(idx == argc-1) {
                // source file
                SourceFile = std::string(argv[idx]);
                SourceFound = true;
            } else {
                throw baka::exceptions::DriverError("invalid argument");
            }
        }
        idx++;
    }


    if(!SourceFound) {
        throw baka::exceptions::DriverError("no source file specified");
    }



    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.SourceFilePath = std::move(SourceFile);
        gctx.BuildType = BuildType;
        gctx.TillStage = UptoStage;
        gctx.VerboseMode = VerboseMode;

    });

    if(Gctx::isVerbose()) {
        Gctx::Print();
    }

    // TODO: actually call them
}

bool baka::driver::Gctx::isVerbose() {
    return Gctx::GetGctxRO().VerboseMode;
}

bool baka::driver::Gctx::isDebug() {
    return Gctx::GetGctxRO().BuildType == BuildType::DEBUG;
}


void baka::driver::Gctx::Print() {
    std::lock_guard<std::mutex> lg(mut);
    const Gctx_t& gctx = Gctx::GetGctxRO();

    std::cout << "Gctx::VerboseMode: " << gctx.VerboseMode << std::endl;
    std::cout << "Gctx::BuildType: " << to_string_view(gctx.BuildType) << std::endl;
    std::cout << "Gctx::UptoStage: " << to_string_view(gctx.TillStage) << std::endl;
    std::cout << "Gctx::SourceFile: " << gctx.SourceFilePath << std::endl;
}
