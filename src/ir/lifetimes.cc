#include "lowering.hh"
#include <algorithm>

namespace baka::ir {
    semantic::TypeId Lowerer::semanticType(TypeId type) const {
        for (auto [source, imported] : TypeMap) {
            if (imported == type) {
                return Analyzer.types().unConst({source});
            }
        }
        return {};
    }

    bool Lowerer::needsConstruction(TypeId id) const {
        const auto& type = Result.type(id);
        if (type.kind == TypeKind::Array) {
            return needsConstruction(type.element);
        }
        if (type.kind != TypeKind::Record || !type.complete) {
            return false;
        }
        const auto& source = Analyzer.types().get(semanticType(id));
        if (source.base.value && needsConstruction(TypeMap.at(Analyzer.types().unConst(source.base).value))) {
            return true;
        }
        for (const auto& member : source.members) {
            if (member.isConstructor) {
                return true;
            }
            if (member.isStatic || member.isType || member.isDestructor ||
                Analyzer.types().get(Analyzer.types().unConst(member.type)).kind == semantic::TypeKind::Function) {
                continue;
            }
            auto* declaration =
                dynamic_cast<const types::SingleDeclarationNode*>(static_cast<const types::ASTNode*>(member.source));
            if ((declaration && declaration->getInitializer()) ||
                needsConstruction(TypeMap.at(Analyzer.types().unConst(member.type).value))) {
                return true;
            }
        }
        return false;
    }

    bool Lowerer::needsDestruction(TypeId id) const {
        const auto& type = Result.type(id);
        if (type.kind == TypeKind::Array) {
            return needsDestruction(type.element);
        }
        if (type.kind != TypeKind::Record || !type.complete) {
            return false;
        }
        const auto& source = Analyzer.types().get(semanticType(id));
        if (source.base.value && needsDestruction(TypeMap.at(Analyzer.types().unConst(source.base).value))) {
            return true;
        }
        for (const auto& member : source.members) {
            if (member.isDestructor) {
                return true;
            }
            if (!member.isStatic && !member.isType && !member.isConstructor &&
                needsDestruction(TypeMap.at(Analyzer.types().unConst(member.type).value))) {
                return true;
            }
        }
        return false;
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
        function().blocks.front().instructions.insert(function().blocks.front().instructions.begin(),
                                                      {Opcode::Copy, result, integer(boolean, 0), {}, Source});
        return result;
    }

    Operand Lowerer::invoke(uint32_t id, const std::vector<Operand>& arguments) {
        for (size_t i = 0; i < arguments.size(); ++i) {
            emit(Opcode::Argument, {}, arguments[i], integer(integerType(), i));
        }
        auto resultType = Result.type(Result.functionById(id)->signature).element;
        auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
        emit(Opcode::Call, result, callable(id), integer(integerType(), arguments.size()));
        return result;
    }

