#include "symbols.hh"
#include "types/parser/ast/expression.hh"

#include <stdexcept>

namespace baka {
    namespace semantic {
        bool Symbols::IsFunctionKind(SymbolKind Kind) {
            return Kind == SymbolKind::Function || Kind == SymbolKind::BuiltinFunction;
        }

        bool Symbols::AreCompatible(TypeId ExistingType, TypeId CandidateType) const {
            if (ExistingType == CandidateType) {
                return true;
            }
            const auto& LeftType = TypeSystem.get(ExistingType);
            const auto& RightType = TypeSystem.get(CandidateType);
            if (LeftType.kind != RightType.kind) {
                return false;
            }
            return LeftType.kind == TypeKind::Array && AreCompatible(LeftType.element, RightType.element) &&
                   (LeftType.bound.kind == BoundKind::Unspecified || RightType.bound.kind == BoundKind::Unspecified ||
                    LeftType.bound == RightType.bound);
        }

        Symbols::Symbols(Types& TypeSystem) : TypeSystem(TypeSystem) {
            enter(ScopeKind::Global);
        }

        ScopeId Symbols::currentScope() const {
            return CurrentScope;
        }

        const Scope& Symbols::scope(ScopeId id) const {
            return ScopeArena.at(id.value);
        }

        const Symbol& Symbols::get(SymbolId id) const {
            return SymbolEntries.at(id.value);
        }

        const std::vector<Diagnostic>& Symbols::diagnostics() const {
            return Diagnostics;
        }

        void Symbols::diagnose(std::string code, std::string message, const types::ASTNode* source,
                               const types::ASTNode* previous) {
            Diagnostics.push_back({std::move(code), std::move(message), source, previous});
        }

        ScopeId Symbols::enter(ScopeKind kind) {
            ScopeArena.push_back({CurrentScope, kind, {}});
            CurrentScope = {static_cast<uint32_t>(ScopeArena.size() - 1)};
            return CurrentScope;
        }

        bool Symbols::leave() {
            if (!CurrentScope.value || !scope(CurrentScope).parent.value) {
                return false;
            }
            CurrentScope = scope(CurrentScope).parent;
            return true;
        }

        std::vector<SymbolId> Symbols::lookupCurrent(const std::string& name) const {
            auto it = scope(CurrentScope).names.find(name);
            return it == scope(CurrentScope).names.end() ? std::vector<SymbolId>{} : it->second;
        }

        std::vector<SymbolId> Symbols::lookup(const std::string& name) const {
            for (auto ScopeToVisit = CurrentScope; ScopeToVisit.value; ScopeToVisit = scope(ScopeToVisit).parent) {
                auto it = scope(ScopeToVisit).names.find(name);
                if (it != scope(ScopeToVisit).names.end()) {
                    std::vector<SymbolId> visible;
                    for (auto id : it->second) {
                        // Top-level function signatures are collected before bodies;
                        // objects still obey their source declaration point.
                        if (scope(ScopeToVisit).kind != ScopeKind::Global ||
                            get(id).introduced.value <= GlobalVisibility || IsFunctionKind(get(id).kind)) {
                            visible.push_back(id);
                        }
                    }
                    if (!visible.empty()) {
                        return visible;
                    }
                }
            }
            return {};
        }

