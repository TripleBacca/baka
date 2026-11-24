#include "declarations.hh"

#include <algorithm>
#include <limits>

namespace baka {
    namespace semantic {
        Declarations::Declarations(Types& TypeSystem, Symbols& SymbolTable)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable) {
        }

        TypeId Declarations::specifier(std::string name, bool constant, bool isUnsigned,
                                       std::optional<TypeKind> expected, const types::ASTNode* source) {
            auto id = SymbolTable.lookupType(name, source);
            if (!id.value) {
                return {};
            }
            if (expected && TypeSystem.get(TypeSystem.unConst(id)).kind != *expected &&
                !((*expected == TypeKind::Struct || *expected == TypeKind::Class) && TypeSystem.isAggregate(id) &&
                    TypeSystem.get(id).kind != TypeKind::Union)) {
                SymbolTable.diagnose("type-kind", "type does not match aggregate/enum specifier", source);
                return {};
            }
            if (isUnsigned) {
                // Only direct primitive spellings may carry unsigned; aliases are not specifiers.
                if (name == "char") {
                    id = TypeSystem.builtin(Builtin::UChar);
                }
                else if (name == "short") {
                    id = TypeSystem.builtin(Builtin::UShort);
                }
                else if (name == "int") {
                    id = TypeSystem.builtin(Builtin::UInt);
                }
                else if (name == "long") {
                    id = TypeSystem.builtin(Builtin::ULong);
                }
                else {
                    SymbolTable.diagnose("unsigned", "unsigned requires an integer type specifier", source);
                    return {};
                }
            }
            return TypeSystem.constant(id, constant);
        }

        TypeId Declarations::boundArray(TypeId element, IntegerResult value, const types::ASTNode* expression,
                                        bool omitted) {
            Bound bound;
            if (omitted) {
                return TypeSystem.array(element);
            }
            if (value.state == ConstantState::Value && value.value > 0) {
                bound = {BoundKind::Constant, static_cast<uint64_t>(value.value)};
            }
            else {
                SymbolTable.diagnose("bound",
                                     value.state == ConstantState::Unavailable
                                         ? "array bound evaluator unavailable"
                                         : "array bound must be a positive integral constant",
                                     expression);
                return {};
            }
            auto id = TypeSystem.array(element, bound);
            if (!id.value) {
                SymbolTable.diagnose("array", "invalid array element type", expression);
            }
            return id;
        }

        bool Declarations::validate(TypeId id, DeclarationContext context, const types::ASTNode* source) {
            if (!id.value) {
                return false;
            }
            if (!TypeSystem.validDerived(id)) {
                SymbolTable.diagnose("derived", "invalid derived type or incomplete array element", source);
                return false;
            }
            auto Kind = TypeSystem.get(TypeSystem.unConst(id)).kind;
            if (Kind == TypeKind::Function || context.prototype || !context.storageRequired) {
                return true;
            }
            auto layout = TypeSystem.layout(id);
            if (!layout) {
                SymbolTable.diagnose("complete", layout.message, source);
                return false;
            }
            return true;
        }

        // Abstract classes exist only as base subobjects: no objects, by-value
        // parameters or by-value results of their type.
        bool Declarations::concrete(TypeId type, const types::ASTNode* source) {
            const auto& info = TypeSystem.get(TypeSystem.unConst(type));
            if (info.kind == TypeKind::Function) {
                bool valid = concrete(info.element, source);
                for (auto parameter : info.parameters) {
                    valid = concrete(parameter, source) && valid;
                }
                return valid;
            }
            if (TypeSystem.abstract(type)) {
                auto element = TypeSystem.unConst(type);
                while (TypeSystem.get(element).kind == TypeKind::Array) {
                    element = TypeSystem.unConst(TypeSystem.get(element).element);
                }
                SymbolTable.diagnose("abstract",
                                     "cannot use abstract class '" + TypeSystem.get(element).name + "' by value",
                                     source);
                return false;
            }
            return true;
        }

