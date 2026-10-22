#include "expressions.hh"

namespace baka {
    namespace semantic {
        namespace {
            const types::ConstantTNode<base::SLString>* StringLiteral(const types::ExpressionNode* expression) {
                while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                    if (list->isBraced() || list->expressions().size() != 1) {
                        break;
                    }
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

        }

        ExprResult ExpressionChecker::checkInitializerExpression(const types::InitializerNode* node, TypeId target) {
            if (target.value) {
                return makeValue(target);
            }
            if (!node->isBraced() && node->expressions().size() == 1) {
                return check(node->expressions().front(), false);
            }
            SymbolTable.diagnose("initializer", "initializer list is not valid in this expression context", node);
            return makeError();
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
                                                 const types::ASTNode* source) {
            if (Annotating) {
                auto& metadata = const_cast<types::InitializerNode&>(initializer).semanticInfo();
                metadata.Type = target.value;
                metadata.Annotated = true;
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
            if (auto* nested = dynamic_cast<const types::InitializerNode*>(values.front())) {
                return initializeScalar(target, *nested, nested);
            }
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
                                               size_t& cursor, bool braced, const types::ASTNode* source) {
            if (Annotating && source) {
                auto* node = const_cast<types::ASTNode*>(static_cast<const types::ASTNode*>(source));
                node->semanticInfo().Type = target.value;
                node->semanticInfo().Annotated = true;
            }
            target = TypeSystem.unConst(target);
            const auto& info = TypeSystem.get(target);
            if (info.kind == TypeKind::Array) {
                uint64_t Index = 0;
                while (cursor < values.size() && (info.bound.kind != BoundKind::Constant || Index < info.bound.count)) {
                    auto* value = values[cursor];
                    if (auto* nested = dynamic_cast<const types::InitializerNode*>(value);
                        nested && nested->isBraced()) {
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
                        if (auto* nested = dynamic_cast<const types::InitializerNode*>(value);
                            nested && nested->isBraced()) {
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
                    if (auto* nested = dynamic_cast<const types::InitializerNode*>(value);
                        nested && nested->isBraced()) {
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
                    SymbolTable.diagnose("initializer", "array initializer requires braces or a character string",
                                         &initializer);
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
                if (!ASTResolver.inferArrayBound(symbol, initializer)) {
                    return false;
                }
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
                auto value = convertConstant(check(initializer->expressions().front(), false), type);
                if (!value.error && value.constantExpression && value.constant.state == ConstantState::Value) {
                    ConstantObjects[symbol.value] = value.constant.value;
                }
            }
            return true;
        }
    }
}
