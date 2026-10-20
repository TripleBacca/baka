#include "lowering.hh"
#include <functional>

namespace baka::ir {
    namespace {
        const types::ExpressionNode* Transparent(const types::ExpressionNode* expression) {
            for (;;) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(expression);
                    list && !list->isBraced() && list->expressions().size() == 1) {
                    expression = list->expressions().front();
                } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression);
                           comma && comma->getExpressions().size() == 1) {
                    expression = comma->getExpressions().front();
                } else {
                    return expression;
                }
            }
        }

        std::optional<std::string> String(const types::ExpressionNode* expression) {
            expression = Transparent(expression);
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression);
            if (!primary) {
                return {};
            }
            auto* constant = std::get_if<types::ConstantNode*>(&primary->getExpression());
            if (!constant) {
                return {};
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<base::SLString>*>(*constant)) {
                return std::string(literal->getValue());
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<std::string>*>(*constant)) {
                return literal->getValue();
            }
            return {};
        }
    }

    std::vector<std::pair<TypeId, uint64_t>> Lowerer::subobjects(TypeId id) {
        auto type = Result.type(id);
        std::vector<std::pair<TypeId, uint64_t>> result;
        if (type.kind == TypeKind::Array) {
            for (uint64_t i = 0; i < type.count; ++i) {
                result.push_back({type.element, i * Result.type(type.element).size});
            }
        } else if (type.kind == TypeKind::Record) {
            semantic::TypeId semanticType;
            for (auto [original, imported] : TypeMap) {
                if (imported == id) {
                    semanticType = {original};
                    break;
                }
            }
            if (!semanticType.value) {
                fail(nullptr, "record initializer lacks a semantic layout");
            }
            auto layout = Analyzer.types().layout(semanticType);
            if (!layout) {
                fail(nullptr, "record initializer has no complete layout");
            }
            const auto& original = Analyzer.types().get(semanticType);
            for (const auto& child : Analyzer.types().subobjects(semanticType)) {
                auto offset =
                    child.memberIndex ? layout.layout.offsets.at(*child.memberIndex) : layout.layout.baseOffset;
                if (!offset) {
                    fail(nullptr, "aggregate initializer member lacks layout offset");
                }
                result.push_back({importType(child.type), *offset});
                if (original.kind == semantic::TypeKind::Union) {
                    break;
                }
            }
        }
        return result;
    }

    std::vector<Lowerer::InitializerEntry> Lowerer::initializerPlan(TypeId target,
                                                                    const types::ExpressionNode* initializer) {
        std::vector<InitializerEntry> result;
        std::function<void(TypeId, uint64_t, const types::ExpressionNode*)> whole;
        std::function<void(TypeId, uint64_t, const std::vector<types::ExpressionNode*>&, size_t&)> consume;
        consume = [&](TypeId type, uint64_t offset, const auto& expressions, size_t& position) {
            if (position == expressions.size()) {
                whole(type, offset, nullptr);
                return;
            }
            auto* expression = Transparent(expressions[position]);
            auto kind = Result.type(type).kind;
            if ((kind == TypeKind::Record && expression &&
                 Result.type(this->type(expression)).kind == TypeKind::Record) ||
                dynamic_cast<const types::InitializerNode*>(expression) ||
                (kind != TypeKind::Array && kind != TypeKind::Record) || String(expression)) {
                ++position;
                whole(type, offset, expression);
                return;
            }
            for (auto [child, childOffset] : subobjects(type)) {
                consume(child, offset + childOffset, expressions, position);
            }
        };
        whole = [&](TypeId type, uint64_t offset, const types::ExpressionNode* expression) {
            expression = Transparent(expression);
            const auto info = Result.type(type);
            if (info.kind == TypeKind::Record && expression &&
                Result.type(this->type(expression)).kind == TypeKind::Record &&
                !dynamic_cast<const types::InitializerNode*>(expression)) {
                result.push_back({offset, type, expression});
                return;
            }
            if (info.kind != TypeKind::Record && info.kind != TypeKind::Array) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                    if (list->expressions().size() > 1) {
                        fail(list, "excess scalar initializer in IR");
                    }
                    expression = list->expressions().empty() ? nullptr : list->expressions().front();
                }
                result.push_back({offset, type, expression});
                return;
            }
            auto* list = dynamic_cast<const types::InitializerNode*>(expression);
            auto stringExpression = expression;
            if (list && list->expressions().size() == 1) {
                stringExpression = list->expressions().front();
            }
            if (info.kind == TypeKind::Array && Result.type(info.element).kind == TypeKind::Integer &&
                Result.type(info.element).bits == 8 && String(stringExpression)) {
                // Byte elements are recorded separately by initialize/initializeGlobal.
                result.push_back({offset, type, stringExpression});
                return;
            }
            if (expression && !list) {
                fail(expression, "aggregate copies are not supported by IR yet");
            }
            auto children = subobjects(type);
            size_t position = 0;
            for (auto [child, childOffset] : children) {
                if (list) {
                    consume(child, offset + childOffset, list->expressions(), position);
                } else {
                    whole(child, offset + childOffset, nullptr);
                }
            }
            if (list && position != list->expressions().size()) {
                fail(list, "aggregate initializer could not be lowered completely");
            }
        };
        whole(target, 0, initializer);
        return result;
    }

    void Lowerer::initialize(Operand destination, TypeId target, const types::ExpressionNode* initializer, bool zero) {
        if (!initializer && !zero) {
            return;
        }
        for (const auto& entry : initializerPlan(target, initializer)) {
            auto info = Result.type(entry.type);
            if (info.kind == TypeKind::Array) {
                auto text = *String(entry.expression);
                text.push_back('\0');
                for (uint64_t i = 0; i < info.count; ++i) {
                    auto location = emitValue(Opcode::AddressOffset, Result.pointer(info.element), destination,
                                              integer(integerType(), entry.offset + i));
                    auto byte = i < text.size() ? static_cast<unsigned char>(text[i]) : 0;
                    emit(Opcode::Store, location, integer(info.element, byte));
                }
                continue;
            }
            Operand initial;
            if (entry.expression) {
                initial = cast(value(entry.expression), entry.type);
            } else if (info.kind == TypeKind::Floating) {
                initial = {OperandKind::Floating, entry.type, 0, 0, 0};
            } else {
                initial = integer(entry.type, 0);
            }
            auto location = destination;
            auto pointer = Result.pointer(entry.type);
            if (entry.offset || location.type != pointer) {
                location = emitValue(Opcode::AddressOffset, pointer, destination, integer(integerType(), entry.offset));
            }
            emit(Opcode::Store, location, initial);
        }
    }

    void Lowerer::declarations(const types::DeclarationList* list, bool global) {
        for (auto* declaration : list->getDeclarations()) {
            auto symbol = declaration->semanticInfo().Symbol;
            if (!symbol) {
                fail(declaration, "declaration lacks a resolved symbol");
            }
            const auto& entry = Analyzer.symbols().get({symbol});
            if (entry.kind == semantic::SymbolKind::Function) {
                continue;
            }
            if (global || entry.isStatic) {
                if (!declaration->getInitializer()) {
                    continue;
                }
                for (auto& object : Result.globals) {
                    if (object.id == symbol) {
                        object.runtimeInitialized = true;
                        break;
                    }
                }
                if (global) {
                    GlobalInitializers.push_back(declaration);
                } else {
                    initializeStatic(symbol, declaration->getInitializer());
                }
            } else {
                initialize(object(symbol), importType(entry.type), declaration->getInitializer());
            }
        }
    }
}
