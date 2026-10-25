#include "lowering.hh"
#include <algorithm>

namespace baka {
    namespace ir {
        semantic::TypeId Lowerer::semanticType(TypeId type) const {
            auto found = ReverseTypes.find(type);
            return found == ReverseTypes.end() ? semantic::TypeId{} : found->second;
        }

        bool Lowerer::needsConstruction(TypeId type) const {
            return (Result.type(type).kind == TypeKind::Record || Result.type(type).kind == TypeKind::Array) &&
                   Analyzer.executionPlans().lifetime(semanticType(type)).construction;
        }

        bool Lowerer::needsDestruction(TypeId type) const {
            return (Result.type(type).kind == TypeKind::Record || Result.type(type).kind == TypeKind::Array) &&
                   Analyzer.executionPlans().lifetime(semanticType(type)).destruction;
        }

        Operand Lowerer::storage(TypeId type, std::string name) {
            Object object;
            object.id = NextObject++;
            object.name = std::move(name);
            object.type = type;
            const auto id = object.id;
            function().objects.push_back(std::move(object));
            return this->object(id);
        }

        Operand Lowerer::activeFlag() {
            auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
            auto result = temporary(boolean);
            // Lifetime flags dominate all branches, including paths that skip a
            // construction. Cleanup clears them, so loop iterations can reuse them.
            function().blocks.front().instructions.insert(
                function().blocks.front().instructions.begin(),
                {Opcode::Copy, result, integer(boolean, 0), {}, State.Source});
            return result;
        }

        Operand Lowerer::invoke(uint32_t id, const std::vector<Operand>& arguments) {
            auto resultType = Result.type(Result.functionById(id)->signature).element;
            auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
            emitCall(result, callable(id), std::move(arguments));
            return result;
        }

        uint32_t Lowerer::declareLifetime(TypeId owner, std::string name, std::vector<TypeId> parameters,
                                          const types::ASTNode* declaration, const types::StatementNode* body,
                                          bool destructor, bool variadic) {
            parameters.insert(parameters.begin(), Result.pointer(owner));
            Function function;
            function.id = NextObject++;
            function.name = std::move(name);
            function.signature =
                Result.function(importType(Analyzer.types().builtin(semantic::Builtin::Void)), parameters, variadic);
            function.internal = true;
            auto id = function.id;
            Result.functions.push_back(std::move(function));
            LifetimeBodies.push_back({id, owner, declaration, body, destructor});
            return id;
        }

        void Lowerer::importLifetimes() {
            const auto count = Result.types.size();
            for (TypeId id = 1; id < count; ++id) {
                if (Result.type(id).kind != TypeKind::Record || !Result.type(id).complete) {
                    continue;
                }
                auto sourceId = semanticType(id);
                const auto& source = Analyzer.types().get(sourceId);
                if (source.kind == semantic::TypeKind::Union && (needsConstruction(id) || needsDestruction(id))) {
                    fail(source.members.empty() ? nullptr
                                                : static_cast<const types::ASTNode*>(source.members.front().source),
                         "nontrivial union lifetimes require active-member metadata");
                }
                bool hasConstructor = false;
                for (size_t i = 0; i < source.members.size(); ++i) {
                    const auto& member = source.members[i];
                    if (!member.isConstructor && !member.isDestructor) {
                        continue;
                    }
                    auto* declaration = static_cast<const types::ASTNode*>(member.source);
                    const auto signature = importType(member.type);
                    const types::StatementNode* body =
                        member.isConstructor ? static_cast<const types::ConstructorNode*>(declaration)->getBody()
                                             : static_cast<const types::DestructorNode*>(declaration)->getBody();
                    auto function = declareLifetime(
                        id, source.name + (member.isConstructor ? ".ctor." + std::to_string(i + 1) : ".dtor"),
                        Result.type(signature).parameters, declaration, body, member.isDestructor,
                        Result.type(signature).variadic);
                    SpecialFunctions[{sourceId.value, static_cast<uint32_t>(i + 1)}] = function;
                    if (member.isConstructor) {
                        hasConstructor = true;
                        if (Analyzer.executionPlans().lifetime(sourceId).defaultConstructor.member == i + 1) {
                            DefaultConstructors[id] = function;
                        }
                    } else {
                        Destructors[id] = function;
                    }
                }
                if (!hasConstructor && needsConstruction(id) &&
                    Analyzer.executionPlans().requiresConstructor(sourceId, 0)) {
                    DefaultConstructors[id] =
                        declareLifetime(id, source.name + ".default_ctor", {}, nullptr, nullptr, false);
                }
                if (needsDestruction(id) && !Destructors.count(id)) {
                    Destructors[id] = declareLifetime(id, source.name + ".default_dtor", {}, nullptr, nullptr, true);
                }
            }
            for (TypeId id = 1; id < count; ++id) {
                if (Result.type(id).kind == TypeKind::Array && needsDestruction(id)) {
                    Destructors[id] =
                        declareLifetime(id, ".array_dtor." + std::to_string(id), {}, nullptr, nullptr, true);
                }
            }
        }

