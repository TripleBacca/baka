#pragma once
#include "ir/ir.hh"
#include <iosfwd>
#include <vector>

namespace baka {
    namespace codegen {
        struct Options {
            // Enables block-local IR optimization before selection and the MIPS
            // peephole pass after register assignment. Selection itself always
            // uses the next-use/descriptor allocator.
            bool optimize = true;
        };

        // Translates a verified module to one GNU assembler file for MIPS64r2,
        // n64 ABI, little-endian, non-PIC and statically linked against libc.
        // Returns false (with diagnostics) for IR the backend cannot represent.
        bool Generate(const ir::Module& module, const Options& options, std::ostream& output,
                      std::vector<ir::Diagnostic>& diagnostics);
    }
}
