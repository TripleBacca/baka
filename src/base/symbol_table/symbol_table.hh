#pragma once

#include "memory/custom_arenas.hh"
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace baka {
namespace base {

struct SymbolTableUpdate {
    std::string_view Symbol;
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class SymbolTable {

    // Node-based storage keeps entry addresses stable across rehashes.
    using Map = std::unordered_map<std::string_view, SymbolTableEntry>;

    std::vector<Map> Scopes;

public:
    SymbolTable() = default;

    void EnterNewScope() {
        Scopes.emplace_back();
    }

    void ExitScope() {
        if (!Scopes.empty()) Scopes.pop_back();
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
        if (Scopes.empty()) EnterNewScope();
        if (Scopes.back().find(Symbol) != Scopes.back().end()) {
            return false;
        }
        Scopes.back().insert({Symbol, std::move(Entry)});
        return true;
    }

    bool RegisterOrReplaceType(std::string_view Symbol, SymbolTableEntry Entry) {
        if (Scopes.empty()) EnterNewScope();
        // true if symbol is new
        auto it = Scopes.back().find(Symbol);
        if (it != Scopes.back().end()) {
            it->second = std::move(Entry);
            return false;
        }
        Scopes.back().insert({Symbol, std::move(Entry)});
        return true;
    }

    std::optional<SymbolTableEntry> LookupCurrent(std::string_view IdentifierName) const {
        if (Scopes.empty()) {
            return std::nullopt;
        }
        auto it = Scopes.back().find(IdentifierName);
        if (it == Scopes.back().end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // Stable until the containing scope is exited.
    SymbolTableEntry* GetEntry(std::string_view Symbol) {
        for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
            auto found = it->find(Symbol);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
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
            // this->AddEntry("unsigned", SymbolTableEntry()); //TODO NOT IMPLEMENTED SOME GET ON THIS RNRNRN
        }

};

}
}
