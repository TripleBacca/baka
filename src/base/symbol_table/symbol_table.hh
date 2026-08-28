#pragma once

#include "memory/custom_arenas.hh"
#include <optional>
#include <string_view>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/hash_policy.hpp>
#include <vector>
#include <cassert>

using namespace __gnu_pbds;


namespace baka {
namespace base {

struct SymbolTableUpdate {
    std::string_view Symbol;
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class SymbolTable {

    // TODO: can cache hash value
    // gp_hash_table template params:
    //   Key, Mapped, Hash_Fn, Eq_Fn,
    //   Comb_Probe_Fn, Probe_Fn, Resize_Policy, Store_Hash, _Alloc
    using Map = gp_hash_table<
        std::string_view,
        SymbolTableEntry,
        std::hash<std::string_view>
    >;

    std::vector<Map> Scopes;

public:
    SymbolTable() = default;

    void EnterNewScope() {
        Scopes.emplace_back();
    }

    void ExitScope() {
        Scopes.pop_back();
    }

    std::optional<SymbolTableEntry> Lookup(std::string_view IdentifierName) {
        for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
            auto found = it->find(IdentifierName);
            if (found != it->end()) {
                return found->second;
            }
        }

        return std::nullopt;
    }

    bool AddEntry(std::string_view Symbol, SymbolTableEntry Entry) {
        if (Scopes.back().find(Symbol) != Scopes.back().end()) {
            assert(false);
            return false;
        }
        Scopes.back().insert({Symbol, std::move(Entry)});
        return true;
    }
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class ParserSymbolTable : public SymbolTable<SymbolTableEntry, Alloc> {

    public:
        ParserSymbolTable() {
            this->EnterNewScope();
            this->AddEntry("int", SymbolTableEntry()); // has unsigned
            this->AddEntry("float", SymbolTableEntry());
            this->AddEntry("double", SymbolTableEntry());
            this->AddEntry("char", SymbolTableEntry()); // has unsigned
            this->AddEntry("void", SymbolTableEntry());
            this->AddEntry("long", SymbolTableEntry());  // has unsigned
            this->AddEntry("short", SymbolTableEntry());  // has unsigned
            this->AddEntry("bool", SymbolTableEntry());
            // this->AddEntry("signed", SymbolTableEntry());
            // this->AddEntry("unsigned", SymbolTableEntry()); //TODO NOT IMPLEMENTED SOME FUCKER GET ON THIS SHIT RNRNRN
        }

};

}
}