        SymbolId Declarations::object(Declaration NewDeclaration, DeclarationContext context) {
            NewDeclaration.type =
                context.parameter ? TypeSystem.parameterObject(NewDeclaration.type) : NewDeclaration.type;
            if (!validate(NewDeclaration.type, context, NewDeclaration.source) ||
                !concrete(NewDeclaration.type, NewDeclaration.source)) {
                return {};
            }
            auto kind = TypeSystem.get(TypeSystem.unConst(NewDeclaration.type)).kind;
            if (NewDeclaration.definition && kind == TypeKind::Function && NewDeclaration.kind == SymbolKind::Object) {
                // A declaration with no body is a function prototype; C passes Function for bodies.
                NewDeclaration.definition = false;
            }
            NewDeclaration.kind = kind == TypeKind::Function
                                      ? SymbolKind::Function
                                      : (context.parameter ? SymbolKind::Parameter : SymbolKind::Object);
            if (NewDeclaration.kind == SymbolKind::Function && NewDeclaration.linkage == Linkage::None) {
                NewDeclaration.linkage = NewDeclaration.isStatic ? Linkage::Internal : Linkage::External;
            }
            return SymbolTable.declare(std::move(NewDeclaration));
        }

        SymbolId Declarations::functionDefinition(Declaration NewDeclaration) {
            const auto& TypeInfo = TypeSystem.get(TypeSystem.unConst(NewDeclaration.type));
            if (TypeInfo.kind != TypeKind::Function) {
                SymbolTable.diagnose("function-body", "body attached to an object declarator", NewDeclaration.source);
                return {};
            }
            if (!TypeSystem.isVoid(TypeInfo.element) && !validate(TypeInfo.element, {}, NewDeclaration.source)) {
                return {};
            }
            for (auto parameter : TypeInfo.parameters) {
                if (!validate(parameter, {}, NewDeclaration.source)) {
                    return {};
                }
            }
            NewDeclaration.kind = SymbolKind::Function;
            NewDeclaration.definition = true;
            NewDeclaration.linkage = NewDeclaration.isStatic ? Linkage::Internal : Linkage::External;
            return SymbolTable.declare(std::move(NewDeclaration));
        }

        SymbolId Declarations::alias(std::string name, TypeId type, const types::ASTNode* source) {
            if (!validate(type, {false, false, false, false}, source)) {
                return {};
            }
            return SymbolTable.declare({std::move(name), SymbolKind::Alias, type, false, false, Linkage::None, source});
        }

        TypeId Declarations::aggregate(std::string name, TypeKind kind, const types::ASTNode* source) {
            if (!name.empty()) {
                auto old = SymbolTable.lookupCurrent(name);
                if (!old.empty()) {
                    if (old.size() == 1 && SymbolTable.get(old[0]).kind == SymbolKind::Type) {
                        auto type = SymbolTable.get(old[0]).type;
                        auto oldKind = TypeSystem.get(type).kind;
                        if (oldKind == kind || ((oldKind == TypeKind::Struct || oldKind == TypeKind::Class) &&
                            (kind == TypeKind::Struct || kind == TypeKind::Class))) {
                            return type;
                        }
                    }
                    SymbolTable.diagnose("aggregate-name", "conflicting aggregate name", source);
                    return {};
                }
            }
            auto type = TypeSystem.nominal(kind, name);
            if (!type.value) {
                return {};
            }
            if (!name.empty()) {
                SymbolTable.declare({std::move(name), SymbolKind::Type, type, false, false, Linkage::None, source});
            }
            return type;
        }

        bool Declarations::beginAggregate(TypeId id, const types::ASTNode* source) {
            if (TypeSystem.beginDefinition(id)) {
                return true;
            }
            SymbolTable.diagnose("aggregate-definition", "aggregate already defined or being defined", source);
            return false;
        }

