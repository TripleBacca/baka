#include "expressions.hh"

#include <algorithm>
#include <limits>
#include <cmath>

namespace baka {
    namespace semantic {
        namespace {
            bool IsComparison(types::ASTBinaryOp op) {
                switch (op) {
                case types::OP_EQ:
                case types::OP_NEQ:
                case types::OP_LT:
                case types::OP_GT:
                case types::OP_LE:
                case types::OP_GE:
                    return true;
                default:
                    return false;
                }
            }

            bool IsAssignment(types::ASTBinaryOp op) {
                switch (op) {
                case types::OP_ASSIGN:
                case types::OP_ADD_ASSIGN:
                case types::OP_SUB_ASSIGN:
                case types::OP_MUL_ASSIGN:
                case types::OP_DIV_ASSIGN:
                case types::OP_MOD_ASSIGN:
                case types::OP_AND_ASSIGN:
                case types::OP_OR_ASSIGN:
                case types::OP_XOR_ASSIGN:
                case types::OP_LSHIFT_ASSIGN:
                case types::OP_RSHIFT_ASSIGN:
                    return true;
                default:
                    return false;
                }
            }

            bool IsBitwise(types::ASTBinaryOp op) {
                return op == types::OP_BIT_AND || op == types::OP_BIT_XOR || op == types::OP_BIT_OR;
            }

            bool IsShift(types::ASTBinaryOp op) {
                return op == types::OP_LSHIFT || op == types::OP_RSHIFT;
            }

            bool IsArithmeticOperation(types::ASTBinaryOp op) {
                return op == types::OP_MUL || op == types::OP_DIV || op == types::OP_MOD || op == types::OP_ADD ||
                       op == types::OP_SUB;
            }

            const types::ConstantTNode<base::SLString>* StringLiteral(const types::ExpressionNode* expression) {
                while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                    if (list->isBraced() || list->expressions().size() != 1) break;
                    expression = list->expressions().front();
                }
                auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression);
                if (!primary) {
                    return nullptr;
                }
                const auto& payload = primary->getExpression();
                if (!std::holds_alternative<types::ConstantNode*>(payload)) {
                    return nullptr;
                }
                return dynamic_cast<const types::ConstantTNode<base::SLString>*>(
                    std::get<types::ConstantNode*>(payload));
            }

