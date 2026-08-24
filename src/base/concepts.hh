#pragma once
#include <concepts>
#include <type_traits>


namespace baka {

namespace base {

template<class T>
concept HasPublicConstructor = std::is_default_constructible_v<T>;


template<class T>
concept IsSingleton = !HasPublicConstructor<T> && requires() {
    {T::getInstance()} -> std::same_as<T&>;
};

}
}
