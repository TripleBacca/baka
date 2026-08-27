#pragma once
#include "memory/arena.hh"
#include "memory/arena_singleton_wrapper.hh"


namespace baka {
namespace base {

using ASTNodeArena = SingletonArena<Arena<1024>, AllocArenaTag::AST_NODE>;
using UtilArena = SingletonArena<Arena<1024>, AllocArenaTag::UTIL_NODE >;

}
}
