#pragma once
#include <utility>

namespace baka {
    namespace base {
        // Restore ambient context on normal returns and on lowering failures.
        template <class T> class ScopedValue {
            T& Target;
            T Saved;

        public:
            explicit ScopedValue(T& target) : Target(target), Saved(target) {}

            ScopedValue(const ScopedValue&) = delete;

            ~ScopedValue() {
                Target = std::move(Saved);
            }
        };
    }
} // namespace baka::base
