#include "semantic_analyzer.hh"
#include <algorithm>
#include <unordered_map>

namespace baka::semantic {
    namespace {
        int64_t CaseValue(int64_t value, TypeId type, const Types& types) {
            auto info = types.arithmetic(type);
            if (!info || info->bits < 32) info = types.arithmetic(types.builtin(Builtin::Int));
            if (info->bits == 64) return value;
            auto bits = static_cast<uint64_t>(value) & ((uint64_t{1} << info->bits) - 1);
            if (info->isSigned && (bits & (uint64_t{1} << (info->bits - 1))))
                bits |= ~((uint64_t{1} << info->bits) - 1);
            return static_cast<int64_t>(bits);
        }
    }

    // This method is const by construction. It cannot resolve names, register
    // declarations, annotate nodes, infer types, or evaluate expressions. All
    // expression facts and control-flow targets arrive from the earlier queues.
    std::vector<Diagnostic> SemanticAnalyzer::checkReadOnly() const {
        std::vector<Diagnostic> diagnostics;
        auto report = [&](std::string code, std::string message, const types::ASTNode* source,
                          const types::ASTNode* previous = nullptr) {
            diagnostics.push_back({std::move(code), std::move(message), source, previous});
        };
        for (const auto& [node, message] : ResolutionErrors) report("control-flow", message, node);
        for (auto* node : InvalidNodes) report("function", "nested functions or unsupported statement are not allowed", node);
        for (const auto& work : Expressions) {
            const auto& value = work.value;
            switch (work.use) {
            case ExpressionUse::Condition:
                if (!value.error && !TypeSystem.arithmetic(value.type) && !TypeSystem.isIntegral(value.type) &&
                    TypeSystem.get(TypeSystem.unConst(value.type)).kind != TypeKind::Pointer)
                    report("condition", "condition requires a scalar value", work.expression);
                break;
            case ExpressionUse::Initializer:
                if (work.symbol.value) {
                    auto type = SymbolTable.get(work.symbol).type;
                    if (!work.expression && TypeSystem.isConst(type))
                        report("initializer", "const object requires an initializer", work.source);
                    if (!TypeSystem.layout(type)) report("complete", "object requires a complete storage type", work.source);
                    // Storage duration does not require a constant initializer.
                    // The checked expression runs before entry for globals, or
                    // once on first use for local statics. Conversion checks were
                    // already performed during annotation, just as for locals.
                }
                break;
            case ExpressionUse::Default:
                if (!value.error && !Checker.convertible(value.type, work.expected, value))
                    report("default-argument", "default argument is not convertible to the parameter type", work.expression);
                break;
            case ExpressionUse::Return:
                if (TypeSystem.isVoid(work.expected)) {
                    if (work.expression) report("return", "void function cannot return a value", work.expression);
                } else if (!work.expression) report("return", "non-void function must return a value", work.source);
                else if (!value.error && !Checker.convertible(value.type, work.expected, value))
                    report("return", "returned expression is not convertible to the function result", work.expression);
                break;
            default: break;
            }
        }
        for (const auto& selection : Switches) {
            const auto& condition = Expressions[selection.condition].value;
            if (condition.error) continue; // already diagnosed; avoid a type cascade
            if (!TypeSystem.isIntegral(condition.type)) {
                report("switch", "switch requires an integral condition", Expressions[selection.condition].expression);
                continue;
            }
            std::unordered_map<int64_t, const types::ASTNode*> cases;
            for (auto index : selection.cases) {
                const auto& work = Expressions[index];
                const auto& value = work.value;
                if (value.error) continue;
                if (!value.constantExpression || value.constant.state != ConstantState::Value || !TypeSystem.isIntegral(value.type)) {
                    report("case", "case label requires an integral constant", work.expression);
                    continue;
                }
                auto normalized = CaseValue(value.constant.value, condition.type, TypeSystem);
                auto [prior, inserted] = cases.emplace(normalized, work.expression);
                if (!inserted) report("case", "duplicate case label after conversion", work.expression, prior->second);
            }
            for (size_t i = 1; i < selection.defaults.size(); ++i)
                report("default", "duplicate default label", selection.defaults[i], selection.defaults.front());
        }
        for (const auto& flow : Flows) {
            std::unordered_map<std::string, const LabelWork*> labels;
            for (const auto& label : flow.labels) {
                auto name = std::string(label.label->getTag()->GetName());
                auto [prior, inserted] = labels.emplace(name, &label);
                if (!inserted) report("label", "duplicate label '" + name + "'", label.label, prior->second->label);
            }
            for (const auto& jump : flow.jumps) {
                if (!jump.target) {
                    std::string message = "no enclosing loop or switch for break";
                    if (dynamic_cast<const types::GotoStatementNode*>(jump.statement)) message = "goto references an undeclared label";
                    else if (dynamic_cast<const types::ContinueStatementNode*>(jump.statement)) message = "no active loop for continue or its named target";
                    else if (auto* statement = dynamic_cast<const types::BreakStatementNode*>(jump.statement); statement && statement->getLabel())
                        message = "no active loop label for break";
                    report("jump", message, jump.statement);
                } else if (auto* go = dynamic_cast<const types::GotoStatementNode*>(jump.statement)) {
                    const auto* target = labels.at(std::string(go->getLabel()->GetName()));
                    for (auto symbol : target->initialized) if (std::find(jump.initialized.begin(), jump.initialized.end(), symbol) == jump.initialized.end()) {
                        report("goto", "goto jumps past initialization of '" + SymbolTable.get(symbol).name + "'", go);
                        break;
                    }
                }
            }
        }
        return diagnostics;
    }

    bool SemanticAnalyzer::check() {
        if (Phase != SemanticPhase::Annotated) return false;
        auto findings = checkReadOnly();
        Diagnostics.insert(Diagnostics.end(), findings.begin(), findings.end());
        Phase = SemanticPhase::Checked;
        return Diagnostics.empty();
    }
}