            int64_t ApplyBinary(types::ASTBinaryOp op, int64_t left, int64_t right, bool& valid) {
                valid = true;
                switch (op) {
                case types::OP_MUL: {
                    int64_t result;
                    valid = !__builtin_mul_overflow(left, right, &result);
                    return valid ? result : 0;
                }
                case types::OP_DIV:
                    if (!right || (left == std::numeric_limits<int64_t>::min() && right == -1)) {
                        valid = false;
                        return 0;
                    }
                    return left / right;
                case types::OP_MOD:
                    if (!right || (left == std::numeric_limits<int64_t>::min() && right == -1)) {
                        valid = false;
                        return 0;
                    }
                    return left % right;
                case types::OP_ADD: {
                    int64_t result; valid = !__builtin_add_overflow(left, right, &result); return valid ? result : 0;
                }
                case types::OP_SUB: {
                    int64_t result; valid = !__builtin_sub_overflow(left, right, &result); return valid ? result : 0;
                }
                case types::OP_LSHIFT:
                    if (right < 0 || right >= 64) {
                        valid = false;
                        return 0;
                    }
                    return static_cast<int64_t>(static_cast<uint64_t>(left) << right);
                case types::OP_RSHIFT:
                    if (right < 0 || right >= 64) {
                        valid = false;
                        return 0;
                    }
                    return left >> right;
                case types::OP_BIT_AND:
                    return left & right;
                case types::OP_BIT_XOR:
                    return left ^ right;
                case types::OP_BIT_OR:
                    return left | right;
                case types::OP_LOGICAL_AND:
                    return left && right;
                case types::OP_LOGICAL_OR:
                    return left || right;
                case types::OP_EQ:
                    return left == right;
                case types::OP_NEQ:
                    return left != right;
                case types::OP_LT:
                    return left < right;
                case types::OP_GT:
                    return left > right;
                case types::OP_LE:
                    return left <= right;
                case types::OP_GE:
                    return left >= right;
                default:
                    valid = false;
                    return 0;
                }
            }
        }

        ExpressionChecker::ExpressionChecker(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                                             ASTDeclarations& ASTResolver)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
              ASTResolver(ASTResolver) {}

        std::vector<SymbolId> ExpressionChecker::lookupIdentifier(const types::IdentifierNode* identifier) const {
            if (!identifier) return {};
            if (identifier->semanticInfo().Resolved) {
                std::vector<SymbolId> result;
                for (auto id : identifier->getBindings()) result.push_back({id});
                return result;
            }
            return SymbolTable.lookup(std::string(identifier->GetName()));
        }

        size_t ExpressionChecker::requiredArguments(SymbolId symbol, TypeId type) const {
            size_t required = TypeSystem.get(type).parameters.size();
            if (!symbol.value) return required; // function pointers never inherit defaults
            const auto& entry = SymbolTable.get(symbol);
            if (entry.kind != SymbolKind::Function && entry.kind != SymbolKind::BuiltinFunction) return required;
            while (required && required <= entry.defaultArguments.size() && entry.defaultArguments[required-1]) --required;
            return required;
        }

        void ExpressionChecker::setFunctionContext(TypeId currentClass, bool staticContext, bool constMethod) {
            CurrentClass = currentClass;
            InStaticContext = staticContext;
            InConstMethod = constMethod;
        }

        ExprResult ExpressionChecker::makeError() {
            ExprResult result;
            result.error = true;
            return result;
        }

        ExprResult ExpressionChecker::makeValue(TypeId type, ValueCategory category) {
            ExprResult result;
            result.type = type;
            result.category = category;
            return result;
        }

        ExprResult ExpressionChecker::makeConstant(TypeId type, int64_t value) {
            ExprResult result;
            result.type = type;
            result.constantExpression = true;
            result.constant = {ConstantState::Value, value};
            return result;
        }

        ExprResult ExpressionChecker::makeBool(bool value) {
            return makeConstant(TypeSystem.builtin(Builtin::Bool), value ? 1 : 0);
        }

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

        TypeId ExpressionChecker::usualArithmetic(TypeId left, TypeId right, const void* source) {
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

        bool ExpressionChecker::isAssignable(TypeId from, TypeId to, const ExprResult& value) const {
            if (!from.value || !to.value) {
                return false;
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
                if (TypeSystem.isConst(sourcePointee) && !TypeSystem.isConst(targetPointee)) return false;
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

        bool ExpressionChecker::checkScalarCondition(const ExprResult& value, const void* source) {
            if (value.error) {
                return false;
            }
            if (!isScalar(value.type)) {
                SymbolTable.diagnose("condition", "condition requires a scalar expression", source);
                return false;
            }
            return true;
        }

        ExprResult ExpressionChecker::checkConstant(const types::ConstantNode* node) {
            using namespace types;
            if (auto* current = dynamic_cast<const ConstantTNode<int>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::Int), current->getValue());
            }
            if (auto* current = dynamic_cast<const ConstantTNode<char>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::Char), current->getValue());
            }
            if (auto* current = dynamic_cast<const ConstantTNode<long long>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::Long), current->getValue());
            }
            if (auto* current = dynamic_cast<const ConstantTNode<unsigned int>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::UInt), current->getValue());
            }
            if (auto* current = dynamic_cast<const ConstantTNode<unsigned long long>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::ULong), static_cast<int64_t>(current->getValue()));
            }
            if (auto* current = dynamic_cast<const ConstantTNode<bool>*>(node)) {
                return makeConstant(TypeSystem.builtin(Builtin::Bool), current->getValue() ? 1 : 0);
            }
            if (auto* current = dynamic_cast<const ConstantTNode<float>*>(node)) {
                ExprResult result = makeValue(TypeSystem.builtin(Builtin::Float));
                result.constantExpression = true; // arithmetic constant, not an integral value
                result.floatingConstant = current->getValue();
                result.constant = {ConstantState::Nonconstant, 0};
                (void)current;
                return result;
            }
            if (auto* current = dynamic_cast<const ConstantTNode<double>*>(node)) {
                ExprResult result = makeValue(TypeSystem.builtin(Builtin::Double));
                result.constantExpression = true; // arithmetic constant, not an integral value
                result.floatingConstant = current->getValue();
                result.constant = {ConstantState::Nonconstant, 0};
                (void)current;
                return result;
            }
            if (auto* current = dynamic_cast<const ConstantTNode<std::nullptr_t>*>(node)) {
                auto result = makeValue(TypeSystem.pointer(TypeSystem.builtin(Builtin::Void)));
                result.nullPointerConstant = true;
                result.constantExpression = true;
                result.constant = {ConstantState::Value, 0};
                (void)current;
                return result;
            }
            if (auto* current = dynamic_cast<const ConstantTNode<base::SLString>*>(node)) {
                auto length = current->getValue().size();
                auto element = TypeSystem.constant(TypeSystem.builtin(Builtin::Char));
                auto type = TypeSystem.array(element, {BoundKind::Constant, length + 1});
                auto result = makeValue(type, ValueCategory::LValue);
                result.constant = {ConstantState::Nonconstant, 0};
                return result;
            }
            if (auto* current = dynamic_cast<const ConstantTNode<std::string>*>(node)) {
                auto length = current->getValue().size();
                auto element = TypeSystem.constant(TypeSystem.builtin(Builtin::Char));
                return makeValue(TypeSystem.array(element, {BoundKind::Constant, length + 1}),
                                 ValueCategory::LValue);
            }
            SymbolTable.diagnose("literal", "unsupported literal type", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkPrimary(const types::PrimaryExpressionNode* node) {
            const auto& payload = node->getExpression();
            if (std::holds_alternative<types::ConstantNode*>(payload)) {
                auto* constant = std::get<types::ConstantNode*>(payload);
                auto result = constant ? checkConstant(constant) : makeError();
                if (Annotating && constant) {
                    auto& metadata = constant->semanticInfo();
                    metadata.Type = result.type.value; metadata.Annotated = true; metadata.Error = result.error;
                }
                return result;
            }
            if (std::holds_alternative<types::ThisNode*>(payload)) {
                if (!CurrentClass.value || InStaticContext) {
                    SymbolTable.diagnose("this", "'this' requires an instance member context", node);
                    return makeError();
                }
                auto type = TypeSystem.pointer(TypeSystem.constant(CurrentClass, InConstMethod));
                if (Annotating) {
                    auto* self = std::get<types::ThisNode*>(payload);
                    self->semanticInfo().Type = type.value;
                    self->semanticInfo().Annotated = true;
                }
                return makeValue(type);
            }
            auto* identifier = std::get<types::IdentifierNode*>(payload);
            if (!identifier) {
                return makeError();
            }
            auto found = lookupIdentifier(identifier);
            if (found.empty()) {
                // Unqualified member names in methods resolve against the enclosing class.
                if (CurrentClass.value) {
                    auto access = DeclarationChecker.memberAccess(CurrentClass, std::string(identifier->GetName()),
                                                                  false, InStaticContext, identifier);
                    if (access.type.value) {
                        if (access.candidates.size() > 1) {
                            SymbolTable.diagnose("overload", "overload set requires a call", identifier);
                            return makeError();
                        }
                        auto result = makeValue(access.type, ValueCategory::LValue);
                        if (access.candidates.size() == 1) {
                            const auto& entry = TypeSystem.get(access.owner).members[access.candidates.front()];
                            result.boundInstanceFunction = !entry.isStatic && TypeSystem.get(TypeSystem.unConst(entry.type)).kind == TypeKind::Function;
                        }
                        return result;
                    }
                }
                SymbolTable.diagnose("undeclared", "undeclared name '" + std::string(identifier->GetName()) + "'",
                                     identifier);
                return makeError();
            }
            if (found.size() != 1) {
                SymbolTable.diagnose("overload", "overload set requires a call", identifier);
                return makeError();
            }
            const auto& symbol = SymbolTable.get(found.front());
            if (Annotating && CurrentClass.value && SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate) {
                auto member = TypeSystem.lookupMember(CurrentClass, symbol.name);
                for (auto index : member.indices) if (TypeSystem.get(member.owner).members[index].symbol == found.front()) {
                    auto& metadata = const_cast<types::PrimaryExpressionNode*>(node)->semanticInfo();
                    metadata.MemberOwner = member.owner.value;
                    metadata.MemberIndex = static_cast<uint32_t>(index + 1);
                }
            }
            if (Annotating) {
                identifier->semanticInfo().Type = symbol.type.value;
                identifier->semanticInfo().Annotated = true;
            }
            if (SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate && !symbol.isStatic && InStaticContext) {
                SymbolTable.diagnose("member-context", "instance member requires an object", identifier);
                return makeError();
            }
            if (symbol.kind == SymbolKind::Type || symbol.kind == SymbolKind::Alias) {
                SymbolTable.diagnose("value", "type name used as a value", identifier);
                return makeError();
            }
            if (symbol.kind == SymbolKind::Enumerator ||
                (symbol.constant && TypeSystem.isConst(symbol.type) && isInteger(symbol.type))) {
                auto result = makeValue(symbol.type, symbol.kind == SymbolKind::Enumerator ? ValueCategory::RValue : ValueCategory::LValue);
                result.constantExpression = true;
                result.constant = {ConstantState::Value, symbol.constant.value_or(0)};
                return result;
            }
            if (symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::BuiltinFunction) {
                auto result = makeValue(symbol.type);
                result.boundInstanceFunction = SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate && !symbol.isStatic;
                return result;
            }
            auto constantObject = ConstantObjects.find(found.front().value);
            if (constantObject != ConstantObjects.end()) {
                auto result = makeValue(symbol.type, ValueCategory::LValue);
                result.constantExpression = true;
                result.constant = {ConstantState::Value, constantObject->second};
                return result;
            }
            return makeValue(symbol.type, ValueCategory::LValue);
        }

        ExprResult ExpressionChecker::checkUnary(const types::FactorNode* node) {
            auto op = node->getUnaryOp();
            auto* operandNode = node->getOperand();
            if (!operandNode) {
                return makeError();
            }
            if (op == types::ASTUnaryOp::OP_ADDR_OF) {
                auto operand = check(operandNode, false);
                if (operand.error) {
                    return operand;
                }
                if (operand.boundInstanceFunction) {
                    SymbolTable.diagnose("member-function", "instance member function cannot form an ordinary function pointer", node);
                    return makeError();
                }
                auto kind = TypeSystem.get(TypeSystem.unConst(operand.type)).kind;
                // Address-of suppresses array-to-pointer decay: &a points to
                // the entire array, preserving its bounds and element qualifiers.
                if (kind == TypeKind::Function) {
                    return makeValue(TypeSystem.pointer(operand.type));
                }
                if (operand.category != ValueCategory::LValue) {
                    SymbolTable.diagnose("address", "address-of requires an lvalue", node);
                    return makeError();
                }
                return makeValue(TypeSystem.pointer(operand.type));
            }
            auto operand = check(operandNode, true);
            if (operand.error) {
                return operand;
            }
            switch (op) {
            case types::ASTUnaryOp::OP_ADDR_OF:
                return makeError();
            case types::ASTUnaryOp::OP_DEREF: {
                if (!isPointer(operand.type)) {
                    SymbolTable.diagnose("dereference", "dereference requires a pointer", node);
                    return makeError();
                }
                auto element = TypeSystem.get(TypeSystem.unConst(operand.type)).element;
                // Dereference designates the pointee; it is not a condition.
                // Records and arrays are valid lvalues, and function pointers
                // designate functions. Keep pointee qualifiers here; value
                // conversion, completeness and condition checks belong to use
                // sites. Void is the exception: it designates no object/function.
                if (TypeSystem.isVoid(element)) {
                    SymbolTable.diagnose("dereference", "cannot dereference a pointer to void", node);
                    return makeError();
                }
                return makeValue(element, ValueCategory::LValue);
            }
            case types::ASTUnaryOp::OP_INC:
            case types::ASTUnaryOp::OP_DEC: {
                auto raw = check(operandNode, false);
                if (raw.error) {
                    return raw;
                }
                if (!isModifiableLValue(raw)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a modifiable lvalue", node);
                    return makeError();
                }
                if (!isScalar(raw.type)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a scalar", node);
                    return makeError();
                }
                if (isPointer(raw.type) && !isCompleteObject(TypeSystem.get(TypeSystem.unConst(raw.type)).element)) {
                    SymbolTable.diagnose("mutation", "pointer increment requires a complete object pointee", node);
                    return makeError();
                }
                raw.category = ValueCategory::LValue;
                raw.constantExpression = false;
                return raw;
            }
            case types::ASTUnaryOp::OP_LOGICAL_NOT: {
                if (!checkScalarCondition(operand, node)) {
                    return makeError();
                }
                if (operand.constantExpression && operand.constant.state == ConstantState::Value) {
                    return makeBool(!operand.constant.value);
                }
                return makeValue(TypeSystem.builtin(Builtin::Bool));
            }
            case types::ASTUnaryOp::OP_TILDE: {
                if (!isInteger(operand.type)) {
                    SymbolTable.diagnose("operator", "bitwise complement requires an integer", node);
                    return makeError();
                }
                auto type = promote(operand.type);
                if (operand.constantExpression && operand.constant.state == ConstantState::Value) {
                    return makeConstant(type, ~operand.constant.value);
                }
                return makeValue(type);
            }
            case types::ASTUnaryOp::OP_UNARY_PLUS:
            case types::ASTUnaryOp::OP_UNARY_MINUS: {
                if (!isArithmetic(operand.type)) {
                    SymbolTable.diagnose("operator", "unary sign requires an arithmetic operand", node);
                    return makeError();
                }
                auto type = promote(operand.type);
                if (operand.constantExpression && operand.constant.state == ConstantState::Value) {
                    if (op == types::ASTUnaryOp::OP_UNARY_MINUS && operand.constant.value == std::numeric_limits<int64_t>::min())
                        return makeValue(type);
                    return makeConstant(type, op == types::ASTUnaryOp::OP_UNARY_MINUS ? -operand.constant.value
                                                                                      : operand.constant.value);
                }
                auto result = makeValue(type);
                result.constantExpression = operand.constantExpression;
                if (operand.floatingConstant) result.floatingConstant =
                    op == types::ASTUnaryOp::OP_UNARY_MINUS ? -*operand.floatingConstant : *operand.floatingConstant;
                return result;
            }
            case types::ASTUnaryOp::OP_SIZEOF:
                SymbolTable.diagnose("sizeof", "malformed sizeof operand", node);
                return makeError();
            case types::ASTUnaryOp::OP_NOOP:
                return operand;
            }
            return makeError();
        }

        ExprResult ExpressionChecker::checkAssignment(const types::BinaryExpressionNode* node) {
            auto left = check(node->getLeft(), false);
            auto right = check(node->getRight(), true);
            if (left.error || right.error) {
                return makeError();
            }
            if (!isModifiableLValue(left)) {
                SymbolTable.diagnose("assignment", "assignment target is not a modifiable lvalue", node);
                return makeError();
            }
            auto op = node->getBinaryOp();
            if (op == types::OP_ASSIGN) {
                if (!isAssignable(right.type, left.type, right)) {
                    SymbolTable.diagnose("conversion", "incompatible assignment conversion", node);
                    return makeError();
                }
            } else {
                auto underlying = op;
                switch (op) {
                case types::OP_ADD_ASSIGN: underlying = types::OP_ADD; break;
                case types::OP_SUB_ASSIGN: underlying = types::OP_SUB; break;
                case types::OP_MUL_ASSIGN: underlying = types::OP_MUL; break;
                case types::OP_DIV_ASSIGN: underlying = types::OP_DIV; break;
                case types::OP_MOD_ASSIGN: underlying = types::OP_MOD; break;
                case types::OP_AND_ASSIGN: underlying = types::OP_BIT_AND; break;
                case types::OP_OR_ASSIGN: underlying = types::OP_BIT_OR; break;
                case types::OP_XOR_ASSIGN: underlying = types::OP_BIT_XOR; break;
                case types::OP_LSHIFT_ASSIGN: underlying = types::OP_LSHIFT; break;
                case types::OP_RSHIFT_ASSIGN: underlying = types::OP_RSHIFT; break;
                default: break;
                }
                bool pointerArithmetic = isPointer(left.type) && (underlying == types::OP_ADD || underlying == types::OP_SUB) &&
                                         isInteger(right.type);
                if (pointerArithmetic) {
                    // valid pointer +/-= integer
                } else if (IsShift(underlying)) {
                    if (!isInteger(left.type) || !isInteger(right.type)) {
                        SymbolTable.diagnose("operator", "shift requires integer operands", node);
                        return makeError();
                    }
                } else if (IsBitwise(underlying)) {
                    if (!isInteger(left.type) || !isInteger(right.type)) {
                        SymbolTable.diagnose("operator", "bitwise operator requires integers", node);
                        return makeError();
                    }
                } else if (!isArithmetic(left.type) || !isArithmetic(right.type)) {
                    SymbolTable.diagnose("operator", "compound assignment requires arithmetic operands", node);
                    return makeError();
                }
            }
            auto result = left;
            result.category = ValueCategory::LValue;
            result.constantExpression = false;
            return result;
        }

        ExprResult ExpressionChecker::checkBinary(const types::BinaryExpressionNode* node) {
            auto op = node->getBinaryOp();
            if (IsAssignment(op)) {
                return checkAssignment(node);
            }
            if (op == types::OP_COMMA) {
                auto left = check(node->getLeft(), true);
                (void)left;
                auto result = check(node->getRight(), true);
                result.constantExpression = false;
                result.constant = {};
                return result;
            }
            if (op == types::OP_DOT || op == types::OP_ARROW) {
                SymbolTable.diagnose("member", "member access requires an identifier", node);
                return makeError();
            }
            auto left = check(node->getLeft(), true);
            if (left.error) {
                return makeError();
            }
            if (op == types::OP_LOGICAL_AND || op == types::OP_LOGICAL_OR) {
                if (!checkScalarCondition(left, node)) {
                    return makeError();
                }
                bool definite = left.constantExpression && left.constant.state == ConstantState::Value;
                bool leftTruth = definite && left.constant.value != 0;
                bool decisive = definite &&
                                ((op == types::OP_LOGICAL_AND && !leftTruth) ||
                                 (op == types::OP_LOGICAL_OR && leftTruth));
                auto right = check(node->getRight(), true);
                if (right.error) {
                    return makeError();
                }
                if (!checkScalarCondition(right, node)) {
                    return makeError();
                }
                if (decisive) return makeBool(leftTruth);
                if (left.constantExpression && left.constant.state == ConstantState::Value &&
                    right.constantExpression && right.constant.state == ConstantState::Value) {
                    return makeBool(right.constant.value != 0);
                }
                return makeValue(TypeSystem.builtin(Builtin::Bool));
            }
            auto right = check(node->getRight(), true);
            if (right.error) {
                return makeError();
            }
            if (IsComparison(op)) {
                bool equality = op == types::OP_EQ || op == types::OP_NEQ;
                if (isArithmetic(left.type) && isArithmetic(right.type)) {
                    if (left.constantExpression && right.constantExpression && left.constant.state == ConstantState::Value &&
                        right.constant.state == ConstantState::Value) {
                        bool valid = true;
                        auto value = ApplyBinary(op, left.constant.value, right.constant.value, valid);
                        if (valid) {
                            return makeBool(value != 0);
                        }
                    }
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(left.type) && isPointer(right.type)) {
                    auto leftPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(left.type)).element);
                    auto rightPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(right.type)).element);
                    bool compatible = leftPointee == rightPointee || TypeSystem.isVoid(leftPointee) ||
                                      TypeSystem.isVoid(rightPointee);
                    if (!compatible) {
                        SymbolTable.diagnose("comparison", "incompatible pointer comparison", node);
                        return makeError();
                    }
                    if (!equality) {
                        auto leftKind = TypeSystem.get(leftPointee).kind;
                        auto rightKind = TypeSystem.get(rightPointee).kind;
                        if (leftKind == TypeKind::Function || rightKind == TypeKind::Function ||
                            TypeSystem.isVoid(leftPointee) || TypeSystem.isVoid(rightPointee)) {
                            SymbolTable.diagnose("comparison", "ordering comparison requires object pointers", node);
                            return makeError();
                        }
                    }
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(left.type) && isNullPointerConstant(right)) {
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                if (isPointer(right.type) && isNullPointerConstant(left)) {
                    return makeValue(TypeSystem.builtin(Builtin::Bool));
                }
                SymbolTable.diagnose("comparison", "incompatible comparison operands", node);
                return makeError();
            }
            if (IsShift(op)) {
                if (!isInteger(left.type) || !isInteger(right.type)) {
                    SymbolTable.diagnose("operator", "shift requires integer operands", node);
                    return makeError();
                }
                auto type = promote(left.type);
                if (left.constantExpression && right.constantExpression &&
                    left.constant.state == ConstantState::Value &&
                    right.constant.state == ConstantState::Value) {
                    bool valid = true;
                    auto value = ApplyBinary(op, left.constant.value, right.constant.value, valid);
                    if (valid) {
                        return makeConstant(type, value);
                    }
                }
                return makeValue(type);
            }
            if (IsBitwise(op)) {
                if (!isInteger(left.type) || !isInteger(right.type)) {
                    SymbolTable.diagnose("operator", "bitwise operator requires integers", node);
                    return makeError();
                }
                auto type = usualArithmetic(left.type, right.type, node);
                if (!type.value) {
                    return makeError();
                }
                if (left.constantExpression && right.constantExpression &&
                    left.constant.state == ConstantState::Value &&
                    right.constant.state == ConstantState::Value) {
                    bool valid = true;
                    auto value = ApplyBinary(op, left.constant.value, right.constant.value, valid);
                    if (valid) {
                        return makeConstant(type, value);
                    }
                }
                return makeValue(type);
            }
            if (IsArithmeticOperation(op)) {
                if (isPointer(left.type) || isPointer(right.type)) {
                    if ((op == types::OP_ADD || op == types::OP_SUB)) {
                        if (isPointer(left.type) && isInteger(right.type)) {
                            if (!isCompleteObject(TypeSystem.get(TypeSystem.unConst(left.type)).element)) {
                                SymbolTable.diagnose("pointer", "pointer arithmetic requires a complete object type", node);
                                return makeError();
                            }
                            return makeValue(left.type);
                        }
                        if (op == types::OP_ADD && isInteger(left.type) && isPointer(right.type)) {
                            if (!isCompleteObject(TypeSystem.get(TypeSystem.unConst(right.type)).element)) {
                                SymbolTable.diagnose("pointer", "pointer arithmetic requires a complete object type", node);
                                return makeError();
                            }
                            return makeValue(right.type);
                        }
                        if (op == types::OP_SUB && isPointer(left.type) && isPointer(right.type)) {
                            auto leftPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(left.type)).element);
                            auto rightPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(right.type)).element);
                            if (leftPointee != rightPointee || !isCompleteObject(leftPointee)) {
                                SymbolTable.diagnose("pointer", "pointer difference requires compatible pointers", node);
                                return makeError();
                            }
                            return makeValue(TypeSystem.builtin(Builtin::Long));
                        }
                    }
                    SymbolTable.diagnose("operator", "invalid pointer arithmetic", node);
                    return makeError();
                }
                if (!isArithmetic(left.type) || !isArithmetic(right.type)) {
                    SymbolTable.diagnose("operator", "arithmetic operator requires arithmetic operands", node);
                    return makeError();
                }
                if (op == types::OP_MOD && (!isInteger(left.type) || !isInteger(right.type))) {
                    SymbolTable.diagnose("operator", "remainder requires integer operands", node);
                    return makeError();
                }
                auto type = usualArithmetic(left.type, right.type, node);
                if (!type.value) {
                    return makeError();
                }
                if (left.constantExpression && right.constantExpression &&
                    left.constant.state == ConstantState::Value &&
                    right.constant.state == ConstantState::Value) {
                    bool valid = true;
                    auto value = ApplyBinary(op, left.constant.value, right.constant.value, valid);
                    if (valid) {
                        return makeConstant(type, value);
                    }
                    // Invalid evaluation is rejected by required constant contexts;
                    // an ordinary/unevaluated expression still has an arithmetic type.
                    return makeValue(type);
                }
                ExprResult result = makeValue(type);
                result.constantExpression = left.constantExpression && right.constantExpression;
                if (result.constantExpression && TypeSystem.arithmetic(type)->floating) {
                    auto numeric = [](const ExprResult& value) -> std::optional<long double> {
                        if (value.floatingConstant) return value.floatingConstant;
                        if (value.constant.state == ConstantState::Value) return value.constant.value;
                        return {};
                    };
                    auto lhs = numeric(left), rhs = numeric(right);
                    if (lhs && rhs) {
                        if (op == types::OP_ADD) result.floatingConstant = *lhs + *rhs;
                        if (op == types::OP_SUB) result.floatingConstant = *lhs - *rhs;
                        if (op == types::OP_MUL) result.floatingConstant = *lhs * *rhs;
                        if (op == types::OP_DIV && *rhs != 0) result.floatingConstant = *lhs / *rhs;
                        if (op == types::OP_DIV && *rhs == 0) result.constantExpression = false;
                    }
                }
                result.constant = {ConstantState::Nonconstant, 0};
                return result;
            }
            SymbolTable.diagnose("operator", "unsupported binary operator", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkIndex(const types::IndexPostfixExpr* node) {
            auto base = check(node->getBase(), true);
            auto index = check(node->getIndex(), true);
            if (base.error || index.error) {
                return makeError();
            }
            if (!isPointer(base.type)) {
                SymbolTable.diagnose("index", "subscript requires a pointer or array", node);
                return makeError();
            }
            if (!isInteger(index.type)) {
                SymbolTable.diagnose("index", "subscript index must be integral", node);
                return makeError();
            }
            auto element = TypeSystem.get(TypeSystem.unConst(base.type)).element;
            return makeValue(element, ValueCategory::LValue);
        }

        ExprResult ExpressionChecker::checkMember(const types::ExpressionNode* baseNode,
                                                  const types::IdentifierNode* member, bool arrow,
                                                  const void* source) {
            auto base = check(baseNode, false);
            if (base.error) {
                return makeError();
            }
            if (base.category == ValueCategory::RValue) {
                base.type = decay(base.type);
            }
            auto access = DeclarationChecker.memberAccess(base.type, std::string(member->GetName()), arrow,
                                                          false, source);
            if (!access.type.value) {
                return makeError();
            }
            if (access.candidates.size() > 1) {
                SymbolTable.diagnose("overload", "overload set requires a call", source);
                return makeError();
            }
            if (Annotating && access.candidates.size() == 1) {
                auto& metadata = const_cast<types::ASTNode*>(static_cast<const types::ASTNode*>(source))->semanticInfo();
                metadata.MemberOwner = access.owner.value;
                metadata.MemberIndex = static_cast<uint32_t>(access.candidates.front() + 1);
                const auto& selected = TypeSystem.get(access.owner).members[access.candidates.front()];
                metadata.Symbol = selected.symbol.value;
                if (selected.symbol.value) {
                    auto* identifier = const_cast<types::IdentifierNode*>(member);
                    identifier->resolve({selected.symbol.value}, selected.name + "." + std::to_string(selected.symbol.value));
                    identifier->semanticInfo().Type = access.type.value;
                }
            }
            auto category = (base.category == ValueCategory::LValue || arrow) ? ValueCategory::LValue
                                                                              : ValueCategory::RValue;
            auto result = makeValue(access.type, category);
            if (access.candidates.size() == 1) {
                const auto& entry = TypeSystem.get(access.owner).members[access.candidates.front()];
                result.boundInstanceFunction = !entry.isStatic && TypeSystem.get(TypeSystem.unConst(entry.type)).kind == TypeKind::Function;
            }
            return result;
        }

        std::vector<ExprResult> ExpressionChecker::argumentList(const types::ExpressionNode* list) {
            std::vector<ExprResult> result;
            if (!list) {
                return result;
            }
            auto* comma = dynamic_cast<const types::CommaExpressionNode*>(list);
            if (!comma) {
                result.push_back(check(list, true));
                return result;
            }
            for (auto* expression : comma->getExpressions()) {
                result.push_back(check(expression, true));
            }
            return result;
        }

        std::vector<SymbolId> ExpressionChecker::resolveCallee(const types::ExpressionNode* callee,
                                                               const void* source) {
            std::vector<SymbolId> result;
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(callee);
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                if (identifier) {
                    result = lookupIdentifier(identifier);
                }
            }
            (void)source;
            return result;
        }

        std::vector<ExpressionChecker::FunctionCandidate> ExpressionChecker::functionCandidates(const types::ExpressionNode* callee,
                                                                  const void* source) {
            std::vector<FunctionCandidate> candidates;
            while (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(callee)) {
                if (comma->getExpressions().size() != 1) break;
                callee = comma->getExpressions().front();
            }
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(callee);
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                if (!identifier) {
                    return candidates;
                }
                for (auto symbolId : lookupIdentifier(identifier)) {
                    const auto& symbol = SymbolTable.get(symbolId);
                    auto type = symbol.type;
                    if (symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::BuiltinFunction) {
                        if (InStaticContext && !symbol.isStatic && SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate) {
                            SymbolTable.diagnose("member-context", "instance member requires an object", identifier);
                            continue;
                        }
                        TypeId owner{}; uint32_t memberIndex = 0;
                        if (CurrentClass.value && SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate) {
                            auto member = TypeSystem.lookupMember(CurrentClass, symbol.name);
                            for (auto index : member.indices) if (TypeSystem.get(member.owner).members[index].symbol == symbolId) {
                                owner = member.owner; memberIndex = static_cast<uint32_t>(index + 1);
                            }
                        }
                        candidates.push_back({type, symbolId, requiredArguments(symbolId, type), owner, memberIndex});
                        continue;
                    }
                    auto kind = TypeSystem.get(TypeSystem.unConst(type)).kind;
                    if (kind == TypeKind::Function) {
                        candidates.push_back({type, symbolId, requiredArguments(symbolId, type)});
                    } else if (kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.get(TypeSystem.unConst(type)).element;
                        if (TypeSystem.get(TypeSystem.unConst(pointee)).kind == TypeKind::Function) {
                            auto raw = check(callee, false);
                            if (!raw.error) candidates.push_back({pointee, {}, TypeSystem.get(pointee).parameters.size(),
                                {callee->semanticInfo().MemberOwner}, callee->semanticInfo().MemberIndex});
                        }
                    }
                }
                return candidates;
            }
            if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(callee)) {
                auto base = check(member->getBase(), false);
                if (base.error) {
                    return candidates;
                }
                auto access = DeclarationChecker.memberAccess(base.type, std::string(member->getMember()->GetName()),
                                                              false, false, source);
                for (auto index : access.candidates) {
                    const auto& entry = TypeSystem.get(access.owner).members[index];
                    auto type = TypeSystem.unConst(entry.type);
                    if (TypeSystem.get(type).kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.unConst(TypeSystem.get(type).element);
                        if (TypeSystem.get(pointee).kind == TypeKind::Function && !check(callee, false).error)
                            candidates.push_back({pointee, {}, TypeSystem.get(pointee).parameters.size(), access.owner,
                                                  static_cast<uint32_t>(index + 1)});
                    } else if (TypeSystem.get(type).kind == TypeKind::Function) {
                        candidates.push_back({type, entry.symbol,
                            entry.symbol.value ? requiredArguments(entry.symbol, type) : entry.minimumArity,
                            access.owner, static_cast<uint32_t>(index + 1)});
                    }
                }
                return candidates;
            }
            if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(callee)) {
                auto base = check(arrow->getBase(), true);
                if (base.error) {
                    return candidates;
                }
                auto access = DeclarationChecker.memberAccess(base.type, std::string(arrow->getMember()->GetName()),
                                                              true, false, source);
                for (auto index : access.candidates) {
                    const auto& entry = TypeSystem.get(access.owner).members[index];
                    auto type = TypeSystem.unConst(entry.type);
                    if (TypeSystem.get(type).kind == TypeKind::Pointer) {
                        auto pointee = TypeSystem.unConst(TypeSystem.get(type).element);
                        if (TypeSystem.get(pointee).kind == TypeKind::Function && !check(callee, false).error)
                            candidates.push_back({pointee, {}, TypeSystem.get(pointee).parameters.size(), access.owner,
                                                  static_cast<uint32_t>(index + 1)});
                    } else if (TypeSystem.get(type).kind == TypeKind::Function) {
                        candidates.push_back({type, entry.symbol,
                            entry.symbol.value ? requiredArguments(entry.symbol, type) : entry.minimumArity,
                            access.owner, static_cast<uint32_t>(index + 1)});
                    }
                }
                return candidates;
            }
            auto value = check(callee, true);
            if (!value.error) {
                auto kind = TypeSystem.get(TypeSystem.unConst(value.type)).kind;
                if (kind == TypeKind::Function) {
                    candidates.push_back({value.type, {}, TypeSystem.get(value.type).parameters.size()});
                } else if (kind == TypeKind::Pointer) {
                    auto pointee = TypeSystem.get(TypeSystem.unConst(value.type)).element;
                    if (TypeSystem.get(TypeSystem.unConst(pointee)).kind == TypeKind::Function) {
                        candidates.push_back({pointee, {}, TypeSystem.get(pointee).parameters.size()});
                    }
                }
            }
            return candidates;
        }

        ExprResult ExpressionChecker::checkCall(const types::FunctionCallPostfixExpr* node) {
            auto arguments = argumentList(node->getArguments());
            for (const auto& argument : arguments) {
                if (argument.error) {
                    return makeError();
                }
            }

            // Constructor call: T(args) where T names an aggregate type.
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(node->getCallee());
            if (primary && std::holds_alternative<types::IdentifierNode*>(primary->getExpression())) {
                auto* identifier = std::get<types::IdentifierNode*>(primary->getExpression());
                auto found = lookupIdentifier(identifier);
                if (found.size() == 1 && SymbolTable.get(found.front()).kind == SymbolKind::Type) {
                    auto type = SymbolTable.get(found.front()).type;
                    if (TypeSystem.isAggregate(type)) {
                        if (Annotating) {
                            auto& metadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                            metadata.Construction = true; metadata.MemberOwner = type.value;
                        }
                        auto special = DeclarationChecker.specialMembers(type);
                        if (special.constructors.empty()) {
                            if (!arguments.empty()) {
                                SymbolTable.diagnose("constructor", "aggregate has no matching constructor", node);
                                return makeError();
                            }
                        } else {
                            std::vector<FunctionCandidate> candidates;
                            for (auto index : special.constructors) {
                                const auto& constructor = TypeSystem.get(TypeSystem.unConst(type)).members[index];
                                candidates.push_back({constructor.type, {}, constructor.minimumArity, TypeSystem.unConst(type), static_cast<uint32_t>(index + 1)});
                            }
                            auto selection = selectOverload(candidates, arguments);
                            if (selection.ambiguous) {
                                SymbolTable.diagnose("overload", "ambiguous overload resolution", node);
                                return makeError();
                            }
                            if (!selection.candidate) {
                                SymbolTable.diagnose("constructor", "no viable overload for this constructor call", node);
                                return makeError();
                            }
                            if (Annotating) {
                                auto& metadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                                metadata.CallSignature = selection.candidate->type.value;
                                metadata.MemberIndex = selection.candidate->memberIndex;
                            }
                        }
                        return makeValue(type);
                    }
                }
            }

            const auto diagnosticsBeforeCandidates = SymbolTable.diagnostics().size();
            auto candidates = functionCandidates(node->getCallee(), node);
            if (candidates.empty()) {
                // Member/name lookup may already have explained why this callee
                // is invalid; avoid obscuring it with a second call diagnostic.
                if (SymbolTable.diagnostics().size() != diagnosticsBeforeCandidates ||
                    check(node->getCallee(), false).error) return makeError();
                SymbolTable.diagnose("call", "called expression is not a function", node);
                return makeError();
            }
            const auto selection = selectOverload(candidates, arguments);
            if (selection.ambiguous) {
                SymbolTable.diagnose("overload", "ambiguous overload resolution", node);
                return makeError();
            }
            if (!selection.candidate) {
                SymbolTable.diagnose("argument", "no viable overload for this call", node);
                return makeError();
            }
            const auto& selected = *selection.candidate;
            const auto best = selected.type;
            const auto bestSymbol = selected.symbol;
            const auto bestOwner = selected.owner;
            const auto bestMemberIndex = selected.memberIndex;
            if (!bestSymbol.value && check(node->getCallee(), false).error) return makeError();
            if (Annotating) {
                auto& callMetadata = const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo();
                callMetadata.CallSignature = best.value;
                callMetadata.MemberOwner = bestOwner.value; callMetadata.MemberIndex = bestMemberIndex;
                auto* callee = const_cast<types::ExpressionNode*>(node->getCallee());
                // A function-pointer object retains its pointer type and lvalue
                // category; selected signature is stored separately for lowering.
                auto original = RawResults.find(callee);
                if (original != RawResults.end()) {
                    callee->semanticInfo().Type = original->second.type.value;
                    callee->semanticInfo().LValue = original->second.category == ValueCategory::LValue;
                } else callee->semanticInfo().Type = best.value;
                callee->semanticInfo().Symbol = bestSymbol.value;
                callee->semanticInfo().MemberOwner = bestOwner.value;
                callee->semanticInfo().MemberIndex = bestMemberIndex;
                callee->semanticInfo().Annotated = true;
                auto* selectedCallee = callee;
                while (auto* comma = dynamic_cast<types::CommaExpressionNode*>(selectedCallee)) {
                    if (comma->getExpressions().size() != 1) break;
                    selectedCallee = comma->getExpressions().front();
                }
                if (selectedCallee != callee && bestSymbol.value) {
                    selectedCallee->semanticInfo() = callee->semanticInfo();
                }
                types::IdentifierNode* selectedName = nullptr;
                if (auto* primaryCallee = dynamic_cast<types::PrimaryExpressionNode*>(selectedCallee)) {
                    auto payload = primaryCallee->getExpression();
                    if (auto* identifier = std::get_if<types::IdentifierNode*>(&payload)) selectedName = *identifier;
                } else if (auto* memberCallee = dynamic_cast<types::MemberPostfixExpr*>(selectedCallee)) selectedName = memberCallee->getMember();
                else if (auto* arrowCallee = dynamic_cast<types::ArrowPostfixExpr*>(selectedCallee)) selectedName = arrowCallee->getMember();
                if (selectedName && bestSymbol.value) {
                    selectedName->resolve({bestSymbol.value}, std::string(selectedName->GetName()) + "." + std::to_string(bestSymbol.value));
                    selectedName->semanticInfo().Type = best.value;
                    selectedName->semanticInfo().Annotated = true;
                }
                const_cast<types::FunctionCallPostfixExpr*>(node)->semanticInfo().Symbol = bestSymbol.value;
            }
            auto resultType = TypeSystem.get(best).element;
            if (TypeSystem.isAggregate(resultType)) {
                // Aggregate results are temporaries; member use sees an rvalue.
            }
            return makeValue(resultType);
        }

        ExprResult ExpressionChecker::checkTernary(const types::TernaryOpNode* node) {
            auto condition = check(node->getCondition(), true);
            auto thenValue = check(node->getThen(), true);
            auto elseValue = check(node->getElse(), true);
            if (condition.error || thenValue.error || elseValue.error) {
                return makeError();
            }
            if (!checkScalarCondition(condition, node)) {
                return makeError();
            }
            TypeId type;
            if (isArithmetic(thenValue.type) && isArithmetic(elseValue.type)) {
                type = usualArithmetic(thenValue.type, elseValue.type, node);
            } else if (isPointer(thenValue.type) && isPointer(elseValue.type)) {
                auto leftPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(thenValue.type)).element);
                auto rightPointee = TypeSystem.unConst(TypeSystem.get(TypeSystem.unConst(elseValue.type)).element);
                bool qualified = TypeSystem.isConst(TypeSystem.get(TypeSystem.unConst(thenValue.type)).element) ||
                                 TypeSystem.isConst(TypeSystem.get(TypeSystem.unConst(elseValue.type)).element);
                if (leftPointee == rightPointee) {
                    type = TypeSystem.pointer(TypeSystem.constant(leftPointee, qualified));
                } else if (TypeSystem.isVoid(leftPointee) || TypeSystem.isVoid(rightPointee)) {
                    type = TypeSystem.pointer(TypeSystem.constant(TypeSystem.builtin(Builtin::Void), qualified));
                }
            } else if (isPointer(thenValue.type) && isNullPointerConstant(elseValue)) {
                type = thenValue.type;
            } else if (isPointer(elseValue.type) && isNullPointerConstant(thenValue)) {
                type = elseValue.type;
            } else if (TypeSystem.unConst(thenValue.type) == TypeSystem.unConst(elseValue.type)) {
                type = thenValue.type;
            }
            if (!type.value) {
                SymbolTable.diagnose("conditional", "incompatible conditional operand types", node);
                return makeError();
            }
            auto result = makeValue(type);
            if (thenValue.type == elseValue.type && thenValue.category == ValueCategory::LValue &&
                elseValue.category == ValueCategory::LValue) {
                result.category = ValueCategory::LValue;
            }
            return result;
        }

        ExprResult ExpressionChecker::checkCast(const types::CastFactorNode* node) {
            if (!node->getType()) {
                return makeError();
            }
            auto target = node->getType()->semanticInfo().Resolved ? TypeId{node->getType()->semanticInfo().Type}
                                                                   : ASTResolver.typeName(*node->getType());
            if (!node->getType()->semanticInfo().Resolved && node->getDeclarator()) {
                target = ASTResolver.declarator(target, node->getDeclarator());
            }
            if (!target.value) {
                return makeError();
            }
            auto source = check(node->getExpression(), true);
            if (source.error) {
                return makeError();
            }
            auto targetKind = TypeSystem.get(TypeSystem.unConst(target)).kind;
            if (targetKind == TypeKind::Array || targetKind == TypeKind::Function) {
                SymbolTable.diagnose("cast", "cast target cannot be an array or function", node);
                return makeError();
            }
            if (isArithmetic(target) && isArithmetic(source.type)) {
                auto result = makeValue(target);
                if (source.constantExpression && isInteger(target) && isInteger(source.type)) {
                    result.constantExpression = true;
                    result.constant = source.constant;
                    auto info = TypeSystem.arithmetic(target);
                    if (source.constant.state == ConstantState::Value && info && info->bits < 64) {
                        auto bits = static_cast<uint64_t>(source.constant.value) & ((uint64_t{1} << info->bits)-1);
                        if (info->isSigned && (bits & (uint64_t{1} << (info->bits-1)))) bits |= ~((uint64_t{1} << info->bits)-1);
                        result.constant.value = static_cast<int64_t>(bits);
                    }
                } else if (source.constantExpression && isInteger(target) && source.floatingConstant) {
                    const auto info = TypeSystem.arithmetic(target);
                    const auto value = std::trunc(*source.floatingConstant);
                    const auto high = std::ldexp(1.0L, info->isSigned ? info->bits-1 : info->bits);
                    const auto low = info->isSigned ? -high : 0.0L;
                    if (std::isfinite(value) && value >= low && value < high) {
                        result.constantExpression = true;
                        result.constant = {ConstantState::Value, info->isSigned ? static_cast<int64_t>(value)
                            : static_cast<int64_t>(static_cast<uint64_t>(value))};
                    }
                } else if (source.constantExpression && TypeSystem.arithmetic(target)->floating) {
                    result.constantExpression = true;
                    result.floatingConstant = source.floatingConstant ? source.floatingConstant
                        : std::optional<long double>{static_cast<long double>(source.constant.value)};
                }
                return result;
            }
            if (isPointer(target) && isPointer(source.type)) {
                return makeValue(target);
            }
            if (isPointer(target) && isInteger(source.type)) {
                return makeValue(target);
            }
            if (isInteger(target) && isPointer(source.type)) {
                return makeValue(target);
            }
            if (TypeSystem.unConst(target) == TypeSystem.unConst(source.type)) {
                return makeValue(target);
            }
            SymbolTable.diagnose("cast", "unsupported cast conversion", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkSizeof(const types::SizeofNode* node) {
            TypeId measured;
            const auto& payload = node->getPayload();
            if (std::holds_alternative<types::SizeofNode::SizeofTypeVariant>(payload)) {
                const auto& variant = std::get<types::SizeofNode::SizeofTypeVariant>(payload);
                if (!variant.Type) {
                    return makeError();
                }
                measured = variant.Type->semanticInfo().Resolved ? TypeId{variant.Type->semanticInfo().Type}
                                                               : ASTResolver.typeName(*variant.Type);
                if (!variant.Type->semanticInfo().Resolved && variant.AbstractDecl) {
                    measured = ASTResolver.declarator(measured, variant.AbstractDecl);
                }
            } else {
                auto* expression = std::get<types::ExpressionNode*>(payload);
                if (!expression) {
                    return makeError();
                }
                auto value = check(expression, false);
                if (value.error) {
                    return makeError();
                }
                measured = value.type;
            }
            if (!measured.value) {
                return makeError();
            }
            auto layout = TypeSystem.layout(measured);
            if (!layout) {
                SymbolTable.diagnose("sizeof", layout.message.empty() ? "sizeof requires a complete object type"
                                                                      : layout.message,
                                     node);
                return makeError();
            }
            return makeConstant(TypeSystem.sizeType(), static_cast<int64_t>(layout.layout.size));
        }

        ExprResult ExpressionChecker::checkInitializerExpression(const types::InitializerNode* node, TypeId target) {
            if (target.value) return makeValue(target);
            if (!node->isBraced() && node->expressions().size() == 1) return check(node->expressions().front(), false);
            SymbolTable.diagnose("initializer", "initializer list is not valid in this expression context", node);
            return makeError();
        }

        ExprResult ExpressionChecker::checkImpl(const types::ExpressionNode* expression, bool shouldDecay) {
            if (!expression || expression->hasError()) {
                return makeError();
            }
            ExprResult result;
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                result = checkPrimary(primary);
            } else if (auto* unary = dynamic_cast<const types::FactorNode*>(expression)) {
                result = checkUnary(unary);
            } else if (auto* binary = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
                result = checkBinary(binary);
            } else if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
                result = checkIndex(index);
            } else if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                result = checkMember(member->getBase(), member->getMember(), false, expression);
            } else if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                result = checkMember(arrow->getBase(), arrow->getMember(), true, expression);
            } else if (auto* call = dynamic_cast<const types::FunctionCallPostfixExpr*>(expression)) {
                result = checkCall(call);
            } else if (auto* postfix = dynamic_cast<const types::UnaryPostfixExpr*>(expression)) {
                auto operand = check(postfix->getOperand(), false);
                if (operand.error) {
                    return operand;
                }
                if (!isModifiableLValue(operand)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a modifiable lvalue", postfix);
                    return makeError();
                }
                if (!isScalar(operand.type)) {
                    SymbolTable.diagnose("mutation", "increment/decrement requires a scalar", postfix);
                    return makeError();
                }
                if (isPointer(operand.type) && !isCompleteObject(TypeSystem.get(TypeSystem.unConst(operand.type)).element)) {
                    SymbolTable.diagnose("mutation", "pointer increment requires a complete object pointee", postfix);
                    return makeError();
                }
                operand.constantExpression = false;
                operand.constant = {};
                operand.category = ValueCategory::RValue;
                result = operand;
            } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
                result = makeError();
                for (auto* item : comma->getExpressions()) {
                    result = check(item, comma->getExpressions().size() == 1 ? shouldDecay : true);
                    if (result.error) {
                        return result;
                    }
                }
                if (comma->getExpressions().size() != 1) {
                    result.constantExpression = false;
                    result.constant = {};
                }
            } else if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
                result = checkTernary(ternary);
            } else if (auto* cast = dynamic_cast<const types::CastFactorNode*>(expression)) {
                result = checkCast(cast);
            } else if (auto* sizeofNode = dynamic_cast<const types::SizeofNode*>(expression)) {
                result = checkSizeof(sizeofNode);
            } else if (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
                result = checkInitializerExpression(initializer, {});
            } else {
                SymbolTable.diagnose("expression", "unsupported expression node", expression);
                return makeError();
            }
            if (result.error) {
                return result;
            }
            if (shouldDecay) {
                result.type = decay(result.type);
                result.category = ValueCategory::RValue;
                result.nullPointerConstant = result.nullPointerConstant;
            }
            return result;
        }

        ExprResult ExpressionChecker::check(const types::ExpressionNode* expression, bool shouldDecay) {
            if (!expression || (expression->semanticInfo().Resolved && expression->semanticInfo().Error))
                return makeError(); // resolver already diagnosed this occurrence
            auto& cache = shouldDecay ? ValueResults : RawResults;
            if (Annotating) {
                auto found = cache.find(expression);
                if (found != cache.end()) return found->second;
            }
            ExprResult raw;
            auto cached = RawResults.find(expression);
            if (Annotating && cached != RawResults.end()) raw = cached->second;
            else raw = checkImpl(expression, false);
            if (Annotating) {
                RawResults[expression] = raw;
                auto& metadata = const_cast<types::ExpressionNode*>(expression)->semanticInfo();
                metadata.Type = raw.type.value; metadata.Annotated = true; metadata.Error = raw.error;
                metadata.LValue = raw.category == ValueCategory::LValue;
                metadata.ConstantExpression = raw.constantExpression;
                metadata.HasConstant = raw.constant.state == ConstantState::Value;
                metadata.Constant = raw.constant.value;
                metadata.NullPointerConstant = raw.nullPointerConstant;
                metadata.HasFloatingConstant = raw.floatingConstant.has_value();
                metadata.FloatingConstant = raw.floatingConstant.value_or(0);
            }
            auto value = raw;
            if (shouldDecay && !value.error && value.boundInstanceFunction) {
                SymbolTable.diagnose("member-function", "instance member function requires an object-bound call", expression);
                value = makeError();
                if (Annotating) const_cast<types::ExpressionNode*>(expression)->semanticInfo().Error = true;
            }
            if (shouldDecay && !value.error) {
                value.type = decay(value.type);
                value.category = ValueCategory::RValue;
            }
            if (Annotating) cache[expression] = value;
            return value;
        }

        IntegerResult ExpressionChecker::integer(const types::ExpressionNode* expression, EvaluationContext context) {
            (void)context;
            while (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
                if (initializer->expressions().size() != 1) return {ConstantState::Nonconstant, 0};
                expression = initializer->expressions().front();
            }
            auto value = check(expression, false);
            if (value.error) {
                return {ConstantState::Error, 0};
            }
            if (!isInteger(value.type)) {
                return {ConstantState::Nonconstant, 0};
            }
            if (!value.constantExpression || value.constant.state != ConstantState::Value) {
                return {ConstantState::Nonconstant, 0};
            }
            return value.constant;
        }

        ExprResult ExpressionChecker::checkValue(const types::ExpressionNode* expression) {
            return check(expression, true);
        }

        bool ExpressionChecker::checkCondition(const types::ExpressionNode* expression, const void* source) {
            auto value = check(expression, true);
            if (value.error) {
                return false;
            }
            return checkScalarCondition(value, source);
        }

        bool ExpressionChecker::flattenMembers(TypeId type, std::vector<TypeId>& out) const {
            type = TypeSystem.unConst(type);
            const auto& info = TypeSystem.get(type);
            if (info.base.value) {
                out.push_back(info.base);
            }
            for (auto index : TypeSystem.instanceMembers(type)) {
                out.push_back(info.members[index].type);
            }
            return true;
        }

        bool ExpressionChecker::initializeScalar(TypeId target, const types::InitializerNode& initializer,
                                                 const void* source) {
            if (Annotating) {
                auto& metadata = const_cast<types::InitializerNode&>(initializer).semanticInfo();
                metadata.Type = target.value; metadata.Annotated = true;
            }
            const auto& values = initializer.expressions();
            if (initializer.isBraced()) {
                if (values.empty()) {
                    return true; // zero initialization
                }
                if (values.size() != 1) {
                    SymbolTable.diagnose("initializer", "too many scalar initializers", source);
                    return false;
                }
            }
            if (values.empty()) {
                return true;
            }
            if (auto* nested = dynamic_cast<const types::InitializerNode*>(values.front()))
                return initializeScalar(target, *nested, nested);
            auto value = check(values.front(), true);
            if (value.error) {
                return false;
            }
            if (!isAssignable(value.type, target, value)) {
                SymbolTable.diagnose("initializer", "initializer is not convertible to the declared type", source);
                return false;
            }
            planConversion(values.front(), target);
            return true;
        }

        bool ExpressionChecker::initializeList(TypeId target, const std::vector<types::ExpressionNode*>& values,
                                               size_t& cursor, bool braced, const void* source) {
            if (Annotating && source) {
                auto* node = const_cast<types::ASTNode*>(static_cast<const types::ASTNode*>(source));
                node->semanticInfo().Type = target.value; node->semanticInfo().Annotated = true;
            }
            target = TypeSystem.unConst(target);
            const auto& info = TypeSystem.get(target);
            if (info.kind == TypeKind::Array) {
                uint64_t Index = 0;
                while (cursor < values.size() &&
                       (info.bound.kind != BoundKind::Constant || Index < info.bound.count)) {
                    auto* value = values[cursor];
                    if (auto* nested = dynamic_cast<const types::InitializerNode*>(value); nested && nested->isBraced()) {
                        ++cursor;
                        size_t nestedCursor = 0;
                        if (!initializeList(info.element, nested->expressions(), nestedCursor, true, nested)) {
                            return false;
                        }
                    } else if (TypeSystem.get(TypeSystem.unConst(info.element)).kind == TypeKind::Array ||
                               TypeSystem.isAggregate(info.element)) {
                        if (!initializeList(info.element, values, cursor, false, source)) {
                            return false;
                        }
                    } else {
                        ++cursor;
                        auto checked = check(value, true);
                        if (checked.error || !isAssignable(checked.type, info.element, checked)) {
                            SymbolTable.diagnose("initializer", "invalid array element initializer", source);
                            return false;
                        }
                        planConversion(value, info.element);
                    }
                    ++Index;
                }
                if (braced && cursor < values.size()) {
                    SymbolTable.diagnose("initializer", "too many array initializers", source);
                    return false;
                }
                return true;
            }
            if (info.kind == TypeKind::Union) {
                if (cursor >= values.size()) {
                    return true;
                }
                auto members = TypeSystem.instanceMembers(target);
                if (members.empty()) {
                    SymbolTable.diagnose("initializer", "union has no initializable member", source);
                    return false;
                }
                auto* value = values[cursor++];
                auto memberType = info.members[members.front()].type;
                if (auto* nested = dynamic_cast<const types::InitializerNode*>(value); nested && nested->isBraced()) {
                    size_t nestedCursor = 0;
                    return initializeList(memberType, nested->expressions(), nestedCursor, true, nested);
                }
                auto checked = check(value, true);
                if (checked.error || !isAssignable(checked.type, memberType, checked)) {
                    SymbolTable.diagnose("initializer", "invalid union initializer", source);
                    return false;
                }
                planConversion(value, memberType);
                return true;
            }
            if (TypeSystem.isAggregate(target)) {
                std::vector<TypeId> members;
                flattenMembers(target, members);
                if (braced) {
                    for (auto member : members) {
                        if (cursor >= values.size()) {
                            return true; // remaining members are zero initialized
                        }
                        auto* value = values[cursor++];
                        if (auto* nested = dynamic_cast<const types::InitializerNode*>(value); nested && nested->isBraced()) {
                            size_t nestedCursor = 0;
                            if (!initializeList(member, nested->expressions(), nestedCursor, true, nested)) {
                                return false;
                            }
                        } else {
                            auto checked = check(value, true);
                            if (!isAssignable(checked.type, member, checked)) {
                                SymbolTable.diagnose("initializer", "invalid member initializer", source);
                                return false;
                            }
                            planConversion(value, member);
                        }
                    }
                    if (cursor < values.size()) {
                        SymbolTable.diagnose("initializer", "too many aggregate initializers", source);
                        return false;
                    }
                    return true;
                }
                for (auto member : members) {
                    if (cursor >= values.size()) {
                        return true;
                    }
                    auto* value = values[cursor];
                    if (auto* nested = dynamic_cast<const types::InitializerNode*>(value); nested && nested->isBraced()) {
                        ++cursor;
                        size_t nestedCursor = 0;
                        if (!initializeList(member, nested->expressions(), nestedCursor, true, nested)) {
                            return false;
                        }
                    } else if (TypeSystem.get(TypeSystem.unConst(member)).kind == TypeKind::Array ||
                               TypeSystem.isAggregate(member)) {
                        if (!initializeList(member, values, cursor, false, source)) {
                            return false;
                        }
                    } else {
                        ++cursor;
                        auto checked = check(value, true);
                        if (!isAssignable(checked.type, member, checked)) {
                            SymbolTable.diagnose("initializer", "invalid member initializer", source);
                            return false;
                        }
                        planConversion(value, member);
                    }
                }
                return true;
            }
            if (cursor < values.size()) {
                auto* value = values[cursor++];
                if (auto* nested = dynamic_cast<const types::InitializerNode*>(value); nested && nested->isBraced()) {
                    size_t nestedCursor = 0;
                    return initializeList(target, nested->expressions(), nestedCursor, true, nested);
                }
                auto checked = check(value, true);
                if (!isAssignable(checked.type, target, checked)) {
                    SymbolTable.diagnose("initializer", "invalid scalar initializer", source);
                    return false;
                }
                planConversion(value, target);
            }
            return true;
        }

        bool ExpressionChecker::initializeArray(TypeId target, const types::InitializerNode& initializer) {
            const auto& info = TypeSystem.get(TypeSystem.unConst(target));
            const auto& values = initializer.expressions();
            if (!initializer.isBraced() && values.size() == 1) {
                auto* constant = StringLiteral(values.front());
                if (!constant || TypeSystem.unConst(info.element) != TypeSystem.builtin(Builtin::Char)) {
                    SymbolTable.diagnose("initializer", "array initializer requires braces or a character string", &initializer);
                    return false;
                }
                // A string-to-array initializer is a copy operation, not decay.
                // Still annotate its literal/array type for later lowering.
                check(values.front(), false);
                auto length = constant->getValue().size() + 1;
                if (info.bound.kind == BoundKind::Constant && info.bound.count < length) {
                    SymbolTable.diagnose("initializer", "string literal does not fit the array", &initializer);
                    return false;
                }
                return true;
            }
            if (initializer.isBraced() && values.size() == 1) {
                auto* constant = StringLiteral(values.front());
                if (constant && TypeSystem.unConst(info.element) == TypeSystem.builtin(Builtin::Char)) {
                    check(values.front(), false);
                    auto length = constant->getValue().size() + 1;
                    if (info.bound.kind == BoundKind::Constant && info.bound.count < length) {
                        SymbolTable.diagnose("initializer", "string literal does not fit the array", &initializer);
                        return false;
                    }
                    return true;
                }
            }
            size_t cursor = 0;
            bool result = initializeList(target, values, cursor, initializer.isBraced(), &initializer);
            if (result && cursor < values.size()) {
                SymbolTable.diagnose("initializer", "too many array initializers", &initializer);
                return false;
            }
            return result;
        }

        bool ExpressionChecker::initializeAggregate(TypeId target, const types::InitializerNode& initializer,
                                                    bool topLevel) {
            const auto& values = initializer.expressions();
            if (!initializer.isBraced() && values.size() == 1) {
                auto checked = check(values.front(), true);
                if (checked.error) {
                    return false;
                }
                if (TypeSystem.unConst(checked.type) != TypeSystem.unConst(target) &&
                    !TypeSystem.derivesFrom(checked.type, target)) {
                    SymbolTable.diagnose("initializer", "aggregate copy requires the same or derived type",
                                         &initializer);
                    return false;
                }
                return true;
            }
            size_t cursor = 0;
            bool result = initializeList(target, values, cursor, initializer.isBraced(), &initializer);
            if (result && topLevel && cursor < values.size()) {
                SymbolTable.diagnose("initializer", "too many aggregate initializers", &initializer);
                return false;
            }
            return result;
        }

        bool ExpressionChecker::initialize(SymbolId symbol, const types::ExpressionNode* initializerNode) {
            if (!symbol.value) {
                return false;
            }
            auto* initializer = dynamic_cast<const types::InitializerNode*>(initializerNode);
            if (!initializer) {
                if (!initializerNode) {
                    return true;
                }
                SymbolTable.diagnose("initializer", "invalid initializer AST", initializerNode);
                return false;
            }
            auto type = SymbolTable.get(symbol).type;
            auto raw = TypeSystem.unConst(type);
            if (TypeSystem.get(raw).kind == TypeKind::Array &&
                TypeSystem.get(raw).bound.kind == BoundKind::Unspecified) {
                if (!ASTResolver.inferArrayBound(symbol, initializer)) return false;
                type = SymbolTable.get(symbol).type;
            }
            raw = TypeSystem.unConst(type);
            if (TypeSystem.get(raw).kind == TypeKind::Array) {
                return initializeArray(type, *initializer);
            }
            if (TypeSystem.isAggregate(raw)) {
                return initializeAggregate(raw, *initializer, true);
            }
            if (!initializeScalar(raw, *initializer, initializer)) {
                return false;
            }
            if (TypeSystem.isConst(type) && isInteger(type) && !initializer->isBraced() &&
                initializer->expressions().size() == 1) {
                auto value = check(initializer->expressions().front(), false);
                if (!value.error && value.constantExpression && value.constant.state == ConstantState::Value) {
                    ConstantObjects[symbol.value] = value.constant.value;
                }
            }
            return true;
        }

        std::optional<Member> ExpressionChecker::memberSignature(const types::ASTNode* node, TypeId owner) {
            if (auto* function = dynamic_cast<const types::FunctionNode*>(node)) {
                auto* returnName = function->getReturnTypeName();
                if (!returnName) {
                    return std::nullopt;
                }
                auto type = ASTResolver.specifier(returnName, function->getModifier(), function->getIsConst(),
                                                  function);
                type = ASTResolver.declarator(type, function->getDeclarator());
                if (!type.value) {
                    return std::nullopt;
                }
                Member member;
                member.name = std::string(function->getDeclarator() ? function->getDeclarator()->getVariableName()
                                                                    : std::string_view{});
                member.type = type;
                if (auto* params = function->getDeclarator() ? dynamic_cast<types::FunctionParameterList*>(
                                                                   function->getDeclarator()->getFunctionParameters())
                                                             : nullptr) {
                    size_t required = 0;
                    for (auto* parameter : params->getParameters()) {
                        if (parameter && !parameter->hasInitalizer()) {
                            ++required;
                        }
                    }
                    member.minimumArity = required;
                }
                member.isStatic = function->getIsStatic();
                member.isConstMethod = false; // leading const is return qualification, not a receiver qualifier
                member.source = function;
                return member;
            }
            if (auto* constructor = dynamic_cast<const types::ConstructorNode*>(node)) {
                std::vector<TypeId> parameters;
                if (auto* args = constructor->getArgs()) {
                    SymbolTable.enter(ScopeKind::Prototype);
                    for (auto* parameter : args->getParameters()) {
                        auto type = ASTResolver.specifier(parameter->getTypeName(), parameter->getModifier(),
                                                          parameter->getIsConst(), parameter);
                        if (auto* declaration = parameter->getDeclaration()) {
                            type = ASTResolver.declarator(type, declaration->getDeclarator());
                        }
                        parameters.push_back(TypeSystem.adjustParameter(type));
                    }
                    SymbolTable.leave();
                }
                auto type = TypeSystem.function(TypeSystem.builtin(Builtin::Void), std::move(parameters));
                if (!type.value) {
                    return std::nullopt;
                }
                Member member;
                size_t required = 0;
                if (auto* args = constructor->getArgs()) {
                    for (auto* parameter : args->getParameters()) {
                        if (parameter && !parameter->hasInitalizer()) {
                            ++required;
                        }
                    }
                    member.minimumArity = required;
                }
                member.name = TypeSystem.get(owner).name;
                member.type = type;
                member.isConstructor = true;
                member.source = constructor;
                return member;
            }
            if (auto* destructor = dynamic_cast<const types::DestructorNode*>(node)) {
                auto type = TypeSystem.function(TypeSystem.builtin(Builtin::Void));
                if (!type.value) {
                    return std::nullopt;
                }
                (void)destructor;
                Member member;
                member.name = "~" + TypeSystem.get(owner).name;
                member.type = type;
                member.isDestructor = true;
                member.source = destructor;
                return member;
            }
            return std::nullopt;
        }
    }
}
