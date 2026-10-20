#include "lowering.hh"

namespace baka::ir {
    void Lowerer::lowerExpressionRegion(const types::ExpressionNode* expression) {
        if (!expression || ExpressionRegions.count(expression)) {
            return;
        }
        auto entry = newBlock();
        select(entry);
        BuildingExpression = true;
        auto operand = value(expression);
        BuildingExpression = false;
        ExpressionRegions.emplace(expression, ExpressionRegion{entry, CurrentBlock, operand, false});
    }

    // Collect once per function, in source order. Nested expression recursion
    // belongs to the expression emitter; only runtime roots become regions.
    // Array bounds, case values, and sizeof operands are compile-time/unevaluated.
    void Lowerer::collectExpressionRegions(const types::ASTNode* node) {
        if (!node) {
            return;
        }
        if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(node)) {
            for (const auto& child : compound->getStatements()) {
                std::visit([&](auto* item) { collectExpressionRegions(item); }, child);
            }
        } else if (auto* declaration = dynamic_cast<const types::DeclarationList*>(node)) {
            for (auto* item : declaration->getDeclarations()) {
                const auto& symbol = Analyzer.symbols().get({item->semanticInfo().Symbol});
                if (symbol.kind == semantic::SymbolKind::Function || !item->getInitializer()) {
                    continue;
                }
                for (const auto& entry : initializerPlan(importType(symbol.type), item->getInitializer())) {
                    if (Result.type(entry.type).kind != TypeKind::Array) {
                        lowerExpressionRegion(entry.expression);
                    }
                }
            }
        } else if (auto* expression = dynamic_cast<const types::ExpressionStatementNode*>(node)) {
            lowerExpressionRegion(expression->getExpression());
        } else if (auto* result = dynamic_cast<const types::ReturnStatementNode*>(node)) {
            lowerExpressionRegion(result->getExpression());
        } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(node)) {
            lowerExpressionRegion(conditional->getIf()->getCondition());
            collectExpressionRegions(conditional->getIf()->getBody());
            for (auto* arm : conditional->getElseIfs()) {
                lowerExpressionRegion(arm->getCondition());
                collectExpressionRegions(arm->getBody());
            }
            if (conditional->getElse()) {
                collectExpressionRegions(conditional->getElse()->getBody());
            }
        } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(node)) {
            std::visit(
                [&](auto* initializer) {
                    if constexpr (std::is_same_v<std::remove_pointer_t<decltype(initializer)>, types::ExpressionNode>) {
                        lowerExpressionRegion(initializer);
                    } else {
                        collectExpressionRegions(initializer);
                    }
                },
                loop->getInitializer());
            lowerExpressionRegion(loop->getCondition());
            lowerExpressionRegion(loop->getUpdate());
            collectExpressionRegions(loop->getBody());
        } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(node)) {
            lowerExpressionRegion(loop->getCondition());
            collectExpressionRegions(loop->getBody());
        } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(node)) {
            collectExpressionRegions(loop->getBody());
            lowerExpressionRegion(loop->getCondition());
        } else if (auto* selection = dynamic_cast<const types::SwitchBlockStatementNode*>(node)) {
            lowerExpressionRegion(selection->getCondition());
            collectExpressionRegions(selection->getBody());
        } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(node)) {
            collectExpressionRegions(label->getStatement());
        } else if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(node)) {
            collectExpressionRegions(label->getStatement());
        } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(node)) {
            collectExpressionRegions(label->getStatement());
        }
    }
}
