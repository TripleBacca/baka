#include "declarations.hh"

#include <algorithm>
#include <limits>

namespace baka {
    namespace semantic {
        Declarations::Declarations(Types& TypeSystem, Symbols& SymbolTable)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable) {}

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
                } else if (name == "short") {
                    id = TypeSystem.builtin(Builtin::UShort);
                } else if (name == "int") {
                    id = TypeSystem.builtin(Builtin::UInt);
                } else if (name == "long") {
                    id = TypeSystem.builtin(Builtin::ULong);
                } else {
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
            } else {
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

        SymbolId Declarations::object(Declaration NewDeclaration, DeclarationContext context) {
            NewDeclaration.type =
                context.parameter ? TypeSystem.parameterObject(NewDeclaration.type) : NewDeclaration.type;
            if (!validate(NewDeclaration.type, context, NewDeclaration.source)) {
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
            bool field = !member.isType && !member.isStatic && TypeInfo.kind != TypeKind::Function;
            if (field && !validate(type, {false, true, false, true}, member.source)) {
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

        bool Declarations::base(TypeId owner, TypeId base, const types::ASTNode* source) {
            if (TypeSystem.setBase(owner, base)) {
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
                return true;
            }
            SymbolTable.diagnose("layout", layout.message, source);
            return false;
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

        Declarations::Access Declarations::memberAccess(TypeId receiver, const std::string& name, bool arrow,
                                                        bool staticContext, const types::ASTNode* source) {
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
