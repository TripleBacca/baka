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

        struct DeclarationPoint {
            uint64_t value = 0;
            bool operator==(const DeclarationPoint&) const = default;
        };

        struct DeclarationRecord {
            DeclarationPoint point;
            const types::ASTNode* source = nullptr;
            bool operator==(const DeclarationRecord&) const = default;
        };

        struct Diagnostic {
            std::string code, message;
            const types::ASTNode* source = nullptr;
            const types::ASTNode* previous = nullptr;
        };

        struct Symbol {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            ScopeId scope;
            Linkage linkage = Linkage::None;
            bool defined = false;
            bool isStatic = false;
            const types::ASTNode* source = nullptr;
            std::optional<int64_t> constant;
            std::vector<DeclarationRecord> declarations;
            std::vector<const types::ExpressionNode*> defaultArguments{};
            DeclarationPoint introduced;
            std::vector<DeclarationPoint> defaultArgumentPoints;
            bool operator==(const Symbol&) const = default;
        };

        struct Scope {
            ScopeId parent;
            ScopeKind kind = ScopeKind::Block;
            std::unordered_map<std::string, std::vector<SymbolId>> names;
            bool operator==(const Scope&) const = default;
        };

        struct Declaration {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            bool definition = false;
            bool isStatic = false;
            Linkage linkage = Linkage::None;
            const types::ASTNode* source = nullptr;
        };

        class Symbols {
            Types& TypeSystem;
            std::deque<Symbol> SymbolEntries{Symbol{}};
            std::deque<Scope> ScopeArena{Scope{}};
            ScopeId CurrentScope;
            std::vector<Diagnostic> Diagnostics;
            uint64_t GlobalVisibility = std::numeric_limits<uint64_t>::max();
            DeclarationPoint CurrentPoint;
            std::unordered_map<const types::ASTNode*, DeclarationPoint> DeclarationPoints;

            static bool IsFunctionKind(SymbolKind Kind);
            bool AreCompatible(TypeId ExistingType, TypeId CandidateType) const;

        public:
            explicit Symbols(Types& TypeSystem);

            ScopeId currentScope() const;
            const Scope& scope(ScopeId id) const;
            const Symbol& get(SymbolId id) const;
            const std::vector<Diagnostic>& diagnostics() const;

            void diagnose(std::string code, std::string message, const types::ASTNode* source = nullptr,
                          const types::ASTNode* previous = nullptr);

            ScopeId enter(ScopeKind kind);
            bool leave();

            void selectScope(ScopeId id) {
                CurrentScope = id;
            }

            uint32_t symbolCount() const {
                return static_cast<uint32_t>(SymbolEntries.size() - 1);
            }

            uint32_t scopeCount() const {
                return static_cast<uint32_t>(ScopeArena.size() - 1);
            }

            void setGlobalVisibility(uint64_t limit) {
                GlobalVisibility = limit;
            }

            uint64_t declarationPoint() const {
                return CurrentPoint.value;
            }

            uint64_t declarationPoint(const types::ASTNode* source) const {
                auto found = DeclarationPoints.find(source);
                return found == DeclarationPoints.end() ? CurrentPoint.value : found->second.value;
            }

            bool defaultVisible(SymbolId id, size_t parameter, uint64_t localPoint) const;
            bool defaults(SymbolId id, const std::vector<const types::ExpressionNode*>& arguments);

            std::vector<SymbolId> lookupCurrent(const std::string& name) const;
            std::vector<SymbolId> lookup(const std::string& name) const;

            SymbolId bind(const std::string& name, const types::ASTNode* source = nullptr);
            SymbolId declare(Declaration NewDeclaration);
            bool completeObject(SymbolId id, TypeId completed);
            bool setConstant(SymbolId id, int64_t value);

            // C supplies an ancestor scope as the destination. Results are in cleanup order.
            std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const;

            TypeId lookupType(const std::string& name, const types::ASTNode* source = nullptr);

            void installTypes();
            void installLibrary();
        };
    }
}
