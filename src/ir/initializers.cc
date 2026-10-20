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
            if (!expression && info.kind == TypeKind::Record && needsConstruction(type)) {
                result.push_back({offset, type, nullptr});
                return;
            }
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
            if (info.kind == TypeKind::Record && list) {
                const auto& record = Analyzer.types().get(semanticType(type));
                bool constructors = false;
                for (const auto& member : record.members) {
                    constructors |= member.isConstructor;
                }
                if (constructors) {
                    if (!list->expressions().empty()) {
                        fail(list,
                             "braced class construction lacks selected-constructor metadata; use Type(arguments)");
                    }
                    result.push_back({offset, type, nullptr});
                    return;
                }
            }
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
            size_t childIndex = 0;
            for (auto [child, childOffset] : children) {
                const types::ExpressionNode* fallback = nullptr;
                if (info.kind == TypeKind::Record) {
                    auto source = semanticType(type);
                    const auto sourceChildren = Analyzer.types().subobjects(source);
                    if (childIndex < sourceChildren.size() && sourceChildren[childIndex].memberIndex) {
                        const auto& member =
                            Analyzer.types().get(source).members.at(*sourceChildren[childIndex].memberIndex);
                        auto* declaration = dynamic_cast<const types::SingleDeclarationNode*>(
                            static_cast<const types::ASTNode*>(member.source));
                        if (declaration) {
                            fallback = declaration->getInitializer();
                        }
                    }
                }
                if (list && position < list->expressions().size()) {
                    consume(child, offset + childOffset, list->expressions(), position);
                } else {
                    auto start = result.size();
                    whole(child, offset + childOffset, fallback);
                    if (fallback) {
                        for (size_t index = start; index < result.size(); ++index) {
                            if (!result[index].owner) {
                                result[index].owner = type;
                                result[index].receiverOffset = offset;
                            }
                        }
                    }
                }
                ++childIndex;
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
            if (needsConstruction(target)) {
                construct(destination, target);
            }
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
            if (info.kind == TypeKind::Record && !entry.expression) {
                auto location = emitValue(Opcode::AddressOffset, Result.pointer(entry.type), destination,
                                          integer(integerType(), entry.offset));
                construct(location, entry.type);
                continue;
            }
            if (entry.expression) {
                auto previousSelf = SelfOverride;
                auto previousOwner = CurrentOwner;
                if (BuildingExpression && entry.owner) {
                    SelfOverride = emitValue(Opcode::AddressOffset, Result.pointer(entry.owner), destination,
                                             integer(integerType(), entry.receiverOffset));
                    CurrentOwner = semanticType(entry.owner);
                }
                if (BuildingExpression && info.kind == TypeKind::Record) {
                    auto location = destination;
                    if (entry.offset || Result.type(location.type).element != entry.type) {
                        location = emitValue(Opcode::AddressOffset, Result.pointer(entry.type), destination,
                                             integer(integerType(), entry.offset));
                    }
                    DirectDestinations[valueRoot(entry.expression)] = location;
                }
                initial = cast(value(entry.expression), entry.type);
                if (BuildingExpression && info.kind == TypeKind::Record) {
                    DirectDestinations.erase(valueRoot(entry.expression));
                }
                SelfOverride = previousSelf;
                CurrentOwner = previousOwner;
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
            const auto span = declaration->getSourceSpan();
            if (span.Valid) {
                Source = {span.Line, span.Col};
            }
            auto symbol = declaration->semanticInfo().Symbol;
            if (!symbol) {
                fail(declaration, "declaration lacks a resolved symbol");
            }
            const auto& entry = Analyzer.symbols().get({symbol});
            if (entry.kind == semantic::SymbolKind::Function) {
                continue;
            }
            if (global || entry.isStatic) {
                if (!declaration->getInitializer() && !needsConstruction(importType(entry.type)) &&
                    !needsDestruction(importType(entry.type))) {
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
                if (auto found = LocalCleanups.find(symbol); found != LocalCleanups.end()) {
                    emit(Opcode::Copy, found->second.active, integer(found->second.active.type, 1));
                    ActiveCleanups.push_back(symbol);
                }
                finishTemps();
            }
        }
    }
}
