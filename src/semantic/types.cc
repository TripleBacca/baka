#include "types.hh"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace baka {
    namespace semantic {
        size_t Types::CanonicalHash(const Type& TypeInfo) const {
            size_t Hash = static_cast<size_t>(TypeInfo.kind) * static_cast<size_t>(0xff51afd7ed558ccdULL);
            MixHash(Hash, static_cast<size_t>(TypeInfo.builtin));
            MixHash(Hash, static_cast<size_t>(TypeInfo.element.value));
            MixHash(Hash, static_cast<size_t>(TypeInfo.bound.kind));
            MixHash(Hash, static_cast<size_t>(TypeInfo.bound.count));
            Hash = HashVector(Hash, TypeInfo.parameters);
            MixHash(Hash, static_cast<size_t>(TypeInfo.variadic ? 1u : 0u));
            return Hash;
        }

        bool Types::CanonicalEqual(const Type& LeftType, const Type& RightType) const {
            return LeftType.kind == RightType.kind && LeftType.builtin == RightType.builtin &&
                   LeftType.element == RightType.element && LeftType.bound == RightType.bound &&
                   LeftType.parameters == RightType.parameters && LeftType.variadic == RightType.variadic;
        }

        size_t Types::InternHash::operator()(uint32_t index) const {
            return TypeSystem->CanonicalHash(TypeSystem->TypeArena[index]);
        }

        bool Types::InternEqual::operator()(uint32_t Left, uint32_t Right) const {
            return Left == Right ||
                   TypeSystem->CanonicalEqual(TypeSystem->TypeArena[Left], TypeSystem->TypeArena[Right]);
        }

        TypeId Types::AddType(Type type) {
            if (TypeArena.size() >= std::numeric_limits<uint32_t>::max()) {
                throw std::length_error("type arena exhausted");
            }
            TypeArena.push_back(std::move(type));
            return {static_cast<uint32_t>(TypeArena.size() - 1)};
        }

        TypeId Types::InternType(Type type) {
            auto id = AddType(std::move(type));
            auto [Entry, added] = InternTable.insert(id.value);
            if (!added) {
                TypeArena.pop_back();
                return {*Entry};
            }
            return id;
        }

        bool Types::AddChecked(uint64_t Left, uint64_t Right, uint64_t& out) {
            if (Right > std::numeric_limits<uint64_t>::max() - Left) {
                return false;
            }
            out = Left + Right;
            return true;
        }

        bool Types::AlignChecked(uint64_t size, uint64_t alignment, uint64_t& out) {
            auto padding = (alignment - size % alignment) % alignment;
            return AddChecked(size, padding, out);
        }

        LayoutResult Types::ComputeLayout(TypeId id, std::vector<TypeId>& active) const {
            id = unConst(id);
            const auto& TypeInfo = get(id);
            if (TypeInfo.objectLayout) {
                return {QueryState::Ready, *TypeInfo.objectLayout, {}};
            }
            if (TypeInfo.kind == TypeKind::TypeError) {
                return {QueryState::Error, {}, "invalid type"};
            }
            if (TypeInfo.kind == TypeKind::Function) {
                return {QueryState::Error, {}, "function has no object layout"};
            }
            if (TypeInfo.kind == TypeKind::Builtin) {
                uint64_t Size = 0;
                switch (TypeInfo.builtin) {
                case Builtin::Void:
                    return {QueryState::Error, {}, "void has no object layout"};
                case Builtin::Bool:
                case Builtin::Char:
                case Builtin::UChar:
                    Size = 1;
                    break;
                case Builtin::Short:
                case Builtin::UShort:
                    Size = 2;
                    break;
                case Builtin::Int:
                case Builtin::UInt:
                case Builtin::Float:
                    Size = 4;
                    break;
                default:
                    Size = 8;
                    break;
                }
                return {QueryState::Ready, {Size, Size, {}, {}}, {}};
            }
            if (TypeInfo.kind == TypeKind::Pointer) {
                return {QueryState::Ready, {8, 8, {}, {}}, {}};
            }
            if (TypeInfo.kind == TypeKind::Enum) {
                if (TypeInfo.completion != Completion::Complete) {
                    return {QueryState::Incomplete, {}, "incomplete enum"};
                }
                return {QueryState::Ready, {4, 4, {}, {}}, {}};
            }
            if (std::find(active.begin(), active.end(), id) != active.end()) {
                return {QueryState::Error, {}, "recursive by-value containment"};
            }
            active.push_back(id);

            struct LayoutStackGuard {
                std::vector<TypeId>& ActiveTypes;

                ~LayoutStackGuard() {
                    ActiveTypes.pop_back();
                }
            } StackGuard{active};

            if (TypeInfo.kind == TypeKind::Array) {
                auto element = ComputeLayout(TypeInfo.element, active);
                if (!element) {
                    return element;
                }
                if (TypeInfo.bound.kind == BoundKind::Unspecified) {
                    return {QueryState::Incomplete, {}, "array bound is unspecified"};
                }
                if (!TypeInfo.bound.count ||
                    TypeInfo.bound.count > std::numeric_limits<uint64_t>::max() / element.layout.size) {
                    return {QueryState::Error, {}, "invalid or overflowing array size"};
                }
                element.layout.size *= TypeInfo.bound.count;
                element.layout.offsets.clear();
                element.layout.baseOffset.reset();
                return element;
            }
            if (TypeInfo.completion != Completion::Complete) {
                return {QueryState::Incomplete, {}, "incomplete aggregate"};
            }
            Layout result;
            if (TypeInfo.base.value) {
                auto base = ComputeLayout(TypeInfo.base, active);
                if (!base) {
                    return base;
                }
                result.size = base.layout.size;
                result.alignment = base.layout.alignment;
                result.baseOffset = 0;
            }
            for (const auto& member : TypeInfo.members) {
                const auto& MemberType = get(unConst(member.type));
                if (member.isStatic || member.isType || member.isConstructor || member.isDestructor ||
                    MemberType.kind == TypeKind::Function) {
                    result.offsets.push_back(std::nullopt);
                    continue;
                }
                auto field = ComputeLayout(member.type, active);
                if (!field) {
                    return field;
                }
                result.alignment = std::max(result.alignment, field.layout.alignment);
                uint64_t offset = 0;
                if (TypeInfo.kind == TypeKind::Union) {
                    result.size = std::max(result.size, field.layout.size);
                }
                else if (!AlignChecked(result.size, field.layout.alignment, offset) ||
                    !AddChecked(offset, field.layout.size, result.size)) {
                    return {QueryState::Error, {}, "aggregate size overflow"};
                }
                result.offsets.push_back(offset);
            }
            result.size = std::max(uint64_t{1}, result.size); // empty objects occupy distinct addresses
            if (!AlignChecked(result.size, result.alignment, result.size)) {
                return {QueryState::Error, {}, "aggregate size overflow"};
            }
            return {QueryState::Ready, std::move(result), {}};
        }

        Types::Types() : InternTable{8, {this}, {this}} {
            for (unsigned Index = 0; Index <= static_cast<unsigned>(Builtin::Double); ++Index) {
                Type TypeInfo;
                TypeInfo.kind = TypeKind::Builtin;
                TypeInfo.builtin = static_cast<Builtin>(Index);
                BuiltinTypes.push_back(AddType(std::move(TypeInfo)));
            }
        }

        const Type& Types::get(TypeId id) const {
            return TypeArena.at(id.value);
        }

        TypeId Types::builtin(Builtin Right) const {
            return BuiltinTypes.at(static_cast<unsigned>(Right));
        }

        std::optional<Types::ArithmeticInfo> Types::arithmetic(TypeId id) const {
            const auto& TypeInfo = get(unConst(id));
            if (TypeInfo.kind == TypeKind::Enum) {
                return ArithmeticInfo{32, 3, true, false};
            }
            if (TypeInfo.kind != TypeKind::Builtin) {
                return std::nullopt;
            }
            switch (TypeInfo.builtin) {
            case Builtin::Void:
                return std::nullopt;
            case Builtin::Bool:
                return ArithmeticInfo{1, 0, false, false};
            case Builtin::Char:
                return ArithmeticInfo{8, 1, true, false};
            case Builtin::UChar:
                return ArithmeticInfo{8, 1, false, false};
            case Builtin::Short:
                return ArithmeticInfo{16, 2, true, false};
            case Builtin::UShort:
                return ArithmeticInfo{16, 2, false, false};
            case Builtin::Int:
                return ArithmeticInfo{32, 3, true, false};
            case Builtin::UInt:
                return ArithmeticInfo{32, 3, false, false};
            case Builtin::Long:
                return ArithmeticInfo{64, 4, true, false};
            case Builtin::ULong:
                return ArithmeticInfo{64, 4, false, false};
            case Builtin::Float:
                return ArithmeticInfo{32, 5, true, true};
            case Builtin::Double:
                return ArithmeticInfo{64, 6, true, true};
            }
            return std::nullopt;
        }

        TypeId Types::sizeType() const {
            return builtin(Builtin::ULong);
        }

        TypeId Types::unConst(TypeId id) const {
            if (get(id).kind != TypeKind::Const) {
                return id;
            }
            return get(id).element;
        }

        bool Types::isConst(TypeId id) const {
            return get(id).kind == TypeKind::Const;
        }

        bool Types::isAggregate(TypeId id) const {
            auto Kind = get(unConst(id)).kind;
            return Kind == TypeKind::Struct || Kind == TypeKind::Class || Kind == TypeKind::Union;
        }

        bool Types::isIntegral(TypeId id) const {
            const auto& TypeInfo = get(unConst(id));
            return TypeInfo.kind == TypeKind::Enum ||
            (TypeInfo.kind == TypeKind::Builtin && TypeInfo.builtin >= Builtin::Bool &&
                TypeInfo.builtin <= Builtin::ULong);
        }

        bool Types::isVoid(TypeId id) const {
            const auto& TypeInfo = get(unConst(id));
            return TypeInfo.kind == TypeKind::Builtin && TypeInfo.builtin == Builtin::Void;
        }

        TypeId Types::constant(TypeId id, bool constantValue) {
            if (!constantValue || !id.value || isConst(id)) {
                return id;
            }
            // Const of an array consts its element (including typedef arrays).
            if (get(id).kind == TypeKind::Array) {
                auto TypeInfo = get(id);
                return array(constant(TypeInfo.element), TypeInfo.bound);
            }
            Type TypeInfo;
            TypeInfo.kind = TypeKind::Const;
            TypeInfo.element = id;
            return InternType(std::move(TypeInfo));
        }

        TypeId Types::pointer(TypeId pointee) {
            if (!pointee.value) {
                return {};
            }
            Type TypeInfo;
            TypeInfo.kind = TypeKind::Pointer;
            TypeInfo.element = pointee;
            return InternType(std::move(TypeInfo));
        }

        TypeId Types::array(TypeId element, Bound bound) {
            auto Kind = get(unConst(element)).kind;
            if (!element.value || isVoid(element) || Kind == TypeKind::Function ||
                (bound.kind == BoundKind::Constant && !bound.count)) {
                return {};
            }
            Type TypeInfo;
            TypeInfo.kind = TypeKind::Array;
            TypeInfo.element = element;
            TypeInfo.bound = bound;
            return InternType(std::move(TypeInfo));
        }

        TypeId Types::parameterObject(TypeId id) {
            const auto TypeInfo = get(unConst(id));
            if (TypeInfo.kind == TypeKind::Array) {
                return pointer(TypeInfo.element);
            }
            if (TypeInfo.kind == TypeKind::Function) {
                return pointer(id);
            }
            return id;
        }

        TypeId Types::adjustParameter(TypeId id) {
            return unConst(parameterObject(id));
        }

        TypeId Types::function(TypeId result, std::vector<TypeId> parameters, bool variadic) {
            auto Kind = get(unConst(result)).kind;
            if (!result.value || Kind == TypeKind::Array || Kind == TypeKind::Function) {
                return {};
            }
            for (auto& CurrentType : parameters) {
                CurrentType = adjustParameter(CurrentType);
                if (!CurrentType.value || isVoid(CurrentType)) {
                    return {};
                }
            }
            Type TypeInfo;
            TypeInfo.kind = TypeKind::Function;
            TypeInfo.element = result;
            TypeInfo.parameters = std::move(parameters);
            TypeInfo.variadic = variadic;
            return InternType(std::move(TypeInfo));
        }

        TypeId Types::nominal(TypeKind kind, std::string name) {
            if (kind != TypeKind::Struct && kind != TypeKind::Class && kind != TypeKind::Union &&
                kind != TypeKind::Enum) {
                return {};
            }
            Type TypeInfo;
            TypeInfo.kind = kind;
            TypeInfo.name = std::move(name);
            return AddType(std::move(TypeInfo));
        }

        bool Types::beginDefinition(TypeId id) {
            if (!id.value) {
                return false;
            }
            auto& TypeInfo = TypeArena.at(id.value);
            if (!(isAggregate(id) || TypeInfo.kind == TypeKind::Enum) ||
                TypeInfo.completion != Completion::Forward) {
                return false;
            }
            TypeInfo.completion = Completion::Defining;
            return true;
        }

        bool Types::setBase(TypeId id, TypeId base) {
            if (!isAggregate(id) || !isAggregate(base) || get(id).completion != Completion::Defining ||
                get(id).base.value || get(base).completion != Completion::Complete ||
                get(id).kind == TypeKind::Union || get(base).kind == TypeKind::Union) {
                return false;
            }
            for (auto CurrentType = base; CurrentType.value; CurrentType = get(CurrentType).base) {
                if (CurrentType == id) {
                    return false;
                }
            }
            TypeArena.at(id.value).base = base;
            return true;
        }

        bool Types::addMember(TypeId id, Member member) {
            if (!isAggregate(id) || get(id).completion != Completion::Defining || !member.type.value) {
                return false;
            }
            const auto& MemberType = get(unConst(member.type));
            for (const auto& MemberInfo : get(id).members) {
                if (MemberInfo.name != member.name) {
                    continue;
                }
                const auto& old = get(unConst(MemberInfo.type));
                // Overload on normalized parameter lists and receiver constness, never result.
                if (MemberType.kind == TypeKind::Function && old.kind == TypeKind::Function && !member.isType &&
                    !MemberInfo.isType && !member.isDestructor && !MemberInfo.isDestructor &&
                    (MemberType.parameters != old.parameters || MemberType.variadic != old.variadic ||
                        (member.isConstMethod != MemberInfo.isConstMethod && !member.isStatic &&
                            !MemberInfo.isStatic))) {
                    continue;
                }
                return false;
            }
            TypeArena.at(id.value).members.push_back(std::move(member));
            return true;
        }

        bool Types::bindMemberSymbol(TypeId owner, size_t index, SymbolId symbol) {
            if (!isAggregate(owner) || index >= get(owner).members.size() || !symbol.value) {
                return false;
            }
            TypeArena.at(owner.value).members[index].symbol = symbol;
            return true;
        }

        LayoutResult Types::finishDefinition(TypeId id) {
            if (!id.value || get(id).completion != Completion::Defining) {
                return {QueryState::Error, {}, "definition not open"};
            }
            TypeArena.at(id.value).completion = Completion::Complete;
            auto result = layout(id);
            if (!result) {
                TypeArena.at(id.value).completion = Completion::Defining;
            } else {
                // Eager publication keeps read-only semantic checks and IR
                // lowering truly read-only; queries never populate caches.
                TypeArena.at(id.value).objectLayout = result.layout;
            }
            return result;
        }

        LayoutResult Types::layout(TypeId id) const {
            std::vector<TypeId> active;
            return ComputeLayout(id, active);
        }

        bool Types::validDerived(TypeId id) const {
            const auto& TypeInfo = get(unConst(id));
            if (!id.value) {
                return false;
            }
            if (TypeInfo.kind == TypeKind::Array) {
                auto ElementLayout = layout(TypeInfo.element);
                if (ElementLayout.state != QueryState::Ready) {
                    return false;
                }
                if (!validDerived(TypeInfo.element)) {
                    return false;
                }
                return TypeInfo.bound.kind != BoundKind::Constant || static_cast<bool>(layout(id));
            }
            if (TypeInfo.kind == TypeKind::Pointer) {
                return validDerived(TypeInfo.element);
            }
            if (TypeInfo.kind == TypeKind::Function) {
                if (!validDerived(TypeInfo.element)) {
                    return false;
                }
                for (auto CurrentType : TypeInfo.parameters) {
                    if (!validDerived(CurrentType)) {
                        return false;
                    }
                }
            }
            return true;
        }

        Types::MemberLookup Types::lookupMember(TypeId receiver, const std::string& name) const {
            receiver = unConst(receiver);
            for (auto CurrentType = receiver; CurrentType.value && isAggregate(CurrentType);
                 CurrentType = get(CurrentType).base) {
                MemberLookup result{CurrentType, {}};
                for (size_t Index = 0; Index < get(CurrentType).members.size(); ++Index) {
                    if (get(CurrentType).members[Index].name == name) {
                        result.indices.push_back(Index);
                    }
                }
                if (!result.indices.empty()) {
                    return result; // derived declarations hide base overloads
                }
            }
            return {};
        }

        bool Types::derivesFrom(TypeId derived, TypeId base) const {
            if (!isAggregate(derived) || !isAggregate(base)) {
                return false;
            }
            for (auto CurrentType = get(unConst(derived)).base; CurrentType.value;
                 CurrentType = get(CurrentType).base) {
                if (CurrentType == unConst(base)) {
                    return true;
                }
            }
            return false;
        }

        bool Types::recursivelyConst(TypeId id) const {
            if (isConst(id)) {
                return true;
            }
            const auto& TypeInfo = get(id);
            if (TypeInfo.kind == TypeKind::Array) {
                return recursivelyConst(TypeInfo.element);
            }
            if (!isAggregate(id) || TypeInfo.completion != Completion::Complete) {
                return false;
            }
            if (TypeInfo.base.value && recursivelyConst(TypeInfo.base)) {
                return true;
            }
            for (const auto& MemberInfo : TypeInfo.members) {
                if (!MemberInfo.isStatic && !MemberInfo.isType &&
                    get(unConst(MemberInfo.type)).kind != TypeKind::Function &&
                    recursivelyConst(MemberInfo.type)) {
                    return true;
                }
            }
            return false;
        }

        std::vector<Types::Subobject> Types::subobjects(TypeId id, bool destruction) const {
            std::vector<Subobject> result;
            const auto& TypeInfo = get(unConst(id));
            if (TypeInfo.base.value) {
                result.push_back({TypeInfo.base, std::nullopt});
            }
            for (auto Index : instanceMembers(id)) {
                result.push_back({TypeInfo.members[Index].type, Index});
            }
            if (destruction) {
                std::reverse(result.begin(), result.end());
            }
            return result;
        }

        std::vector<size_t> Types::instanceMembers(TypeId id, bool destruction) const {
            std::vector<size_t> result;
            const auto& TypeInfo = get(unConst(id));
            for (size_t Index = 0; Index < TypeInfo.members.size(); ++Index) {
                const auto& MemberInfo = TypeInfo.members[Index];
                if (!MemberInfo.isStatic && !MemberInfo.isType &&
                    get(unConst(MemberInfo.type)).kind != TypeKind::Function) {
                    result.push_back(Index);
                }
            }
            if (destruction) {
                std::reverse(result.begin(), result.end());
            }
            return result;
        }
    }
}
