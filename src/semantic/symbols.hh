#pragma once

#include "types.hh"
#include <functional>
#include <unordered_map>

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

            static bool IsFunctionKind(SymbolKind Kind) {
                return Kind == SymbolKind::Function || Kind == SymbolKind::BuiltinFunction;
            }

            bool AreCompatible(TypeId ExistingType, TypeId CandidateType) const {
                if (ExistingType == CandidateType) {
                    return true;
                }
                const auto& LeftType = TypeSystem.get(ExistingType);
                const auto& RightType = TypeSystem.get(CandidateType);
                if (LeftType.kind != RightType.kind) {
                    return false;
                }
                return LeftType.kind == TypeKind::Array && AreCompatible(LeftType.element, RightType.element) &&
                       (LeftType.bound.kind == BoundKind::Unspecified ||
                        RightType.bound.kind == BoundKind::Unspecified || LeftType.bound == RightType.bound);
            }

        public:
            explicit Symbols(Types& TypeSystem) : TypeSystem(TypeSystem) {
                enter(ScopeKind::Global);
            }

            ScopeId currentScope() const {
                return CurrentScope;
            }

            const Scope& scope(ScopeId id) const {
                return ScopeArena.at(id.value);
            }

            const Symbol& get(SymbolId id) const {
                return SymbolEntries.at(id.value);
            }

            const auto& diagnostics() const {
                return Diagnostics;
            }

            void diagnose(std::string code, std::string message, const void* source = nullptr,
                          const void* previous = nullptr) {
                Diagnostics.push_back({std::move(code), std::move(message), source, previous});
            }

            ScopeId enter(ScopeKind kind) {
                ScopeArena.push_back({CurrentScope, kind, {}});
                CurrentScope = {static_cast<uint32_t>(ScopeArena.size() - 1)};
                return CurrentScope;
            }

            bool leave() {
                if (!CurrentScope.value || !scope(CurrentScope).parent.value) {
                    return false;
                }
                CurrentScope = scope(CurrentScope).parent;
                return true;
            }

            std::vector<SymbolId> lookupCurrent(const std::string& name) const {
                auto it = scope(CurrentScope).names.find(name);
                return it == scope(CurrentScope).names.end() ? std::vector<SymbolId>{} : it->second;
            }

            std::vector<SymbolId> lookup(const std::string& name) const {
                for (auto ScopeToVisit = CurrentScope; ScopeToVisit.value; ScopeToVisit = scope(ScopeToVisit).parent) {
                    auto it = scope(ScopeToVisit).names.find(name);
                    if (it != scope(ScopeToVisit).names.end()) {
                        return it->second;
                    }
                }
                return {};
            }

            SymbolId bind(const std::string& name, const void* source = nullptr) {
                auto found = lookup(name);
                if (found.empty()) {
                    diagnose("undeclared", "undeclared name '" + name + "'", source);
                    return {};
                }
                if (found.size() != 1) {
                    diagnose("overload", "overload set requires call resolution", source);
                    return {};
                }
                auto id = found.front();
                if (get(id).kind == SymbolKind::Alias || get(id).kind == SymbolKind::Type) {
                    diagnose("value", "type name used as a value", source);
                    return {};
                }
                return id;
            }

            SymbolId declare(Declaration NewDeclaration) {
                if (!NewDeclaration.type.value) {
                    diagnose("type", "invalid declaration type", NewDeclaration.source);
                    return {};
                }
                if (IsFunctionKind(NewDeclaration.kind) &&
                    TypeSystem.get(NewDeclaration.type).kind != TypeKind::Function) {
                    diagnose("signature", "function symbol requires a function type", NewDeclaration.source);
                    return {};
                }
                auto found = lookupCurrent(NewDeclaration.name);
                for (auto id : found) {
                    auto& old = SymbolEntries.at(id.value);
                    if (IsFunctionKind(old.kind) && IsFunctionKind(NewDeclaration.kind)) {
                        const auto& ExistingType = TypeSystem.get(old.type);
                        const auto& CandidateType = TypeSystem.get(NewDeclaration.type);
                        if (ExistingType.parameters != CandidateType.parameters ||
                            ExistingType.variadic != CandidateType.variadic) {
                            if (old.kind == SymbolKind::BuiltinFunction ||
                                NewDeclaration.kind == SymbolKind::BuiltinFunction) {
                                diagnose("builtin",
                                         "incompatible redeclaration of builtin '" + NewDeclaration.name + "'",
                                         NewDeclaration.source, old.source);
                                return {};
                            }
                            continue;
                        }
                        // Return types cannot distinguish overloads.
                    }
                    bool legal = old.kind == NewDeclaration.kind ||
                                 (IsFunctionKind(old.kind) && IsFunctionKind(NewDeclaration.kind));
                    legal = legal && AreCompatible(old.type, NewDeclaration.type) &&
                            old.linkage == NewDeclaration.linkage && old.isStatic == NewDeclaration.isStatic;
                    bool repeatable = IsFunctionKind(NewDeclaration.kind) || NewDeclaration.kind == SymbolKind::Alias ||
                                      NewDeclaration.kind == SymbolKind::Type;
                    repeatable = repeatable || (NewDeclaration.kind == SymbolKind::Object &&
                                                scope(CurrentScope).kind == ScopeKind::Global);
                    if (!legal || !repeatable || (old.defined && NewDeclaration.definition)) {
                        diagnose("redeclaration",
                                 "conflicting or repeated declaration of '" + NewDeclaration.name + "'",
                                 NewDeclaration.source, old.source);
                        return {};
                    }
                    old.defined = old.defined || NewDeclaration.definition;
                    if (TypeSystem.get(old.type).kind == TypeKind::Array &&
                        TypeSystem.get(old.type).bound.kind == BoundKind::Unspecified) {
                        old.type = NewDeclaration.type;
                    }
                    old.declarations.push_back(NewDeclaration.source);
                    return id;
                }
                SymbolEntries.push_back({NewDeclaration.name,
                                         NewDeclaration.kind,
                                         NewDeclaration.type,
                                         CurrentScope,
                                         NewDeclaration.linkage,
                                         NewDeclaration.definition,
                                         NewDeclaration.isStatic,
                                         NewDeclaration.source,
                                         {},
                                         {NewDeclaration.source}});
                SymbolId id{static_cast<uint32_t>(SymbolEntries.size() - 1)};
                ScopeArena.at(CurrentScope.value).names[NewDeclaration.name].push_back(id);
                return id;
            }

            bool completeObject(SymbolId id, TypeId completed) {
                if (!id.value || !completed.value) {
                    return false;
                }
                auto& symbol = SymbolEntries.at(id.value);
                const auto& old = TypeSystem.get(symbol.type);
                const auto& next = TypeSystem.get(completed);
                if (symbol.kind != SymbolKind::Object || old.kind != TypeKind::Array || next.kind != TypeKind::Array ||
                    old.bound.kind != BoundKind::Unspecified || next.bound.kind != BoundKind::Constant ||
                    old.element != next.element || !TypeSystem.layout(completed)) {
                    return false;
                }
                symbol.type = completed;
                return true;
            }

            bool setConstant(SymbolId id, int64_t value) {
                if (!id.value || get(id).kind != SymbolKind::Enumerator) {
                    return false;
                }
                SymbolEntries.at(id.value).constant = value;
                return true;
            }

            // C supplies an ancestor scope as the destination. Results are in cleanup order.
            std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const {
                std::vector<SymbolId> result;
                auto ScopeToVisit = from;
                while (ScopeToVisit.value && ScopeToVisit != destination) {
                    if (scope(ScopeToVisit).kind != ScopeKind::Global) {
                        for (uint32_t Index = static_cast<uint32_t>(SymbolEntries.size()); Index > 1; --Index) {
                            const auto& entry = SymbolEntries[Index - 1];
                            if (entry.scope == ScopeToVisit && !entry.isStatic &&
                                (entry.kind == SymbolKind::Object || entry.kind == SymbolKind::Parameter)) {
                                result.push_back({Index - 1});
                            }
                        }
                    }
                    ScopeToVisit = scope(ScopeToVisit).parent;
                }
                if (ScopeToVisit != destination) {
                    throw std::invalid_argument("cleanup destination is not an ancestor scope");
                }
                return result;
            }

            TypeId lookupType(const std::string& name, const void* source = nullptr) {
                auto found = lookup(name);
                if (found.size() == 1 &&
                    (get(found[0]).kind == SymbolKind::Alias || get(found[0]).kind == SymbolKind::Type)) {
                    return get(found[0]).type;
                }
                diagnose("type-name", "unknown or hidden type name '" + name + "'", source);
                return {};
            }

            void installTypes() {
                const std::pair<const char*, Builtin> names[] = {
                    {"void", Builtin::Void},   {"bool", Builtin::Bool},    {"char", Builtin::Char},
                    {"short", Builtin::Short}, {"int", Builtin::Int},      {"long", Builtin::Long},
                    {"float", Builtin::Float}, {"double", Builtin::Double}};
                for (auto [name, type] : names) {
                    declare({name, SymbolKind::Type, TypeSystem.builtin(type)});
                }
            }

            void installLibrary() {
                auto integer = TypeSystem.builtin(Builtin::Int);
                auto character = TypeSystem.builtin(Builtin::Char);
                auto string = TypeSystem.pointer(TypeSystem.constant(character));
                declare({"printf", SymbolKind::BuiltinFunction, TypeSystem.function(integer, {string}, true), false,
                         false, Linkage::External});
                declare({"scanf", SymbolKind::BuiltinFunction, TypeSystem.function(integer, {string}, true), false,
                         false, Linkage::External});
                // No guessed FILE layout or undeclared file API signatures.
            }
        };
    }
}