        SymbolId Symbols::bind(const std::string& name, const types::ASTNode* source) {
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

        bool Symbols::defaults(SymbolId id, const std::vector<const types::ExpressionNode*>& arguments) {
            if (!id.value) {
                return false;
            }
            auto& entry = SymbolEntries.at(id.value);
            if (entry.defaultArguments.empty()) {
                entry.defaultArguments.resize(arguments.size());
                entry.defaultArgumentPoints.resize(arguments.size());
            }
            bool valid = true;
            for (size_t i = 0; i < arguments.size(); ++i) {
                if (!arguments[i]) {
                    continue;
                }
                if (entry.defaultArguments[i]) {
                    diagnose("default-argument", "default argument is already specified", arguments[i],
                             entry.defaultArguments[i]);
                    valid = false;
                } else {
                    entry.defaultArguments[i] = arguments[i];
                    entry.defaultArgumentPoints[i] = CurrentPoint;
                }
            }
            return valid;
        }

        bool Symbols::defaultVisible(SymbolId id, size_t parameter, uint64_t localPoint) const {
            const auto& symbol = get(id);
            const auto kind = scope(symbol.scope).kind;
            const auto limit = kind == ScopeKind::Global      ? GlobalVisibility
                               : kind == ScopeKind::Aggregate ? std::numeric_limits<uint64_t>::max()
                                                              : localPoint;
            return parameter < symbol.defaultArguments.size() && symbol.defaultArguments[parameter] &&
                   symbol.defaultArgumentPoints[parameter].value <= limit;
        }

        SymbolId Symbols::declare(Declaration NewDeclaration) {
            const DeclarationPoint point{++CurrentPoint.value};
            if (NewDeclaration.source) {
                DeclarationPoints[NewDeclaration.source] = point;
            }
            if (NewDeclaration.name == "__baka_allocate" || NewDeclaration.name == "__baka_deallocate") {
                diagnose("reserved", "name is reserved for the allocation runtime", NewDeclaration.source);
                return {};
            }
            if (!NewDeclaration.type.value) {
                diagnose("type", "invalid declaration type", NewDeclaration.source);
                return {};
            }
            if (IsFunctionKind(NewDeclaration.kind) && TypeSystem.get(NewDeclaration.type).kind != TypeKind::Function) {
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
                            diagnose("builtin", "incompatible redeclaration of builtin '" + NewDeclaration.name + "'",
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
                    diagnose("redeclaration", "conflicting or repeated declaration of '" + NewDeclaration.name + "'",
                             NewDeclaration.source, old.source);
                    return {};
                }
                old.defined = old.defined || NewDeclaration.definition;
                if (TypeSystem.get(old.type).kind == TypeKind::Array &&
                    TypeSystem.get(old.type).bound.kind == BoundKind::Unspecified) {
                    old.type = NewDeclaration.type;
                }
                old.declarations.push_back({point, NewDeclaration.source});
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
                                     {{point, NewDeclaration.source}},
                                     {},
                                     point,
                                     {}});
            SymbolId id{static_cast<uint32_t>(SymbolEntries.size() - 1)};
            ScopeArena.at(CurrentScope.value).names[NewDeclaration.name].push_back(id);
            return id;
        }

        bool Symbols::completeObject(SymbolId id, TypeId completed) {
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

        bool Symbols::setConstant(SymbolId id, int64_t value) {
            if (!id.value || (get(id).kind != SymbolKind::Enumerator &&
                              !(get(id).kind == SymbolKind::Object && TypeSystem.isConst(get(id).type) &&
                                TypeSystem.isIntegral(get(id).type)))) {
                return false;
            }
            SymbolEntries.at(id.value).constant = value;
            return true;
        }

        std::vector<SymbolId> Symbols::objectsLeaving(ScopeId from, ScopeId destination) const {
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

        TypeId Symbols::lookupType(const std::string& name, const types::ASTNode* source) {
            auto found = lookup(name);
            if (found.size() == 1 &&
                (get(found[0]).kind == SymbolKind::Alias || get(found[0]).kind == SymbolKind::Type)) {
                return get(found[0]).type;
            }
            diagnose("type-name", "unknown or hidden type name '" + name + "'", source);
            return {};
        }

        void Symbols::installTypes() {
            const std::pair<const char*, Builtin> names[] = {{"void", Builtin::Void},   {"bool", Builtin::Bool},
                                                             {"char", Builtin::Char},   {"short", Builtin::Short},
                                                             {"int", Builtin::Int},     {"long", Builtin::Long},
                                                             {"float", Builtin::Float}, {"double", Builtin::Double}};
            for (auto [name, type] : names) {
                declare({name, SymbolKind::Type, TypeSystem.builtin(type)});
            }
        }

        void Symbols::installLibrary() {
            auto integer = TypeSystem.builtin(Builtin::Int);
            auto character = TypeSystem.builtin(Builtin::Char);
            auto string = TypeSystem.pointer(TypeSystem.constant(character));
            declare({"printf", SymbolKind::BuiltinFunction, TypeSystem.function(integer, {string}, true), false, false,
                     Linkage::External});
            declare({"scanf", SymbolKind::BuiltinFunction, TypeSystem.function(integer, {string}, true), false, false,
                     Linkage::External});
            // No guessed FILE layout or undeclared file API signatures.
        }
    }
}
