#include "lowering.hh"
#include <algorithm>

namespace baka {
    namespace ir {
        void Lowerer::lowerFunction(const types::FunctionNode* declaration) {
            if (!declaration->getBody()) {
                return;
            }
            auto symbol = declaration->semanticInfo().Symbol;
            auto binding = Members.find(symbol);
            lowerCallable(declaration, declaration->getBody(), symbol,
                          binding != Members.end() && !binding->second.isStatic
                              ? semantic::TypeId{binding->second.owner}
                              : semantic::TypeId{});
        }

        void Lowerer::lowerCallable(const types::ASTNode* declaration, const types::StatementNode* body,
                                    uint32_t symbol, semantic::TypeId owner, bool constructor, bool destructor) {
            auto it = std::find_if(Result.functions.begin(), Result.functions.end(),
                                   [&](const Function& item) { return item.id == symbol; });
            if (it == Result.functions.end()) {
                fail(declaration, "function definition has no IR declaration");
            }
            State = FunctionContext{};
            State.FunctionIndex = static_cast<size_t>(it - Result.functions.begin());
            function().defined = true;
            State.CurrentOwner = owner;
            State.InDestructor = destructor;
            auto functionScope = declaration ? declaration->semanticInfo().Scope : 0;
            for (auto id : Analyzer.executionPlans().locals({functionScope})) {
                const auto i = id.value;
                const auto& symbol = Analyzer.symbols().get(id);
                Object object;
                object.id = i;
                object.name = symbol.name;
                object.type = importType(symbol.type);
                function().objects.push_back(std::move(object));
                if (symbol.kind == semantic::SymbolKind::Parameter && symbol.scope.value == functionScope) {
                    function().parameters.push_back(i);
                    if (symbol.name == "this") {
                        State.Self = i;
                    }
                }
            }
            if (owner.value && !State.Self) {
                auto receiver = storage(Result.type(function().signature).parameters.front(), "this");
                State.Self = receiver.id;
                function().parameters.push_back(State.Self);
            }
            select(newBlock());
            for (const auto& local : function().objects) {
                if (!needsDestruction(local.type) && !needsConstruction(local.type)) {
                    continue;
                }
                auto flag = activeFlag();
                State.LocalCleanups[local.id] = {object(local.id), local.type, flag};
                if (std::find(function().parameters.begin(), function().parameters.end(), local.id) !=
                    function().parameters.end()) {
                    emit(Opcode::Copy, flag, integer(flag.type, 1));
                    State.ActiveCleanups.push_back(local.id);
                }
            }
            if (constructor) {
                initializeMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
                finishTemps();
            }
            statement(body);
            auto signature = Result.type(function().signature);
            if (block().instructions.empty() || !IsTerminator(block().instructions.back().opcode)) {
                Operand result;
                const auto& type = Result.type(signature.element);
                if (type.kind == TypeKind::Floating) {
                    result = {OperandKind::Floating, signature.element, 0, 0, 0};
                } else if (type.kind == TypeKind::Record) {
                    Object storage;
                    storage.id = NextObject++;
                    storage.name = ".fallthrough";
                    storage.type = signature.element;
                    auto id = storage.id;
                    function().objects.push_back(std::move(storage));
                    zeroObject(object(id), signature.element);
                    result = load(object(id), signature.element);
                } else if (type.kind != TypeKind::Void) {
                    result = integer(signature.element, 0);
                }
                cleanupTo(0);
                if (State.InDestructor) {
                    destroyMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
                }
                emit(Opcode::Return, {}, result);
            }
        }
    }
}
