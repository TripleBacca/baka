#pragma once

#include "symbols.hh"

namespace baka {
    namespace semantic {
        // B supplies the result of required constant evaluation. Unavailable is not a constant.
        enum class ConstantState { Value, Nonconstant, Unavailable, Error };

        struct IntegerResult {
            ConstantState state = ConstantState::Unavailable;
            int64_t value = 0;
        };

        struct DeclarationContext {
            bool parameter = false;
            bool member = false;
            bool prototype = false;
            bool storageRequired = true;
        };

        struct SpecialMembers {
            std::vector<size_t> constructors;
            std::optional<size_t> destructor;
            bool implicitDefaultConstructor = false;
            bool implicitCopyConstructor = false;
            bool implicitDestructor = false;
            bool nontrivial = false;
        };

        class Declarations {
            Types& TypeSystem;
            Symbols& SymbolTable;
            mutable std::unordered_map<uint32_t, SpecialMembers> SpecialMemberCache;

        public:
            Declarations(Types& TypeSystem, Symbols& SymbolTable);

            TypeId specifier(std::string name, bool constant = false, bool isUnsigned = false,
                             std::optional<TypeKind> expected = {}, const types::ASTNode* source = nullptr);

            TypeId boundArray(TypeId element, IntegerResult value, const types::ASTNode* expression = nullptr,
                              bool omitted = false);

            bool validate(TypeId id, DeclarationContext context = {}, const types::ASTNode* source = nullptr);

            SymbolId object(Declaration NewDeclaration, DeclarationContext context = {});

            SymbolId functionDefinition(Declaration NewDeclaration);

            SymbolId alias(std::string name, TypeId type, const types::ASTNode* source = nullptr);

            TypeId aggregate(std::string name, TypeKind kind, const types::ASTNode* source = nullptr);

            bool beginAggregate(TypeId id, const types::ASTNode* source = nullptr);

            bool member(TypeId owner, Member member);

            bool base(TypeId owner, TypeId base, semantic::Access access, const types::ASTNode* source = nullptr);

            bool finishAggregate(TypeId type, const types::ASTNode* source = nullptr);

            SymbolId enumerator(std::string name, TypeId type, IntegerResult value,
                                const types::ASTNode* source = nullptr);

            IntegerResult nextEnumerator(std::optional<int64_t> previous) const;

            SpecialMembers specialMembers(TypeId id) const;

            struct Access {
                TypeId type;
                TypeId owner;
                std::vector<size_t> candidates;
                bool lvalue = false;
            };

            // accessContext: the class whose member is being checked (empty TypeId at
            // namespace scope). Without it access is checked later by the caller (calls).
            Access memberAccess(TypeId receiver, const std::string& name, bool arrow = false,
                                bool staticContext = false, const types::ASTNode* source = nullptr,
                                std::optional<TypeId> accessContext = std::nullopt);

            // Whether member `index` of `owner`, named through class `naming`, is accessible
            // from member functions of `context` (no context: namespace scope).
            bool accessible(TypeId naming, TypeId owner, size_t index, TypeId context) const;

            // accessible(), diagnosing "'m' is private within this context" when not.
            bool checkAccess(TypeId naming, TypeId owner, size_t index, TypeId context,
                             const types::ASTNode* source);

            TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod,
                            const types::ASTNode* source = nullptr);
        };
    }
}