        bool Declarations::member(TypeId owner, Member member) {
            auto type = member.type;
            const auto& TypeInfo = TypeSystem.get(TypeSystem.unConst(type));
            const auto& ownerName = TypeSystem.get(owner).name;
            if ((member.isConstructor && (ownerName.empty() || member.name != ownerName)) ||
                (member.isDestructor && (ownerName.empty() || member.name != "~" + ownerName))) {
                SymbolTable.diagnose("special-member-name", "special member name does not match its enclosing type",
                                     member.source);
                return false;
            }
            if (member.isStatic && member.isConstMethod) {
                SymbolTable.diagnose("static-method", "static methods have no const-qualified receiver", member.source);
                return false;
            }
            if (!member.isType && TypeInfo.kind == TypeKind::Reference) {
                // There are no constructor member-initializer lists to bind them.
                SymbolTable.diagnose("reference", "reference members are not supported", member.source);
                return false;
            }
            if (member.isConstructor && TypeInfo.kind == TypeKind::Function && !TypeInfo.parameters.empty() &&
                member.minimumArity <= 1 && TypeSystem.isReference(TypeInfo.parameters.front()) &&
                TypeSystem.unConst(TypeSystem.referenced(TypeInfo.parameters.front())) == TypeSystem.unConst(owner)) {
                // Record copies are memberwise IR copies; a user copy constructor would
                // be silently bypassed by them.
                SymbolTable.diagnose("copy-constructor", "user-defined copy constructors are not supported",
                                     member.source);
                return false;
            }
            if (member.isPure && (TypeSystem.get(owner).kind != TypeKind::Class || member.isStatic ||
                                  TypeInfo.kind != TypeKind::Function)) {
                SymbolTable.diagnose("pure", "only non-static class methods can be pure ('= 0')", member.source);
                return false;
            }
            bool field = !member.isType && !member.isStatic && TypeInfo.kind != TypeKind::Function;
            if (field && !validate(type, {false, true, false, true}, member.source)) {
                return false;
            }
            if (!member.isType && !concrete(type, member.source)) {
                return false;
            }
            if (!field && !validate(type, {false, true, true, false}, member.source)) {
                return false;
            }
            auto lifetimeType = TypeSystem.unConst(type);
            while (TypeSystem.get(lifetimeType).kind == TypeKind::Array) {
                lifetimeType = TypeSystem.unConst(TypeSystem.get(lifetimeType).element);
            }
            if (TypeSystem.get(owner).kind == TypeKind::Union && field && TypeSystem.isAggregate(lifetimeType) &&
                specialMembers(lifetimeType).nontrivial) {
                SymbolTable.diagnose("union-lifetime", "nontrivial union members are unsupported", member.source);
                return false;
            }
            if (member.isConstructor && (TypeInfo.kind != TypeKind::Function || !TypeSystem.isVoid(TypeInfo.element) ||
                member.isStatic || member.isConstMethod)) {
                SymbolTable.diagnose("constructor", "invalid constructor signature", member.source);
                return false;
            }
            if (member.isConstructor &&
                std::find(TypeInfo.parameters.begin(), TypeInfo.parameters.end(), owner) != TypeInfo.parameters.end()) {
                SymbolTable.diagnose("constructor-copy", "a constructor cannot take its own type by value",
                                     member.source);
                return false;
            }
            if (member.isDestructor &&
                (TypeInfo.kind != TypeKind::Function || !TypeSystem.isVoid(TypeInfo.element) ||
                    !TypeInfo.parameters.empty() || TypeInfo.variadic || member.isStatic || member.isConstMethod)) {
                SymbolTable.diagnose("destructor", "invalid destructor signature", member.source);
                return false;
            }
            if (!TypeSystem.addMember(owner, member)) {
                SymbolTable.diagnose("member", "duplicate or conflicting aggregate member", member.source);
                return false;
            }
            return true;
        }

