#include "lowering.hh"

namespace baka {
    namespace ir {
        void Lowerer::returnStatement(const types::ReturnStatementNode* result) {
            auto operand = value(result->getExpression());
            transfer(operand);
            finishTemps();
            cleanupExit(result);
            if (State.InDestructor) {
                destroyMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
            }
            emit(Opcode::Return, {}, operand);
        }

        void Lowerer::gotoStatement(const types::GotoStatementNode* jump) {
            auto target = jump->semanticInfo().Target;
            if (!target) {
                fail(jump, "goto lacks resolved semantic target");
            }
            if (!State.Labels.count(target)) {
                State.Labels[target] = newBlock();
            }
            cleanupExit(jump);
            this->jump(State.Labels.at(target));
        }

        void Lowerer::breakStatement(const types::BreakStatementNode* jump) {
            if (!State.Breaks.count(jump->semanticInfo().Target)) {
                fail(jump, "break lacks an active resolved IR target");
            }
            cleanupExit(jump);
            this->jump(State.Breaks.at(jump->semanticInfo().Target));
        }

        void Lowerer::continueStatement(const types::ContinueStatementNode* jump) {
            if (!State.Continues.count(jump->semanticInfo().Target)) {
                fail(jump, "continue lacks an active resolved IR target");
            }
            cleanupExit(jump);
            this->jump(State.Continues.at(jump->semanticInfo().Target));
        }

        void Lowerer::gotoLabel(const types::GotoLabelStatementNode* label) {
            auto target = label->semanticInfo().Target;
            if (!State.Labels.count(target)) {
                State.Labels[target] = newBlock();
            }
            link(State.Labels.at(target));
            select(State.Labels.at(target));
            this->statement(label->getStatement());
        }

        void Lowerer::cleanupExit(const types::StatementNode* source) {
            for (auto id : Analyzer.executionPlans().cleanup(source)) {
                cleanup(State.LocalCleanups.at(id.value));
            }
        }
    }
}
