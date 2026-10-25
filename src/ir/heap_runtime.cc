#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>

extern "C" void* __baka_allocate(uint64_t bytes, uint64_t alignment) noexcept {
    if (!alignment || (alignment & (alignment - 1)) ||
        bytes > std::numeric_limits<size_t>::max()) {
        return nullptr;
    }
    auto size = static_cast<size_t>(bytes ? bytes : 1);
    if (alignment <= alignof(std::max_align_t)) {
        return std::malloc(size);
    }
    if (alignment > std::numeric_limits<size_t>::max() ||
        size > std::numeric_limits<size_t>::max() - static_cast<size_t>(alignment - 1)) {
        return nullptr;
    }
    auto alignedSize = (size + static_cast<size_t>(alignment - 1)) &
                       ~static_cast<size_t>(alignment - 1);
    return std::aligned_alloc(static_cast<size_t>(alignment), alignedSize);
}

extern "C" void __baka_deallocate(void* pointer) noexcept {
    std::free(pointer);
}
