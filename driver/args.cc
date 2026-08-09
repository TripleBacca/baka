
#include "defs.hh"
#include "gctx.hh"
#include <cstddef>
#include "../types/exceptions.hh"


// cli args cases:
// --lex
// --parse
// --codegen
//
// --build_type
// --verbose
// passed by tf: '{self.cc} {options} {source_file}'


namespace baka {
namespace driver {


void ParseArgs(int argc, char **argv) {
    if(argc == 1) {
        throw baka::exceptions::DriverError("no input file");
    }
    std::size_t idx = 1;
    // TODO: maybe change these defaults later
    Stage UptoStage = Stage::LEX;
    baka::driver::BuildType BuildType = BuildType::DEBUG;
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
        } else if (curr == "--build_type") {
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
        gctx.BuildType_v = BuildType;
        gctx.TillStage = UptoStage;
        gctx.VerboseMode = VerboseMode;
    });

    if(Gctx::isVerbose()) {
        Gctx::Print();
    }
}

}
}