        bool Declarations::base(TypeId owner, TypeId base, semantic::Access access, const types::ASTNode* source) {
            const auto OwnerKind = TypeSystem.get(owner).kind;
            const auto BaseKind = base.value ? TypeSystem.get(base).kind : TypeKind::TypeError;
            if (OwnerKind == TypeKind::Class && BaseKind == TypeKind::Struct) {
                SymbolTable.diagnose("base-kind", "a class cannot inherit from a struct", source);
                return false;
            }
            if (OwnerKind == TypeKind::Struct && BaseKind == TypeKind::Class) {
                SymbolTable.diagnose("base-kind", "a struct cannot inherit from a class", source);

                return false;
            }
            if (TypeSystem.setBase(owner, base, access)) {
                return true;
            }
            SymbolTable.diagnose("base", "invalid, incomplete, duplicate, or cyclic base", source);
            return false;
        }

        bool Declarations::finishAggregate(TypeId type, const types::ASTNode* source) {
            auto layout = TypeSystem.finishDefinition(type);
            if (layout) {
                if (TypeSystem.get(type).kind == TypeKind::Union && specialMembers(type).nontrivial) {
                    SymbolTable.diagnose("union-lifetime", "nontrivial union lifetimes are unsupported", source);
                    return false;
                }
                return !TypeSystem.polymorphic(type) || virtualTable(type);
            }
            SymbolTable.diagnose("layout", layout.message, source);
            return false;
        }

        // Builds the class vtable: the base's slots are inherited, a method with
        // the same name, parameter types and const-ness replaces its slot, and any
        // other non-static method appends one. Slot 0 is the class's destructor.
        bool Declarations::virtualTable(TypeId type) {
            const auto& info = TypeSystem.get(type);
            std::vector<VirtualSlot> table;
            if (info.base.value) {
                table = TypeSystem.get(TypeSystem.unConst(info.base)).vtable;
            }
            if (table.empty()) {
                table.push_back({});
            }
            table.front() = {type, 0, false};
            std::vector<std::pair<size_t, uint32_t>> slots;
            bool valid = true;
            for (size_t index = 0; index < info.members.size(); ++index) {
                const auto& member = info.members[index];
                const auto& signature = TypeSystem.get(TypeSystem.unConst(member.type));
                if (member.isStatic || member.isType || member.isConstructor || member.isDestructor ||
                    signature.kind != TypeKind::Function) {
                    continue;
                }
                uint32_t slot = 0;
                for (uint32_t candidate = 1; candidate < table.size() && !slot; ++candidate) {
                    const auto& entry = table[candidate];
                    if (entry.owner == type) {
                        continue; // overloads declared in this class never override each other
                    }
                    const auto& previous = TypeSystem.get(entry.owner).members.at(entry.member);
                    const auto& previousSignature = TypeSystem.get(TypeSystem.unConst(previous.type));
                    if (previous.name == member.name && previousSignature.parameters == signature.parameters &&
                        previousSignature.variadic == signature.variadic) {
                        slot = candidate;
                        if (previousSignature.element != signature.element) {
                            SymbolTable.diagnose("override",
                                                 "overriding method '" + member.name +
                                                     "' must have the same return type",
                                                 member.source);
                            valid = false;
                        }
                        if (previous.isConstMethod != member.isConstMethod) {
                            SymbolTable.diagnose("override",
                                                 "overriding method '" + member.name + "' must match its const-ness",
                                                 member.source);
                            valid = false;
                        }
                    }
                }
                if (!slot) {
                    slot = static_cast<uint32_t>(table.size());
                    table.push_back({});
                }
                table[slot] = {type, index, member.isPure};
                slots.push_back({index, slot});
            }
            TypeSystem.setVirtualTable(type, std::move(table), slots);
            return valid;
        }

        SymbolId Declarations::enumerator(std::string name, TypeId type, IntegerResult value,
                                          const types::ASTNode* source) {
            if (TypeSystem.get(type).kind != TypeKind::Enum || value.state != ConstantState::Value ||
                value.value < std::numeric_limits<int32_t>::min() ||
                value.value > std::numeric_limits<int32_t>::max()) {
                SymbolTable.diagnose("enumerator", "enumerator requires an available signed 32-bit constant", source);
                return {};
            }
            auto id = SymbolTable.declare(
                {std::move(name), SymbolKind::Enumerator, type, true, false, Linkage::None, source});
            if (id.value) {
                SymbolTable.setConstant(id, value.value);
            }
            return id;
        }

