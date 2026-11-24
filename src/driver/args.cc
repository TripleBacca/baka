#include "types/driver/defs.hh"
#include "gctx.hh"
#include <cstddef>
#include <filesystem>
#include "types/exceptions.hh"


// cli args cases:
// --lex
// --parse
// --codegen
// --optimize / --no-optimize
//
// --build_type
// --verbose
// passed by tf: '{self.cc} {options} {source_file}'


namespace baka {
    namespace driver {
        // Sets the requested stage, build mode, optimization, verbosity, and input in the driver context.
        // Returns the output path and defaults to linking an executable. Throws DriverError on invalid arguments.
        std::string ParseArgs(int argc, char** argv) {
            if (argc == 1) {
                throw baka::exceptions::DriverError("no input file");
            }
            std::size_t idx = 1;
            Stage UptoStage = Stage::LINK;
            baka::driver::BuildType BuildType = BuildType::DEBUG;
            std::string SourceFile;
            std::string OutputFile;
            bool VerboseMode = false;
            std::optional<bool> ShouldOptimize;

            while (idx < argc) {
                std::string_view curr = argv[idx];

                if (curr == "--lex") {
                    UptoStage = Stage::LEX;
                }
                else if (curr == "--parse") {
                    UptoStage = Stage::PARSE;
                }
                else if (curr == "--semantic") {
                    UptoStage = Stage::SEMANTIC;
                }
                else if (curr == "--ir") {
                    UptoStage = Stage::IR;
                }
                else if (curr == "--codegen") {
                    UptoStage = Stage::CODEGEN;
                }
                else if (curr == "--assemble") {
                    UptoStage = Stage::ASSEMBLE;
                }
                else if (curr == "--link") {
                    UptoStage = Stage::LINK;
                }
                else if (curr == "-o") {
                    if (++idx == argc || !argv[idx][0] || std::string_view(argv[idx]).starts_with('-')) {
                        throw baka::exceptions::DriverError("-o requires an output path");
                    }
                    OutputFile = argv[idx];
                }
                else if (curr == "--build_type") {
                    idx++;
                    if (idx < argc) {
                        std::string_view type = argv[idx];

                        if (type == "release") {
                            BuildType = BuildType::RELEASE;
                        }
                        else if (type == "debug") {
                            BuildType = BuildType::DEBUG;
                        }
                        else {
                            throw baka::exceptions::DriverError("invalid built type for --build_type");
                        }
                    }
                    else {
                        throw baka::exceptions::DriverError("no built type specified for --build_type");
                    }
                }
                else if (curr == "--optimize") {
                    ShouldOptimize = true;
                }
                else if (curr == "--no-optimize") {
                    ShouldOptimize = false;
                }
                else if (curr == "--verbose") {
                    VerboseMode = true;
                }
                else if (curr.starts_with('-')) {
                    throw baka::exceptions::DriverError("unknown option: " + std::string(curr));
                }
                else if (SourceFile.empty()) {
                    SourceFile = curr;
                }
                else {
                    throw baka::exceptions::DriverError("expected one source file");
                }
                idx++;
            }

            if (SourceFile.empty()) {
                throw baka::exceptions::DriverError("no source file specified");
            }
            if (UptoStage < Stage::ASSEMBLE && !OutputFile.empty()) {
                throw baka::exceptions::DriverError("-o is for --assemble or --link; redirect stdout for text output");
            }
            if (OutputFile.empty()) {
                OutputFile = UptoStage == Stage::ASSEMBLE
                                 ? std::filesystem::path(SourceFile).stem().string() + ".o"
                                 : "a.out";
            }


            Gctx::ModifyGctx([&](Gctx_t& gctx) {
                gctx.SourceFilePath = std::move(SourceFile);
                gctx.BuildType_v = BuildType;
                gctx.TillStage = UptoStage;
                gctx.VerboseMode = VerboseMode;
                gctx.ShouldOptimize = ShouldOptimize;
            });

            if (Gctx::isVerbose()) {
                Gctx::Print();
            }
            return OutputFile;
        }
    }
}
