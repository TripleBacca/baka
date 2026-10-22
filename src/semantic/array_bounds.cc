#include "ast_declarations.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/initializer.hh"

namespace baka {
    namespace semantic {
        namespace {
            const types::ExpressionNode* UnwrapInitializer(const types::ExpressionNode* expression) {
                while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                    if (list->isBraced() || list->expressions().size() != 1) {
                        break;
                    }
                    expression = list->expressions().front();
                }
                return expression;
            }

            std::optional<size_t> StringLength(const types::ExpressionNode* expression) {
                auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(UnwrapInitializer(expression));
                if (!primary) {
                    return {};
                }
                const auto& payload = primary->getExpression();
                auto* constant = std::get_if<types::ConstantNode*>(&payload);
                if (!constant) {
                    return {};
                }
                if (auto* string = dynamic_cast<const types::ConstantTNode<base::SLString>*>(*constant)) {
                    return string->getValue().size() + 1;
                }
                return {};
            }
        }

        bool ASTDeclarations::isAggregateValue(const types::ExpressionNode* expression, TypeId target) const {
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression);
            if (!primary) {
                return false;
            }
            auto* name = std::get_if<types::IdentifierNode*>(&primary->getExpression());
            if (!name) {
                return false;
            }
            auto found = SymbolTable.lookup(std::string((*name)->GetName()));
            return found.size() == 1 &&
                   TypeSystem.unConst(SymbolTable.get(found.front()).type) == TypeSystem.unConst(target);
        }

        void ASTDeclarations::consumeArrayElement(TypeId target, const std::vector<types::ExpressionNode*>& values,
                                                  size_t& cursor) {
            if (cursor >= values.size()) {
                return;
            }
            auto* value = UnwrapInitializer(values[cursor]);
            if (auto* list = dynamic_cast<const types::InitializerNode*>(value); list && list->isBraced()) {
                ++cursor;
                return;
            }
            const auto& info = TypeSystem.get(TypeSystem.unConst(target));
            if (info.kind == TypeKind::Array) {
                if (StringLength(value)) {
                    ++cursor;
                    return;
                }
                for (uint64_t i = 0; i < info.bound.count && cursor < values.size(); ++i) {
                    consumeArrayElement(info.element, values, cursor);
                }
            } else if (TypeSystem.isAggregate(target)) {
                if (isAggregateValue(value, target)) {
                    ++cursor;
                    return;
                }
                auto subobjects = TypeSystem.subobjects(target);
                if (info.kind == TypeKind::Union && subobjects.size() > 1) {
                    subobjects.resize(1);
                }
                for (const auto& child : subobjects) {
                    consumeArrayElement(child.type, values, cursor);
                }
            } else {
                ++cursor;
            }
        }

        bool ASTDeclarations::inferArrayBound(SymbolId symbol, const types::ExpressionNode* expression) {
            if (!symbol.value) {
                return false;
            }
            auto type = SymbolTable.get(symbol).type;
            const auto& array = TypeSystem.get(TypeSystem.unConst(type));
            if (array.kind != TypeKind::Array || array.bound.kind != BoundKind::Unspecified) {
                return true;
            }
            auto* initializer = dynamic_cast<const types::InitializerNode*>(expression);
            if (!initializer) {
                return false;
            }
            const auto& values = initializer->expressions();
            size_t count = 0;
            if (values.size() == 1 && StringLength(values.front()) &&
                TypeSystem.unConst(array.element) == TypeSystem.builtin(Builtin::Char)) {
                count = *StringLength(values.front());
            } else if (initializer->isBraced()) {
                // Consume one element's source initializer at a time. This also
                // handles brace elision: int a[][2]={1,2,3,4} has two rows, not four.
                size_t cursor = 0;
                while (cursor < values.size()) {
                    auto before = cursor;
                    consumeArrayElement(array.element, values, cursor);
                    if (before == cursor) {
                        break;
                    }
                    ++count;
                }
            }
            if (!count) {
                SymbolTable.diagnose("bound", "cannot infer a positive array bound from this initializer", initializer);
                return false;
            }
            auto completed = TypeSystem.array(array.element, {BoundKind::Constant, count});
            return SymbolTable.completeObject(symbol, completed);
        }

    }
}
