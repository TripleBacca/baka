#include "lowering.hh"

namespace baka {
    namespace ir {
        void Lowerer::ifStatement(const types::IfSuperBlockStatementNode* conditional) {
            std::vector<std::pair<const types::ExpressionNode*, const types::StatementNode*>> arms;
            arms.push_back({conditional->getIf()->getCondition(), conditional->getIf()->getBody()});
            for (auto* arm : conditional->getElseIfs()) {
                arms.push_back({arm->getCondition(), arm->getBody()});
            }
            auto join = newBlock();
            for (const auto& [condition, body] : arms) {
                auto conditionValue = value(condition);
                finishTemps();
                auto yes = newBlock(), no = newBlock();
                branch(conditionValue, yes, no);
                select(yes);
                this->statement(body);
                link(join);
                select(no);
            }
            if (conditional->getElse()) {
                this->statement(conditional->getElse()->getBody());
            }
            link(join);
            select(join);
        }

        void Lowerer::switchStatement(const types::SwitchBlockStatementNode* selection) {
            auto condition = value(selection->getCondition());
            finishTemps();
            auto exit = newBlock();
            State.Breaks[selection->semanticInfo().Target] = exit;
            std::vector<const types::CaseLabelStatementNode*> cases;
            const types::DefaultLabelStatementNode* fallback = nullptr;
            collectCases(selection->getBody(), cases, fallback);
            for (auto* label : cases) {
                const auto& metadata = label->getTag()->semanticInfo();
                if (!metadata.HasConstant) {
                    fail(label, "case label lacks an integral semantic constant");
                }
                auto next = newBlock();
                auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
                auto equal = emitValue(Opcode::Equal, boolean, condition,
                                       integer(condition.type, static_cast<uint64_t>(metadata.Constant)));
                branch(equal, State.Cases.at(label), next);
                select(next);
            }
            jump(fallback ? State.Cases.at(fallback) : exit);
            // The lexical pre-case region is deliberately disconnected from switch
            // dispatch, but goto can still reach labels within it.
            select(newBlock());
            this->statement(selection->getBody());
            link(exit);
            select(exit);
        }

        void Lowerer::collectCases(const types::StatementNode* statement,
                                   std::vector<const types::CaseLabelStatementNode*>& cases,
                                   const types::DefaultLabelStatementNode*& fallback) {
            if (!statement || dynamic_cast<const types::SwitchBlockStatementNode*>(statement)) {
                return;
            }
            if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(statement)) {
                State.Cases[label] = newBlock();
                cases.push_back(label);
                collectCases(label->getStatement(), cases, fallback);
            } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(statement)) {
                State.Cases[label] = newBlock();
                fallback = label;
                collectCases(label->getStatement(), cases, fallback);
            } else if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(statement)) {
                for (const auto& item : compound->getStatements()) {
                    std::visit(
                        [&](auto* child) {
                            if (auto* stmt = dynamic_cast<const types::StatementNode*>(child)) {
                                collectCases(stmt, cases, fallback);
                            }
                        },
                        item);
                }
            } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(statement)) {
                collectCases(conditional->getIf()->getBody(), cases, fallback);
                for (auto* arm : conditional->getElseIfs()) {
                    collectCases(arm->getBody(), cases, fallback);
                }
                if (conditional->getElse()) {
                    collectCases(conditional->getElse()->getBody(), cases, fallback);
                }
            } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(statement)) {
                collectCases(loop->getBody(), cases, fallback);
            } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(statement)) {
                collectCases(loop->getBody(), cases, fallback);
            } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(statement)) {
                collectCases(loop->getBody(), cases, fallback);
            } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(statement)) {
                collectCases(label->getStatement(), cases, fallback);
            }
        }

        void Lowerer::caseStatement(const types::CaseLabelStatementNode* label) {
            if (!State.Cases.count(label)) {
                fail(label, "case label has no owning IR switch");
            }
            link(State.Cases.at(label));
            select(State.Cases.at(label));
            this->statement(label->getStatement());
        }

        void Lowerer::defaultStatement(const types::DefaultLabelStatementNode* label) {
            if (!State.Cases.count(label)) {
                fail(label, "default label has no owning IR switch");
            }
            link(State.Cases.at(label));
            select(State.Cases.at(label));
            this->statement(label->getStatement());
        }
    }
}
