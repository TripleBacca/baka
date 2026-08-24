#pragma once
#include "memory/arena.hh"
#include "memory/arena_alloc_interface.hh"
#include "memory/arena_singleton_wrapper.hh"
#include <string>


namespace baka {
namespace base {

// string for storing string literals
using SLString = std::basic_string<char,
                    std::char_traits<char>,
                    ArenaAllocatorWrapper<
                        char,
                        SingletonArena<
                            Arena<1024>,
                            AllocArenaTag::SL_STRING
                        >
                    >
                >;


}
}
