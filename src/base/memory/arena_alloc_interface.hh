#pragma once

#include <cstddef>
#include <type_traits>

#include "concepts.hh"

namespace baka {
namespace base {

    // singleton area shoudl have getInstance() static function that returns an instance
    // the instance should have:
    //
    // template<class T, class ...Args>
    // requires (sizeof(T) <= SZ)
    // std::span<T> AllocArray(size_t N ,Args&& ...args)
    //
    // and
    //
    // template<class T, class ...Args>
    // requires (sizeof(T) <= SZ)
    // T* Alloc(Args&&... args)
    //


    // commented out things are deprecated
    // https://en.cppreference.com/cpp/memory/allocator
    template<class T, IsSingleton SingletonArenaT>
    requires std::is_trivially_constructible_v<T> // need this cuz this should technically return an unintialized buffer (but we are returning an intialized one)
    class ArenaAllocatorWrapper {
    public:
        using value_type = T;
        // using pointer = T*;
        // using const_pointer = const T*;
        // using reference = T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;

        constexpr ArenaAllocatorWrapper() noexcept = default;

        template<class U>
        constexpr ArenaAllocatorWrapper(const ArenaAllocatorWrapper<U, SingletonArenaT>&) noexcept {}

        T* allocate(std::size_t n) {
            if(n == 0) {
                return nullptr;
            }
            auto& arena = SingletonArenaT::getInstance();
            return arena.template AllocArray<T>(n).data();
        }

        void deallocate(T*, std::size_t) noexcept {
            // noop
        }

        template<class U>
        bool operator==(const ArenaAllocatorWrapper<U, SingletonArenaT>&) const noexcept {
            return true;
        }
    };
}
}
