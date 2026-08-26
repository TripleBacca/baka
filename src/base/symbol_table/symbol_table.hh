#pragma once

#include "memory/arena_alloc_interface.hh"
#include "memory/custom_arenas.hh"
#include <optional>
#include <queue>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <ext/pb_ds/assoc_container.hpp>
#include <vector>
using namespace __gnu_pbds;


namespace baka {
namespace base {

struct SymbolTableUpdate {
    std::string_view Symbol;
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class SymbolTable {

    // TODO: can cache hash value
    using Map = gp_hash_table<std::string_view,
        SymbolTableEntry,
        std::hash<std::string_view>,
        std::equal_to<std::string_view>,
        ArenaAllocatorWrapper<
            char,
            SingletonArena<
                Arena<1024>,
                AllocArenaTag::UTIL_NODE
            >
        >
    >;

    std::stack<Map> Scopes;

public:
    SymbolTable() = default;

    void EnterNewScope() {
        Scopes.push(Map());
    }

    void ExitScope() {
        Scopes.pop();
    }

    std::optional<SymbolTableEntry> Lookup(std::string_view IdentifierName) {
        for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
            if (it->contains(IdentifierName)) {
                return it->at(IdentifierName);
            }
        }

        return std::nullopt;
    }

    bool AddEntry(std::string_view Symbol, SymbolTableEntry Entry) {
        if(Scopes.top().contain(Symbol)) {
            assert(false);
            return false;
        }
        Scopes.top().insert({Symbol, std::move(Entry)});
        return true;
    }
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class ParserSymbolTable : public SymbolTable<SymbolTableEntry, Alloc> {

    public:
        ParserSymbolTable() {
            // prefill
            this->EnterNewScope();


        }

};

}
}
