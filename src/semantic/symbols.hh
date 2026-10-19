#pragma once

#include "types.hh"
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

namespace baka {
    namespace semantic {
        enum class ScopeKind { Global, Block, Function, Prototype, Aggregate, For };
        enum class SymbolKind { Object, Function, Alias, Type, Enumerator, Parameter, BuiltinFunction };
        enum class Linkage { None, Internal, External };

        struct Diagnostic {
            std::string code, message;
            const void* source = nullptr;
            const void* previous = nullptr;
        };

        struct Symbol {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            ScopeId scope;
            Linkage linkage = Linkage::None;
            bool defined = false;
            bool isStatic = false;
            const void* source = nullptr;
            std::optional<int64_t> constant;
            // Stored for B/C. Declaration order is symbol ID order.
            std::vector<const void*> declarations;
            std::vector<const void*> defaultArguments{};
        };

        struct Scope {
            ScopeId parent;
            ScopeKind kind = ScopeKind::Block;
            std::unordered_map<std::string, std::vector<SymbolId>> names;
        };

        struct Declaration {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            bool definition = false;
            bool isStatic = false;
            Linkage linkage = Linkage::None;
            const void* source = nullptr;
        };

        class Symbols {
            Types& TypeSystem;
            std::deque<Symbol> SymbolEntries{Symbol{}};
            std::deque<Scope> ScopeArena{Scope{}};
            ScopeId CurrentScope;
            std::vector<Diagnostic> Diagnostics;
            uint32_t GlobalVisibility = std::numeric_limits<uint32_t>::max();

            static bool IsFunctionKind(SymbolKind Kind);
            bool AreCompatible(TypeId ExistingType, TypeId CandidateType) const;

        public:
            explicit Symbols(Types& TypeSystem);

            ScopeId currentScope() const;
            const Scope& scope(ScopeId id) const;
            const Symbol& get(SymbolId id) const;
            const std::vector<Diagnostic>& diagnostics() const;

            void diagnose(std::string code, std::string message, const void* source = nullptr,
                          const void* previous = nullptr);

            ScopeId enter(ScopeKind kind);
            bool leave();
            void selectScope(ScopeId id) { CurrentScope = id; }
            uint32_t symbolCount() const { return static_cast<uint32_t>(SymbolEntries.size() - 1); }
            void setGlobalVisibility(uint32_t limit) { GlobalVisibility = limit; }
            bool defaults(SymbolId id, const std::vector<const void*>& arguments);

            std::vector<SymbolId> lookupCurrent(const std::string& name) const;
            std::vector<SymbolId> lookup(const std::string& name) const;

            SymbolId bind(const std::string& name, const void* source = nullptr);
            SymbolId declare(Declaration NewDeclaration);
            bool completeObject(SymbolId id, TypeId completed);
            bool setConstant(SymbolId id, int64_t value);

            // C supplies an ancestor scope as the destination. Results are in cleanup order.
            std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const;

            TypeId lookupType(const std::string& name, const void* source = nullptr);

            void installTypes();
            void installLibrary();
        };
    }
}
