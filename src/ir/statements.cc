#include "lowering.hh"

namespace baka {
    namespace ir {
        void Lowerer::node(const types::ASTNode* node) {
            if (!node) {
                return;
            }
            if (auto* stmt = dynamic_cast<const types::StatementNode*>(node)) {
                statement(stmt);
            } else if (auto* declaration = dynamic_cast<const types::DeclarationList*>(node)) {
                declarations(declaration);
            } else if (dynamic_cast<const types::TypedefNode*>(node)) {
                return;
            } else {
                fail(node, "unsupported declaration in an IR function body");
            }
        }

        void Lowerer::statement(const types::StatementNode* statement) {
            if (!statement) {
                return;
            }
            auto span = statement->getSourceSpan();
            if (span.Valid) {
                State.Source = {span.Line, span.Col};
            }
            if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(statement)) {
                compoundStatement(compound);
            } else if (dynamic_cast<const types::EmptyStatementNode*>(statement)) {
                return;
            } else if (auto* expression = dynamic_cast<const types::ExpressionStatementNode*>(statement)) {
                value(expression->getExpression());
                finishTemps();
            } else if (auto* result = dynamic_cast<const types::ReturnStatementNode*>(statement)) {
                returnStatement(result);
            } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(statement)) {
                ifStatement(conditional);
            } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(statement)) {
                forStatement(loop);
            } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(statement)) {
                whileStatement(loop);
            } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(statement)) {
                doWhileStatement(loop);
            } else if (auto* selection = dynamic_cast<const types::SwitchBlockStatementNode*>(statement)) {
                switchStatement(selection);
            } else if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(statement)) {
                caseStatement(label);
            } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(statement)) {
                defaultStatement(label);
            } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(statement)) {
                gotoLabel(label);
            } else if (auto* jump = dynamic_cast<const types::GotoStatementNode*>(statement)) {
                gotoStatement(jump);
            } else if (auto* jump = dynamic_cast<const types::BreakStatementNode*>(statement)) {
                breakStatement(jump);
            } else if (auto* jump = dynamic_cast<const types::ContinueStatementNode*>(statement)) {
                continueStatement(jump);
            } else {
                fail(statement, "unsupported statement node in IR lowering");
            }
        }

        void Lowerer::compoundStatement(const types::CompoundStatementNode* compound) {
            const auto depth = State.ActiveCleanups.size();
            for (const auto& item : compound->getStatements()) {
                std::visit([&](auto* child) { node(child); }, item);
            }
            if (block().instructions.empty() || !IsTerminator(block().instructions.back().opcode)) {
                cleanupTo(depth);
            }
            State.ActiveCleanups.resize(depth);
        }
    }
}
