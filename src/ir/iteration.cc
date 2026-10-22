#include "lowering.hh"

namespace baka {
    namespace ir {
        void Lowerer::forStatement(const types::ForBlockStatementNode* loop) {
            const auto before = State.ActiveCleanups.size();
            std::visit(
                [&](auto* initializer) {
                    if constexpr (std::is_same_v<std::remove_pointer_t<decltype(initializer)>, types::ExpressionNode>) {
                        value(initializer);
                        finishTemps();
                    } else {
                        node(initializer);
                    }
                },
                loop->getInitializer());
            auto condition = newBlock(), body = newBlock(), update = newBlock(), exit = newBlock();
            State.Breaks[loop->semanticInfo().Target] = exit;
            State.Continues[loop->semanticInfo().Target] = update;
            link(condition);
            select(condition);
            if (loop->getCondition()) {
                auto test = value(loop->getCondition());
                finishTemps();
                branch(test, body, exit);
            } else {
                jump(body);
            }
            select(body);
            this->statement(loop->getBody());
            link(update);
            select(update);
            value(loop->getUpdate());
            finishTemps();
            link(condition);
            select(exit);
            cleanupTo(before);
            State.ActiveCleanups.resize(before);
        }

        void Lowerer::whileStatement(const types::WhileBlockStatementNode* loop) {
            auto condition = newBlock(), body = newBlock(), exit = newBlock();
            State.Breaks[loop->semanticInfo().Target] = exit;
            State.Continues[loop->semanticInfo().Target] = condition;
            link(condition);
            select(condition);
            auto test = value(loop->getCondition());
            finishTemps();
            branch(test, body, exit);
            select(body);
            this->statement(loop->getBody());
            link(condition);
            select(exit);
        }

        void Lowerer::doWhileStatement(const types::DoWhileBlockStatementNode* loop) {
            auto body = newBlock(), condition = newBlock(), exit = newBlock();
            State.Breaks[loop->semanticInfo().Target] = exit;
            State.Continues[loop->semanticInfo().Target] = condition;
            link(body);
            select(body);
            this->statement(loop->getBody());
            link(condition);
            select(condition);
            auto test = value(loop->getCondition());
            finishTemps();
            branch(test, body, exit);
            select(exit);
        }
    }
}
