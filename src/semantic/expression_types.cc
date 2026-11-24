#include "expressions.hh"

namespace baka {
    namespace semantic {
        bool ExpressionChecker::isInteger(TypeId type) const {
            return TypeSystem.isIntegral(type);
        }

        bool ExpressionChecker::isArithmetic(TypeId type) const {
            return static_cast<bool>(TypeSystem.arithmetic(type));
        }

        bool ExpressionChecker::isPointer(TypeId type) const {
            return TypeSystem.get(TypeSystem.unConst(type)).kind == TypeKind::Pointer;
        }

        bool ExpressionChecker::isScalar(TypeId type) const {
            return isArithmetic(type) || isPointer(type);
        }

        bool ExpressionChecker::isNullPointerConstant(const ExprResult& value) const {
            return value.nullPointerConstant ||
                   (value.constantExpression && value.constant.state == ConstantState::Value &&
                    value.constant.value == 0 && isInteger(value.type));
        }

        bool ExpressionChecker::isCompleteObject(TypeId type) const {
            auto layout = TypeSystem.layout(type);
            return static_cast<bool>(layout);
        }

        TypeId ExpressionChecker::decay(TypeId type) const {
            const auto& TypeInfo = TypeSystem.get(TypeSystem.unConst(type));
            if (TypeInfo.kind == TypeKind::Array) {
                return TypeSystem.pointer(TypeInfo.element);
            }
            if (TypeInfo.kind == TypeKind::Function) {
                return TypeSystem.pointer(type);
            }
            return type;
        }

        TypeId ExpressionChecker::promote(TypeId type) const {
            auto info = TypeSystem.arithmetic(type);
            if (!info) {
                return type;
            }
            if (TypeSystem.get(TypeSystem.unConst(type)).kind == TypeKind::Enum) {
                return TypeSystem.builtin(Builtin::Int);
            }
            if (info->rank < 3 || (info->bits < 32)) {
                return TypeSystem.builtin(Builtin::Int);
            }
            return TypeSystem.unConst(type);
        }

        // The common type of two arithmetic operands after promotion; diagnoses and returns an invalid id if
        // either is not arithmetic. Floating wins by rank; mixed signedness goes to the unsigned type unless
        // the signed one is strictly wider, and at equal width the unsigned counterpart of the signed type is
        // chosen.
        TypeId ExpressionChecker::usualArithmetic(TypeId left, TypeId right, const types::ASTNode* source) {
            left = promote(left);
            right = promote(right);
            if (left == right) {
                return left;
            }
            auto leftInfo = TypeSystem.arithmetic(left);
            auto rightInfo = TypeSystem.arithmetic(right);
            if (!leftInfo || !rightInfo) {
                SymbolTable.diagnose("conversion", "arithmetic operand required", source);
                return {};
            }
            if (leftInfo->floating || rightInfo->floating) {
                return (leftInfo->rank >= rightInfo->rank) ? left : right;
            }
            if (leftInfo->isSigned == rightInfo->isSigned) {
                return (leftInfo->rank >= rightInfo->rank) ? left : right;
            }
            auto unsignedType = leftInfo->isSigned ? right : left;
            auto signedType = leftInfo->isSigned ? left : right;
            auto unsignedInfo = TypeSystem.arithmetic(unsignedType);
            auto signedInfo = TypeSystem.arithmetic(signedType);
            if (unsignedInfo->rank >= signedInfo->rank) {
                return unsignedType;
            }
            if (signedInfo->bits > unsignedInfo->bits) {
                return signedType;
            }
            // Same width: unsigned corresponding to the signed type.
            for (unsigned Index = 0; Index <= static_cast<unsigned>(Builtin::Double); ++Index) {
                auto candidate = TypeSystem.builtin(static_cast<Builtin>(Index));
                auto info = TypeSystem.arithmetic(candidate);
                if (info && info->bits == signedInfo->bits && !info->isSigned && !info->floating) {
                    return candidate;
                }
            }
            return unsignedType;
        }

        bool ExpressionChecker::hasDestruction(TypeId type) const {
            type = TypeSystem.unConst(type);
            while (TypeSystem.get(type).kind == TypeKind::Array) {
                type = TypeSystem.unConst(TypeSystem.get(type).element);
            }
            if (!TypeSystem.isAggregate(type) || TypeSystem.get(type).completion != Completion::Complete) {
                return false;
            }
            if (DeclarationChecker.specialMembers(type).destructor) {
                return true;
            }
            for (const auto& subobject : TypeSystem.subobjects(type)) {
                if (hasDestruction(subobject.type)) {
                    return true;
                }
            }
            return false;
        }