        IntegerResult Declarations::nextEnumerator(std::optional<int64_t> previous) const {
            if (!previous) {
                return {ConstantState::Value, 0};
            }
            if (*previous >= std::numeric_limits<int32_t>::max()) {
                return {ConstantState::Error, 0};
            }
            return {ConstantState::Value, *previous + 1};
        }

        SpecialMembers Declarations::specialMembers(TypeId id) const {
            id = TypeSystem.unConst(id);
            if (auto found = SpecialMemberCache.find(id.value); found != SpecialMemberCache.end()) {
                return found->second;
            }
            SpecialMembers result;
            const auto& TypeInfo = TypeSystem.get(TypeSystem.unConst(id));
            for (size_t Index = 0; Index < TypeInfo.members.size(); ++Index) {
                const auto& MemberInfo = TypeInfo.members[Index];
                if (MemberInfo.isConstructor) {
                    result.constructors.push_back(Index);
                }
                if (MemberInfo.isDestructor) {
                    result.destructor = Index;
                }
            }
            result.implicitDefaultConstructor = result.constructors.empty();
            result.implicitCopyConstructor = true; // explicit by-value copy constructors are invalid under this subset
            result.implicitDestructor = !result.destructor;
            result.nontrivial = !result.constructors.empty() || result.destructor.has_value();
            for (const auto& member : TypeInfo.members) {
                if (!member.isStatic && member.initializer) {
                    result.nontrivial = true;
                }
            }
            if (TypeInfo.completion == Completion::Complete) {
                if (TypeInfo.base.value) {
                    result.nontrivial = result.nontrivial || specialMembers(TypeInfo.base).nontrivial;
                }
                for (auto Index : TypeSystem.instanceMembers(id)) {
                    auto MemberInfo = TypeInfo.members[Index].type;
                    while (TypeSystem.get(TypeSystem.unConst(MemberInfo)).kind == TypeKind::Array) {
                        MemberInfo = TypeSystem.get(TypeSystem.unConst(MemberInfo)).element;
                    }
                    if (TypeSystem.isAggregate(MemberInfo)) {
                        result.nontrivial = result.nontrivial || specialMembers(MemberInfo).nontrivial;
                    }
                }
            }
            if (TypeInfo.completion == Completion::Complete) {
                SpecialMemberCache.emplace(id.value, result);
            }
            return result;
        }

        bool Declarations::accessible(TypeId naming, TypeId owner, size_t index, TypeId context) const {
            naming = TypeSystem.unConst(naming);
            owner = TypeSystem.unConst(owner);
            context = context.value ? TypeSystem.unConst(context) : TypeId{};
            // Inheritance path from the naming class down to the declaring class.
            std::vector<TypeId> path;
            for (auto current = naming; current.value; current = TypeSystem.get(current).base) {
                path.push_back(current);
                if (current == owner) {
                    break;
                }
            }
            if (path.empty() || path.back() != owner) {
                return false;
            }
            const auto related = [&](TypeId derived, TypeId base) {
                return derived == base || TypeSystem.derivesFrom(derived, base);
            };
            // Effective access of the member as a member of each class on the path,
            // computed outward from the declaring class. A private member of a base is
            // inaccessible (nullopt) in every class derived from it.
            std::vector<std::optional<semantic::Access>> levels(path.size());
            levels.back() = TypeSystem.get(owner).members.at(index).access;
            for (size_t i = path.size() - 1; i-- > 0;) {
                const auto inner = levels[i + 1];
                if (inner && *inner != semantic::Access::Private) {
                    levels[i] = std::max(*inner, TypeSystem.get(path[i]).baseAccess);
                }
            }
            // [class.access.base]: m named in N is accessible if its access as a member
            // of N permits the context, or if N's base B is accessible from the context
            // and m is accessible when named in B.
            const auto permits = [&](semantic::Access access, TypeId at) {
                switch (access) {
                case semantic::Access::Public:
                    return true;
                case semantic::Access::Private:
                    return context.value && context == at;
                case semantic::Access::Protected:
                    // Derived members may use protected members only through objects of
                    // their own class (or classes derived from it).
                    return context.value &&
                        (context == at || (TypeSystem.derivesFrom(context, at) && related(naming, context)));
                }
                return false;
            };
            for (size_t i = 0; i < path.size(); ++i) {
                if (levels[i] && permits(*levels[i], path[i])) {
                    return true;
                }
                if (i + 1 < path.size() && !permits(TypeSystem.get(path[i]).baseAccess, path[i])) {
                    return false; // the base subobject itself is not accessible here
                }
            }
            return false;
        }

