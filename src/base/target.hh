#pragma once

namespace baka {
    namespace target {
        // One target contract for semantic layout, arithmetic and IR import.
        inline constexpr unsigned ByteBits = 8, CharBits = 8, ShortBits = 16, IntBits = 32, LongBits = 64,
                                  FloatBits = 32, DoubleBits = 64, PointerBits = 64;
        inline constexpr unsigned PointerBytes = PointerBits / ByteBits;
    }
} // namespace baka::target