        bool ExpressionChecker::bindsDirectly(const ExprResult& value, TypeId reference) const {
            const auto referent = TypeSystem.get(reference).element;
            const auto source = value.sourceType.value ? value.sourceType : value.type;
            if (value.error || !value.sourceLValue || !source.value) {
                return false;
            }
            const auto from = TypeSystem.unConst(source), to = TypeSystem.unConst(referent);
            if (from != to && !TypeSystem.derivesFrom(from, to)) {
                return false;
            }
            // T& cannot drop the const of a const object.
            const bool sourceConst = TypeSystem.isConst(source) ||
                                     (TypeSystem.get(from).kind == TypeKind::Array &&
                                      TypeSystem.isConst(TypeSystem.get(from).element));
            const bool targetConst = TypeSystem.isConst(referent) ||
                                     (TypeSystem.get(to).kind == TypeKind::Array &&
                                      TypeSystem.isConst(TypeSystem.get(to).element));
            return !sourceConst || targetConst;
        }

        bool ExpressionChecker::bindsReference(const ExprResult& value, TypeId reference, bool exactly) const {
            if (value.error || !TypeSystem.isReference(reference)) {
                return false;
            }
            const auto referent = TypeSystem.get(reference).element;
            if (bindsDirectly(value, reference)) {
                const auto source = value.sourceType.value ? value.sourceType : value.type;
                return !exactly || TypeSystem.unConst(source) == TypeSystem.unConst(referent);
            }
            // Only const T& binds a temporary holding the converted value. Temporaries
            // that need destruction would need lifetime extension, which is not modelled.
            if (exactly || !TypeSystem.isConst(referent) || hasDestruction(referent)) {
                return false;
            }
            return isAssignable(value.type, TypeSystem.unConst(referent), value);
        }

        // Whether a value of type `from` converts implicitly to `to`: reference binding, null pointer
        // constants, arithmetic and enum-to-integer conversions, and pointer conversions (to void*, to a base,
        // adding const but never dropping it). Aggregates only copy from the identical type, and arrays,
        // functions and void never receive.
        bool ExpressionChecker::isAssignable(TypeId from, TypeId to, const ExprResult& value) const {
            if (!from.value || !to.value) {
                return false;
            }
            if (TypeSystem.isReference(to)) {
                return bindsReference(value, to);
            }
            if (isNullPointerConstant(value) && isPointer(to)) {
                return true;
            }
            auto target = TypeSystem.unConst(to);
            auto source = TypeSystem.unConst(from);
            const auto& targetInfo = TypeSystem.get(target);
            if (targetInfo.kind == TypeKind::Array || targetInfo.kind == TypeKind::Function ||
                TypeSystem.isVoid(target)) {
                return false;
            }
            if (source == target) {
                return true;
            }
            if (isArithmetic(source) && isArithmetic(target)) {
                return true;
            }
            if (TypeSystem.get(source).kind == TypeKind::Enum && isInteger(target)) {
                return true;
            }
            if (isPointer(source) && isPointer(target)) {
                auto sourcePointee = TypeSystem.get(source).element;
                auto targetPointee = TypeSystem.get(target).element;
                auto sourceBase = TypeSystem.unConst(sourcePointee);
                auto targetBase = TypeSystem.unConst(targetPointee);
                if (TypeSystem.isConst(sourcePointee) && !TypeSystem.isConst(targetPointee)) {
                    return false;
                }
                if (TypeSystem.isVoid(targetBase)) {
                    return TypeSystem.get(sourceBase).kind != TypeKind::Function;
                }
                if (TypeSystem.isVoid(sourceBase)) {
                    return false;
                }
                if (sourceBase != targetBase && !TypeSystem.derivesFrom(sourceBase, targetBase)) {
                    return false;
                }
                // Qualification may be added, never removed.
                return !TypeSystem.isConst(sourcePointee) || TypeSystem.isConst(targetPointee);
            }
            if (TypeSystem.isAggregate(target) && source == target) {
                return isCompleteObject(target);
            }
            return false;
        }

        bool ExpressionChecker::isModifiableLValue(const ExprResult& value) const {
            if (value.error || value.category != ValueCategory::LValue || !value.type.value ||
                TypeSystem.recursivelyConst(value.type)) {
                return false;
            }
            auto kind = TypeSystem.get(TypeSystem.unConst(value.type)).kind;
            return kind != TypeKind::Array && kind != TypeKind::Function && !TypeSystem.isVoid(value.type);
        }
    }
}