        bool Declarations::checkAccess(TypeId naming, TypeId owner, size_t index, TypeId context,
                                       const types::ASTNode* source) {
            if (accessible(naming, owner, index, context)) {
                return true;
            }
            const auto& member = TypeSystem.get(TypeSystem.unConst(owner)).members.at(index);
            const char* level = member.access == semantic::Access::Private
                                    ? "private"
                                    : member.access == semantic::Access::Protected
                                    ? "protected"
                                    : "inaccessible";
            auto name = member.isConstructor
                            ? "constructor of '" + TypeSystem.get(TypeSystem.unConst(owner)).name + "'"
                            : "'" + member.name + "'";
            SymbolTable.diagnose("access", name + " is " + level + " within this context", source);
            return false;
        }

        Declarations::Access Declarations::memberAccess(TypeId receiver, const std::string& name, bool arrow,
                                                        bool staticContext, const types::ASTNode* source,
                                                        std::optional<TypeId> accessContext) {
            if (arrow) {
                auto TypeInfo = TypeSystem.get(TypeSystem.unConst(receiver));
                if (TypeInfo.kind != TypeKind::Pointer) {
                    SymbolTable.diagnose("arrow", "arrow requires a pointer", source);
                    return {};
                }
                receiver = TypeInfo.element;
            }
            if (!TypeSystem.isAggregate(receiver)) {
                SymbolTable.diagnose("receiver", "member receiver must be an aggregate", source);
                return {};
            }
            auto found = TypeSystem.lookupMember(receiver, name);
            if (found.indices.empty()) {
                SymbolTable.diagnose("missing-member", "no member named '" + name + "'", source);
                return {};
            }
            for (auto Index : found.indices) {
                const auto& MemberInfo = TypeSystem.get(found.owner).members[Index];
                if (MemberInfo.isType || MemberInfo.isConstructor || MemberInfo.isDestructor ||
                    (staticContext && !MemberInfo.isStatic)) {
                    SymbolTable.diagnose("member-context", "member is unavailable in this expression context", source);
                    return {};
                }
            }
            if (found.indices.size() != 1) {
                return {{}, found.owner, found.indices, false};
            }
            if (accessContext && !checkAccess(receiver, found.owner, found.indices[0], *accessContext, source)) {
                return {};
            }
            const auto& MemberInfo = TypeSystem.get(found.owner).members[found.indices[0]];
            bool function = TypeSystem.get(TypeSystem.unConst(MemberInfo.type)).kind == TypeKind::Function;
            auto type = (!MemberInfo.isStatic && !function)
                            ? TypeSystem.constant(MemberInfo.type, TypeSystem.isConst(receiver))
                            : MemberInfo.type;
            return {type, found.owner, found.indices, !function};
        }

        TypeId Declarations::thisType(TypeId currentClass, bool staticContext, bool constMethod,
                                      const types::ASTNode* source) {
            if (!TypeSystem.isAggregate(currentClass) || staticContext) {
                SymbolTable.diagnose("this", "this requires an instance member context", source);
                return {};
            }
            return TypeSystem.pointer(TypeSystem.constant(currentClass, constMethod));
        }
    }
}