    void Lowerer::importLifetimes() {
        auto declare = [&](TypeId owner, std::string name, std::vector<TypeId> parameters,
                           const types::ASTNode* declaration, const types::StatementNode* body, bool destructor,
                           bool variadic = false) {
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
        };
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
                auto function =
                    declare(id, source.name + (member.isConstructor ? ".ctor." + std::to_string(i + 1) : ".dtor"),
                            Result.type(signature).parameters, declaration, body, member.isDestructor,
                            Result.type(signature).variadic);
                SpecialFunctions[{sourceId.value, static_cast<uint32_t>(i + 1)}] = function;
                if (member.isConstructor) {
                    hasConstructor = true;
                    if (member.minimumArity == 0 && !DefaultConstructors.count(id)) {
                        DefaultConstructors[id] = function;
                    }
                } else {
                    Destructors[id] = function;
                }
            }
            if (!hasConstructor && needsConstruction(id)) {
                DefaultConstructors[id] = declare(id, source.name + ".default_ctor", {}, nullptr, nullptr, false);
            }
            if (needsDestruction(id) && !Destructors.count(id)) {
                Destructors[id] = declare(id, source.name + ".default_dtor", {}, nullptr, nullptr, true);
            }
        }
        for (TypeId id = 1; id < count; ++id) {
            if (Result.type(id).kind == TypeKind::Array && needsDestruction(id)) {
                Destructors[id] = declare(id, ".array_dtor." + std::to_string(id), {}, nullptr, nullptr, true);
            }
        }
    }

    void Lowerer::lowerLifetimes() {
        // Registration is complete before any body is emitted; recursive member
        // calls and base/member destruction can refer to later definitions.
        for (const auto& work : LifetimeBodies) {
            lowerCallable(work.declaration, work.body, work.function, semanticType(work.type), !work.destructor,
                          work.destructor);
        }
    }

    void Lowerer::construct(Operand destination, TypeId type, uint32_t selected,
                            std::vector<const types::ExpressionNode*> expressions) {
        if (Result.type(type).kind == TypeKind::Array) {
            for (auto [child, offset] : subobjects(type)) {
                construct(emitValue(Opcode::AddressOffset, Result.pointer(child), destination,
                                    integer(integerType(), offset)),
                          child);
                // Default-argument temporaries for one array element end before
                // the next element's default constructor is invoked.
                finishTemps();
            }
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
        const types::ConstructorNode* declaration = nullptr;
        for (const auto& work : LifetimeBodies) {
            if (work.function == selected) {
                declaration = dynamic_cast<const types::ConstructorNode*>(work.declaration);
            }
        }
        const auto signature = Result.type(Result.functionById(selected)->signature);
        while (expressions.size() + 1 < signature.parameters.size()) {
            auto* parameter = declaration
                                  ? declaration->getArgs()->getParameters().at(expressions.size())->getDeclaration()
                                  : nullptr;
            if (!parameter || !parameter->getInitializer()) {
                fail(declaration, "constructor lacks a checked default argument");
            }
            expressions.push_back(parameter->getInitializer());
        }
        std::vector<Operand> arguments{cast(destination, signature.parameters.front())};
        const bool previous = BuildingExpression;
        BuildingExpression = true;
        for (size_t i = 0; i < expressions.size(); ++i) {
            auto argument = value(expressions[i]);
            if (i + 1 < signature.parameters.size()) {
                argument = cast(argument, signature.parameters.at(i + 1));
                transfer(argument);
            }
            arguments.push_back(argument);
        }
        BuildingExpression = previous;
        if (!previous) {
            PendingTemps.insert(PendingTemps.end(), BuildingTemps.begin(), BuildingTemps.end());
            BuildingTemps.clear();
        }
        invoke(selected, arguments);
    }

    void Lowerer::zeroObject(Operand destination, TypeId id) {
        auto kind = Result.type(id).kind;
        if (kind == TypeKind::Record || kind == TypeKind::Array) {
            for (auto [child, offset] : subobjects(id)) {
                zeroObject(emitValue(Opcode::AddressOffset, Result.pointer(child), destination,
                                     integer(integerType(), offset)),
                           child);
            }
        } else {
            auto zero = kind == TypeKind::Floating ? Operand{OperandKind::Floating, id} : integer(id, 0);
            emit(Opcode::Store, destination, zero);
        }
    }

    void Lowerer::initializeMembers(Operand destination, TypeId id) {
        if (Result.type(id).kind == TypeKind::Array) {
            construct(destination, id);
            return;
        }
        const auto source = semanticType(id);
        const auto& record = Analyzer.types().get(source);
        const auto layout = Analyzer.types().layout(source);
        for (const auto& child : Analyzer.types().subobjects(source)) {
            auto offset = child.memberIndex ? layout.layout.offsets.at(*child.memberIndex) : layout.layout.baseOffset;
            auto type = importType(child.type);
            auto address =
                emitValue(Opcode::AddressOffset, Result.pointer(type), destination, integer(integerType(), *offset));
            const types::ExpressionNode* initializer = nullptr;
            if (child.memberIndex) {
                const auto& member = record.members.at(*child.memberIndex);
                auto* declaration = dynamic_cast<const types::SingleDeclarationNode*>(
                    static_cast<const types::ASTNode*>(member.source));
                if (declaration) {
                    initializer = declaration->getInitializer();
                }
            }
            initialize(address, type, initializer);
            finishTemps();
            if (record.kind == semantic::TypeKind::Union) {
                break;
            }
        }
    }

    void Lowerer::destroyMembers(Operand destination, TypeId id) {
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
        for (size_t i = ActiveCleanups.size(); i > depth; --i) {
            cleanup(LocalCleanups.at(ActiveCleanups[i - 1]));
        }
    }

    void Lowerer::finishTemps() {
        auto& pending = BuildingExpression ? BuildingTemps : PendingTemps;
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
            emit(Opcode::Store, location, captured);
        }
        auto flag = activeFlag();
        emit(Opcode::Copy, flag, integer(flag.type, 1));
        Cleanup entry{location, captured.type, flag};
        BuildingTemps.push_back(entry);
        ValueOwners[captured.id] = {entry};
        ValueLocations[captured.id] = location;
        return captured;
    }

    void Lowerer::transfer(Operand value) {
        if (value.kind != OperandKind::Temporary) {
            return;
        }
        for (const auto& entry : ValueOwners[value.id]) {
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
                expression = cast->getOperand();
            } else {
                return expression;
            }
        }
    }

    Operand Lowerer::destination(const types::ExpressionNode* expression) {
        if (auto found = DirectDestinations.find(expression); found != DirectDestinations.end()) {
            return found->second;
        }
        auto found = Destinations.find(expression);
        if (found == Destinations.end()) {
            return {};
        }
        const auto& plan = found->second;
        auto result = object(plan.object);
        if (plan.offset || Result.type(result.type).element != plan.type) {
            result = emitValue(Opcode::AddressOffset, Result.pointer(plan.type), result,
                               integer(integerType(), plan.offset));
        }
        return result;
    }

    void Lowerer::planCleanupTargets(const types::ASTNode* node, std::vector<uint32_t> active) {
        if (!node) {
            return;
        }
        auto append = [&](const types::DeclarationList* list) {
            for (auto* declaration : list->getDeclarations()) {
                auto id = declaration->semanticInfo().Symbol;
                if (LocalCleanups.count(id)) {
                    active.push_back(id);
                }
            }
        };
        if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(node)) {
            for (const auto& child : compound->getStatements()) {
                std::visit(
                    [&](auto* item) {
                        planCleanupTargets(item, active);
                        if (auto* list = dynamic_cast<const types::DeclarationList*>(item)) {
                            append(list);
                        }
                    },
                    child);
            }
        } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(node)) {
            LabelCleanups[label->semanticInfo().Target] = active;
            planCleanupTargets(label->getStatement(), active);
        } else if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(node)) {
            CaseCleanups[label] = active;
            planCleanupTargets(label->getStatement(), active);
        } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(node)) {
            CaseCleanups[label] = active;
            planCleanupTargets(label->getStatement(), active);
        } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(node)) {
            planCleanupTargets(conditional->getIf()->getBody(), active);
            for (auto* arm : conditional->getElseIfs()) {
                planCleanupTargets(arm->getBody(), active);
            }
            if (conditional->getElse()) {
                planCleanupTargets(conditional->getElse()->getBody(), active);
            }
        } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(node)) {
            std::visit(
                [&](auto* initializer) {
                    if (auto* list = dynamic_cast<const types::DeclarationList*>(initializer)) {
                        append(list);
                    }
                },
                loop->getInitializer());
            planCleanupTargets(loop->getBody(), active);
        } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(node)) {
            planCleanupTargets(loop->getBody(), active);
        } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(node)) {
            planCleanupTargets(loop->getBody(), active);
        } else if (auto* selection = dynamic_cast<const types::SwitchBlockStatementNode*>(node)) {
            planCleanupTargets(selection->getBody(), active);
        }
    }
}
