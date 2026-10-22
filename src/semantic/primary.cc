#include "expressions.hh"
#include <limits>

namespace baka {
    namespace semantic {
        std::vector<SymbolId> ExpressionChecker::lookupIdentifier(const types::IdentifierNode* identifier) const {
            if (!identifier) {
                return {};
            }
            if (identifier->semanticInfo().Resolved) {
                std::vector<SymbolId> result;
                for (auto id : identifier->getBindings()) {
                    result.push_back({id});
                }
                return result;
            }
            return SymbolTable.lookup(std::string(identifier->GetName()));
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
                return makeValue(TypeSystem.array(element, {BoundKind::Constant, length + 1}), ValueCategory::LValue);
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
                    metadata.Type = result.type.value;
                    metadata.Annotated = true;
                    metadata.Error = result.error;
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
                            result.boundInstanceFunction =
                                !entry.isStatic &&
                                TypeSystem.get(TypeSystem.unConst(entry.type)).kind == TypeKind::Function;
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
                for (auto index : member.indices) {
                    if (TypeSystem.get(member.owner).members[index].symbol == found.front()) {
                        auto& metadata = const_cast<types::PrimaryExpressionNode*>(node)->semanticInfo();
                        metadata.MemberOwner = member.owner.value;
                        metadata.MemberIndex = static_cast<uint32_t>(index + 1);
                    }
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
                auto result = makeValue(symbol.type, symbol.kind == SymbolKind::Enumerator ? ValueCategory::RValue
                                                                                           : ValueCategory::LValue);
                result.constantExpression = true;
                result.constant = {ConstantState::Value, symbol.constant.value_or(0)};
                return result;
            }
            if (symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::BuiltinFunction) {
                auto result = makeValue(symbol.type);
                result.boundInstanceFunction =
                    SymbolTable.scope(symbol.scope).kind == ScopeKind::Aggregate && !symbol.isStatic;
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

        ExprResult ExpressionChecker::checkComma(const types::CommaExpressionNode* comma) {
            auto result = makeError();
            for (auto* item : comma->getExpressions()) {
                result = check(item, comma->getExpressions().size() != 1);
                if (result.error) {
                    return result;
                }
            }
            if (comma->getExpressions().size() != 1) {
                result.constantExpression = false;
                result.constant = {};
            }
            return result;
        }

    }
}
