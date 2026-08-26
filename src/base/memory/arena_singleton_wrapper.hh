#pragma once


namespace baka {
namespace base {


// enumerate all kinds of different arenas we need
enum class AllocArenaTag {
    SL_STRING, // one for string literals
    AST_NODE,
    UTIL_NODE
};


template<class Arena, AllocArenaTag TAG_>
class SingletonArena : public Arena {
    private:
        SingletonArena() = default;

    public:
        static SingletonArena& getInstance() {
            static SingletonArena instance;
            return instance;
        }
};

}
}
