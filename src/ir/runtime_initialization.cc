#include "lowering.hh"

namespace baka {
    namespace ir {
        // Non-local declaration traversal records work without switching the active
        // function builder. Emit it once, after ordinary function bodies, so forward
        // function references are callable and no frontend data is mutated.
        void Lowerer::lowerGlobalInitializers() {
            if (GlobalInitializers.empty()) {
                return;
            }
            Function startup;
            startup.id = NextObject++;
            startup.name = ".module_init";
            startup.signature =
                Result.function(importType(Analyzer.types().builtin(semantic::Builtin::Void)), {}, false);
            startup.defined = startup.internal = true;
            Result.initializer = startup.id;
            Result.functions.push_back(std::move(startup));
            State = FunctionContext{};
            State.FunctionIndex = Result.functions.size() - 1;
            select(newBlock());
            for (auto* declaration : GlobalInitializers) {
                const auto span = declaration->getSourceSpan();
                State.Source = {span.Line, span.Col};
                const auto symbol = declaration->semanticInfo().Symbol;
                auto destination = object(symbol);
                initialize(destination, Analyzer.executionPlans().initialization(Analyzer.symbols().get({symbol}).type,
                                                                                 declaration->getInitializer()));
                registerDestructor(destination, Result.type(destination.type).element);
                finishTemps();
            }
            emit(Opcode::Return);
        }

        void Lowerer::initializeStatic(uint32_t symbol, const types::ExpressionNode* initializer) {
            // Each declaration owns a distinct zero-filled guard. The initializer
            // regions are connected only on the first-use path, so even constant
            // arithmetic executes at first use rather than being pre-filled data.
            Object guard;
            guard.id = NextObject++;
            guard.name = ".static_guard." + std::to_string(symbol);
            guard.type = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
            guard.global = guard.internal = true;
            auto guardId = guard.id;
            auto boolean = guard.type;
            Result.globals.push_back(std::move(guard));
            auto initialized = load(object(guardId), boolean);
            auto initializeBlock = newBlock(), done = newBlock();
            branch(initialized, done, initializeBlock);
            select(initializeBlock);
            auto destination = object(symbol);
            initialize(destination,
                       Analyzer.executionPlans().initialization(Analyzer.symbols().get({symbol}).type, initializer));
            registerDestructor(destination, Result.type(destination.type).element);
            finishTemps();
            // Publish completion only after all initialization side effects/stores.
            // Exceptions and thread-safe/reentrant static initialization are outside
            // the current language/runtime; the guard is an ordinary typed object.
            store(object(guardId), integer(boolean, 1));
            jump(done);
            select(done);
        }
    }
}
