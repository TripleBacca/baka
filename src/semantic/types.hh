#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace baka {
    namespace types {
        class ASTNode;
        class ExpressionNode;
    }

    namespace semantic {
        // IDs are arena indices. Zero is always invalid/error; never a fabricated int.
        struct TypeId {
            uint32_t value = 0;
            bool operator==(const TypeId&) const = default;
        };

        struct SymbolId {
            uint32_t value = 0;
            bool operator==(const SymbolId&) const = default;
        };

        struct ScopeId {
            uint32_t value = 0;
            bool operator==(const ScopeId&) const = default;
        };

        enum class TypeKind { TypeError, Builtin, Pointer, Array, Function, Const, Enum, Struct, Class, Union };

        enum class Builtin { Void, Bool, Char, UChar, Short, UShort, Int, UInt, Long, ULong, Float, Double };

        enum class Completion { Forward, Defining, Complete };

        enum class BoundKind { Unspecified, Constant };

        struct Bound {
            BoundKind kind = BoundKind::Unspecified;
            uint64_t count = 0;
            bool operator==(const Bound&) const = default;
        };

        struct Member {
            std::string name;
            TypeId type;
            SymbolId symbol{};
            bool isStatic = false;
            bool isType = false;
            bool isConstructor = false;
            bool isDestructor = false;
            bool isConstMethod = false;
            const types::ASTNode* source = nullptr;
            size_t minimumArity = static_cast<size_t>(-1);
            const types::ExpressionNode* initializer = nullptr;
            bool operator==(const Member&) const = default;
        };

        struct Layout {
            uint64_t size = 0, alignment = 1;
            std::vector<std::optional<uint64_t>> offsets; // indexed like Type::members
            std::optional<uint64_t> baseOffset;
            bool operator==(const Layout&) const = default;
        };

        struct Type {
            TypeKind kind = TypeKind::TypeError;
            Builtin builtin = Builtin::Void;
            TypeId element; // pointee, element, return, or qualified type
            Bound bound;
            std::vector<TypeId> parameters;
            bool variadic = false;
            std::string name;
            Completion completion = Completion::Forward;
            TypeId base;
            std::vector<Member> members; // source order, includes static/type/special members
            // Published once by successful finishDefinition. Forward/failed
            // definitions have no layout. Completed nominal types cannot change
            // storage shape, so all later queries reuse these offsets and size.
            std::optional<Layout> objectLayout;
            bool operator==(const Type&) const = default;
        };

        // 64-bit mixing constant (Kynea / golden ratio derived).
        static constexpr uint64_t HashSeed = 0x9e3779b97f4a7c15ULL;

        inline void MixHash(size_t& Hash, size_t Value) {
            Hash ^= Value + static_cast<size_t>(HashSeed) + (Hash << 6) + (Hash >> 2);
        }

        // Fills the seed with the vector length and every element ID, order-sensitive.
        inline size_t HashVector(size_t Seed, const std::vector<TypeId>& Values) {
            auto Hash = Seed;
            MixHash(Hash, Values.size());
            for (auto Current : Values) {
                MixHash(Hash, Current.value);
            }
            return Hash;
        }

        enum class QueryState { Ready, Incomplete, Error };

        struct LayoutResult {
            QueryState state = QueryState::Error;
            Layout layout;
            std::string message;

            explicit operator bool() const {
                return state == QueryState::Ready;
            }
        };

        // Fixed LP64 target contract. This service performs no expression evaluation.
        class Types {
            std::deque<Type> TypeArena{Type{}};
            std::vector<TypeId> BuiltinTypes;

            struct TypeKey {
                TypeKind kind;
                Builtin builtin;
                TypeId element;
                Bound bound;
                std::vector<TypeId> parameters;
                bool variadic;
                bool operator==(const TypeKey&) const = default;
            };

            struct KeyHash {
                size_t operator()(const TypeKey& key) const;
            };

            std::unordered_map<TypeKey, TypeId, KeyHash> InternTable;

            TypeId AddType(Type type);
            TypeId InternType(Type type);
            static bool AddChecked(uint64_t Left, uint64_t Right, uint64_t& out);
            static bool AlignChecked(uint64_t size, uint64_t alignment, uint64_t& out);
            LayoutResult ComputeLayout(TypeId id, std::vector<TypeId>& active) const;
            LayoutResult ComputeStorageLayout(TypeId id, std::vector<TypeId>& active) const;

        public:
            // Identities belong to one compilation context.
            Types(const Types&) = delete;
            Types& operator=(const Types&) = delete;

            Types();

            const Type& get(TypeId id) const;

            size_t typeCount() const {
                return TypeArena.size() - 1;
            }

            TypeId builtin(Builtin Right) const;

            struct ArithmeticInfo {
                unsigned bits, rank;
                bool isSigned, floating;
            };

            std::optional<ArithmeticInfo> arithmetic(TypeId id) const;

            TypeId sizeType() const;
            TypeId unConst(TypeId id) const;
            bool isConst(TypeId id) const;
            bool isAggregate(TypeId id) const;
            bool isIntegral(TypeId id) const;
            bool isVoid(TypeId id) const;

            TypeId constant(TypeId id, bool constantValue = true);
            TypeId pointer(TypeId pointee);
            TypeId array(TypeId element, Bound bound = {});
            TypeId parameterObject(TypeId id);
            TypeId adjustParameter(TypeId id);
            TypeId function(TypeId result, std::vector<TypeId> parameters = {}, bool variadic = false);

            // Nominal declarations are never structurally interned; aliases keep this ID.
            TypeId nominal(TypeKind kind, std::string name = {});

            bool beginDefinition(TypeId id);
            bool setBase(TypeId id, TypeId base);
            bool addMember(TypeId id, Member member);
            bool bindMemberSymbol(TypeId owner, size_t index, SymbolId symbol);
            LayoutResult finishDefinition(TypeId id);
            LayoutResult layout(TypeId id) const;

            // Borrow cached nominal layout without copying its offset vector.
            const Layout* storedLayout(TypeId id) const {
                const auto& type = get(unConst(id));
                return type.objectLayout ? &*type.objectLayout : nullptr;
            }

            // Checks array formation even below pointer layers, independently of sizeof(pointer).
            bool validDerived(TypeId id) const;

            struct MemberLookup {
                TypeId owner;
                std::vector<size_t> indices;
            };

            MemberLookup lookupMember(TypeId receiver, const std::string& name) const;
            bool derivesFrom(TypeId derived, TypeId base) const;
            bool recursivelyConst(TypeId id) const;

            struct Subobject {
                TypeId type;
                std::optional<size_t> memberIndex;
            };

            std::vector<Subobject> subobjects(TypeId id, bool destruction = false) const;
            // Construction and cleanup share these ordered subobject lists; C records jumps.
            std::vector<size_t> instanceMembers(TypeId id, bool destruction = false) const;
        };
    }
}
