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

        public:
            Declarations(Types& TypeSystem, Symbols& SymbolTable);

            TypeId specifier(std::string name, bool constant = false, bool isUnsigned = false,
                             std::optional<TypeKind> expected = {}, const void* source = nullptr);

            TypeId boundArray(TypeId element, IntegerResult value, const void* expression = nullptr,
                              bool omitted = false);

            bool validate(TypeId id, DeclarationContext context = {}, const void* source = nullptr);

            SymbolId object(Declaration NewDeclaration, DeclarationContext context = {});

            SymbolId functionDefinition(Declaration NewDeclaration);

            SymbolId alias(std::string name, TypeId type, const void* source = nullptr);

            TypeId aggregate(std::string name, TypeKind kind, const void* source = nullptr);

            bool beginAggregate(TypeId id, const void* source = nullptr);

            bool member(TypeId owner, Member member);

            bool base(TypeId owner, TypeId base, const void* source = nullptr);

            bool finishAggregate(TypeId type, const void* source = nullptr);

            SymbolId enumerator(std::string name, TypeId type, IntegerResult value, const void* source = nullptr);

            IntegerResult nextEnumerator(std::optional<int64_t> previous) const;

            SpecialMembers specialMembers(TypeId id) const;

            struct Access {
                TypeId type;
                TypeId owner;
                std::vector<size_t> candidates;
                bool lvalue = false;
            };

            Access memberAccess(TypeId receiver, const std::string& name, bool arrow = false,
                                bool staticContext = false, const void* source = nullptr);

            TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod, const void* source = nullptr);
        };
    }
}
