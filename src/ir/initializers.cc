#include "lowering.hh"
#include "base/scoped_value.hh"

namespace baka {
    namespace ir {
        std::vector<std::pair<TypeId, uint64_t>> Lowerer::subobjects(TypeId id) {
            std::vector<std::pair<TypeId, uint64_t>> result;
            for (const auto& child : Analyzer.executionPlans().lifetime(semanticType(id)).children) {
                result.push_back({importType(child.type), child.offset});
            }
            return result;
        }

        void Lowerer::initialize(Operand destination, const semantic::InitializationPlan& plan) {
            for (const auto& entry : plan.steps) {
                const auto type = importType(entry.type);
                const auto info = Result.type(type);
                auto location = destination;
                if (entry.offset || location.type != Result.pointer(type)) {
                    location = emitValue(Opcode::AddressOffset, Result.pointer(type), destination,
                                         integer(integerType(), entry.offset));
                }
                switch (entry.kind) {
                case semantic::InitializationKind::Zero:
                    zeroObject(location, type, entry.byteCount);
                    break;
                case semantic::InitializationKind::Construct:
                    construct(location, type, 0, {}, entry.elements);
                    break;
                case semantic::InitializationKind::String: {
                    zeroObject(location, type);
                    for (uint64_t i = 0; i < entry.bytes.size() && i < info.count; ++i) {
                        auto address = emitValue(Opcode::AddressOffset, Result.pointer(info.element), location,
                                                 integer(integerType(), i));
                        store(address, integer(info.element, static_cast<unsigned char>(entry.bytes[i])));
                    }
                    break;
                }
                case semantic::InitializationKind::Value: {
                    base::ScopedValue receiver(State.SelfOverride);
                    base::ScopedValue owner(State.CurrentOwner);
                    if (entry.receiver.value) {
                        State.SelfOverride =
                            emitValue(Opcode::AddressOffset, Result.pointer(importType(entry.receiver)), destination,
                                      integer(integerType(), entry.receiverOffset));
                        State.CurrentOwner = entry.receiver;
                    }
                    const auto* root = valueRoot(entry.expression);
                    const auto previous = State.DirectDestinations.find(root);
                    const auto saved = previous == State.DirectDestinations.end() ? Operand{} : previous->second;
                    if (info.kind == TypeKind::Record) {
                        State.DirectDestinations[root] = location;
                    }
                    auto initial = cast(value(entry.expression), type);
                    if (info.kind == TypeKind::Record) {
                        if (saved) {
                            State.DirectDestinations[root] = saved;
                        } else {
                            State.DirectDestinations.erase(root);
                        }
                    }
                    store(location, initial);
                    break;
                }
                }
            }
        }

        void Lowerer::declarations(const types::DeclarationList* list, bool global) {
            for (auto* declaration : list->getDeclarations()) {
                const auto span = declaration->getSourceSpan();
                if (span.Valid) {
                    State.Source = {span.Line, span.Col};
                }
                const auto symbol = declaration->semanticInfo().Symbol;
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
                    initialize(object(symbol),
                               Analyzer.executionPlans().initialization(entry.type, declaration->getInitializer()));
                    if (auto found = State.LocalCleanups.find(symbol); found != State.LocalCleanups.end()) {
                        emit(Opcode::Copy, found->second.active, integer(found->second.active.type, 1));
                        State.ActiveCleanups.push_back(symbol);
                    }
                    finishTemps();
                }
            }
        }
    }
}