        void Lowerer::lowerLifetimes() {
            // Registration is complete before any body is emitted; recursive member
            // calls and base/member destruction can refer to later definitions.
            for (const auto& work : LifetimeBodies) {
                if (!work.destructor) {
                    uint32_t member = 0;
                    if (work.declaration) {
                        const auto& members = Analyzer.types().get(semanticType(work.type)).members;
                        for (size_t i = 0; i < members.size(); ++i) {
                            if (members[i].source == work.declaration) {
                                member = i + 1;
                            }
                        }
                    }
                    if (!Analyzer.executionPlans().requiresConstructor(semanticType(work.type), member)) {
                        continue;
                    }
                }
                lowerCallable(work.declaration, work.body, work.function, semanticType(work.type), !work.destructor,
                              work.destructor);
            }
        }

        void Lowerer::construct(Operand destination, TypeId type, uint32_t selected,
                                std::vector<const types::ExpressionNode*> expressions, uint64_t elements) {
            if (Result.type(type).kind == TypeKind::Array) {
                const auto child = Result.type(type).element;
                forEachElement(destination, type, elements ? elements : Result.type(type).count, false,
                               [&](Operand address) {
                                   construct(address, child);
                                   finishTemps();
                               });
                return;
            }
            if (!selected) {
                auto found = DefaultConstructors.find(type);
                if (found == DefaultConstructors.end()) {
                    if (needsConstruction(type)) {
                        fail(nullptr, "object has no callable default constructor");
                    }
                    return;
                }
                selected = found->second;
            }
            const auto signature = Result.type(Result.functionById(selected)->signature);
            if (expressions.empty()) {
                expressions = Analyzer.executionPlans().lifetime(semanticType(type)).defaultConstructor.arguments;
            }
            std::vector<Operand> arguments{cast(destination, signature.parameters.front())};
            for (size_t i = 0; i < expressions.size(); ++i) {
                auto argument = value(expressions[i]);
                if (i + 1 < signature.parameters.size()) {
                    argument = cast(argument, signature.parameters.at(i + 1));
                    transfer(argument);
                }
                arguments.push_back(argument);
            }
            invoke(selected, arguments);
        }

        void Lowerer::forEachElement(Operand destination, TypeId array, uint64_t count, bool reverse,
                                     const std::function<void(Operand)>& emitElement) {
            const auto element = Result.type(array).element;
            const auto size = Result.type(element).size;
            if (count <= 16) {
                for (uint64_t i = 0; i < count; ++i) {
                    const auto index = reverse ? count - 1 - i : i;
                    emitElement(emitValue(Opcode::AddressOffset, Result.pointer(element), destination,
                                          integer(integerType(), index * size)));
                }
                return;
            }
            auto index = temporary(integerType());
            emit(Opcode::Copy, index, integer(integerType(), 0));
            auto condition = newBlock(), body = newBlock(), done = newBlock();
            jump(condition);
            select(condition);
            auto test = emitValue(Opcode::Less, importType(Analyzer.types().builtin(semantic::Builtin::Bool)), index,
                                  integer(integerType(), count));
            branch(test, body, done);
            select(body);
            auto ordinal =
                reverse ? emitValue(Opcode::Subtract, integerType(), integer(integerType(), count - 1), index) : index;
            auto offset = emitValue(Opcode::Multiply, integerType(), ordinal, integer(integerType(), size));
            emitElement(emitValue(Opcode::AddressOffset, Result.pointer(element), destination, offset));
            auto next = emitValue(Opcode::Add, integerType(), index, integer(integerType(), 1));
            emit(Opcode::Copy, index, next);
            jump(condition);
            select(done);
        }

        void Lowerer::zeroObject(Operand destination, TypeId id, uint64_t bytes) {
            emit(Opcode::ZeroMemory, {}, destination,
                 integer(importType(Analyzer.types().sizeType()), bytes ? bytes : Result.type(id).size));
        }

