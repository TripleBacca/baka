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

            bool base(TypeId owner, TypeId base, const types::ASTNode* source = nullptr);

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

            Access memberAccess(TypeId receiver, const std::string& name, bool arrow = false,
                                bool staticContext = false, const types::ASTNode* source = nullptr);

            TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod,
                            const types::ASTNode* source = nullptr);
        };
    }
}
