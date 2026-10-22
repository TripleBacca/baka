#pragma once
#include "ir.hh"

namespace baka {
    namespace ir {
        // Shared typed constant arithmetic for static data and optional optimization.
        // No frontend constant-value cache is consulted. A missing result means the
        // operation must remain runtime IR (or is not a representable static value).
        std::optional<Operand> FoldConstant(const Module& module, const Instruction& instruction);
    }
}