        void Lowerer::initializeMembers(Operand destination, TypeId id) {
            if (Result.type(id).kind == TypeKind::Array) {
                construct(destination, id);
                return;
            }
            for (const auto& child : Analyzer.executionPlans().lifetime(semanticType(id)).children) {
                auto type = importType(child.type);
                auto address = emitValue(Opcode::AddressOffset, Result.pointer(type), destination,
                                         integer(integerType(), child.offset));
                initialize(address, Analyzer.executionPlans().initialization(child.type, child.initializer));
                finishTemps();
            }
        }

        void Lowerer::destroyMembers(Operand destination, TypeId id) {
            if (Result.type(id).kind == TypeKind::Array) {
                const auto child = Result.type(id).element;
                if (needsDestruction(child)) {
                    forEachElement(destination, id, Result.type(id).count, true,
                                   [&](Operand address) { destroy(address, child); });
                }
                return;
            }
            auto children = subobjects(id);
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (!needsDestruction(it->first)) {
                    continue;
                }
                auto address = emitValue(Opcode::AddressOffset, Result.pointer(it->first), destination,
                                         integer(integerType(), it->second));
                destroy(address, it->first);
            }
        }

        void Lowerer::destroy(Operand destination, TypeId type) {
            if (auto found = Destructors.find(type); found != Destructors.end()) {
                invoke(found->second, {destination});
            }
        }

        void Lowerer::registerDestructor(Operand destination, TypeId type) {
            if (auto found = Destructors.find(type); found != Destructors.end()) {
                emit(Opcode::RegisterDestructor, {}, callable(found->second), destination);
            }
        }

        void Lowerer::cleanup(const Cleanup& entry) {
            if (!needsDestruction(entry.type)) {
                emit(Opcode::Copy, entry.active, integer(entry.active.type, 0));
                return;
            }
            auto yes = newBlock(), done = newBlock();
            branch(entry.active, yes, done);
            select(yes);
            emit(Opcode::Copy, entry.active, integer(entry.active.type, 0));
            destroy(entry.address, entry.type);
            jump(done);
            select(done);
        }

        void Lowerer::cleanupTo(size_t depth) {
            for (size_t i = State.ActiveCleanups.size(); i > depth; --i) {
                cleanup(State.LocalCleanups.at(State.ActiveCleanups[i - 1]));
            }
        }

        void Lowerer::finishTemps() {
            auto& pending = State.PendingTemps;
            auto cleanups = std::move(pending);
            pending.clear();
            for (auto it = cleanups.rbegin(); it != cleanups.rend(); ++it) {
                cleanup(*it);
            }
        }

        Operand Lowerer::ownTemporary(Operand captured, Operand location) {
            if (!needsDestruction(captured.type)) {
                return captured;
            }
            if (!location) {
                location = storage(captured.type, ".temporary");
                store(location, captured);
            }
            auto flag = activeFlag();
            emit(Opcode::Copy, flag, integer(flag.type, 1));
            Cleanup entry{location, captured.type, flag};
            State.PendingTemps.push_back(entry);
            State.ValueOwners[captured.id] = {entry};
            State.ValueLocations[captured.id] = location;
            return captured;
        }

        void Lowerer::transfer(Operand value) {
            if (value.kind != OperandKind::Temporary) {
                return;
            }
            for (const auto& entry : State.ValueOwners[value.id]) {
                emit(Opcode::Copy, entry.active, integer(entry.active.type, 0));
            }
        }

        const types::ExpressionNode* Lowerer::valueRoot(const types::ExpressionNode* expression) const {
            for (;;) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(expression);
                    list && list->expressions().size() == 1) {
                    expression = list->expressions().front();
                } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression);
                           comma && comma->getExpressions().size() == 1) {
                    expression = comma->getExpressions().front();
                } else if (auto* cast = dynamic_cast<const types::ImplicitCastNode*>(expression)) {
                    if (cast->getConversionKind() == types::ImplicitConversionKind::ConstructorConversion) {
                        return expression;
                    }
                    expression = cast->getOperand();
                } else {
                    return expression;
                }
            }
        }

        Operand Lowerer::destination(const types::ExpressionNode* expression) {
            if (auto found = State.DirectDestinations.find(expression); found != State.DirectDestinations.end()) {
                return found->second;
            }
            return {};
        }

    }
}
