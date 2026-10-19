#include "semantic_analyzer.hh"

namespace baka::semantic {
    bool SemanticAnalyzer::annotateTypes() {
        if (Phase != SemanticPhase::Resolved) return false;
        Checker.beginAnnotation();
        for (auto& work : Expressions) {
            select(work.context);
            if (work.use == ExpressionUse::Initializer) {
                bool initialized = Checker.initialize(work.symbol, work.expression);
                if (work.expression) work.expression->semanticInfo().Error = !initialized;
                if (work.symbol.value) {
                    work.expected = SymbolTable.get(work.symbol).type; // inferred array bounds
                    auto* declaration = const_cast<types::ASTNode*>(work.source);
                    declaration->semanticInfo().Type = work.expected.value;
                    declaration->semanticInfo().Annotated = true;
                    declaration->semanticInfo().Error = !initialized;
                    if (auto* item = dynamic_cast<types::SingleDeclarationNode*>(declaration)) {
                        item->getDeclarator()->semanticInfo().Type = work.expected.value;
                        item->getDeclarator()->getIdentifier()->semanticInfo().Type = work.expected.value;
                    }
                    if (work.expression) {
                        auto& metadata = work.expression->semanticInfo();
                        metadata.Type = work.expected.value;
                        metadata.Annotated = true;
                    }
                }
            } else if (work.expression) {
                auto* expression = work.expression;
                // Defaults use initializer syntax, but their expected type is a
                // parameter type, not a separately registered object symbol.
                if (work.use == ExpressionUse::Default) {
                    while (auto* list = dynamic_cast<types::InitializerNode*>(expression)) {
                        if (list->expressions().size() != 1) break;
                        expression = list->expressions().front();
                    }
                }
                work.value = Checker.checkValue(expression);
                if (work.use == ExpressionUse::Default) {
                    work.expression->semanticInfo().Type = work.value.type.value;
                    work.expression->semanticInfo().Annotated = true;
                }
            }
        }
        for (const auto& selection : Switches) {
            const auto& condition = Expressions[selection.condition].value;
            if (TypeSystem.isIntegral(condition.type)) for (auto index : selection.cases)
                Expressions[index].expected = Checker.promotedType(condition.type);
        }
        // Rewriting follows typing so cached intrinsic facts remain available
        // to read-only checks. Each work item's owner replaces the actual AST
        // slot, not merely the queue pointer (important for IR traversal).
        if (SymbolTable.diagnostics().empty()) for (auto& work : Expressions) {
            if (!work.expression) continue;
            select(work.context);
            TypeId target;
            if ((work.use == ExpressionUse::Return || work.use == ExpressionUse::Default) &&
                !TypeSystem.isVoid(work.expected) &&
                Checker.convertible(work.value.type, work.expected, work.value)) target = work.expected;
            if (work.use == ExpressionUse::Condition && work.value.type.value &&
                (TypeSystem.arithmetic(work.value.type) || TypeSystem.isIntegral(work.value.type) ||
                 TypeSystem.get(TypeSystem.unConst(work.value.type)).kind == TypeKind::Pointer))
                target = TypeSystem.builtin(Builtin::Bool);
            if (work.use == ExpressionUse::Case && TypeSystem.isIntegral(work.value.type)) target = work.expected;
            if (work.use == ExpressionUse::Switch && TypeSystem.isIntegral(work.value.type))
                target = Checker.promotedType(work.value.type);
            auto* original = work.expression;
            work.expression = Checker.materializeConversions(original, work.use != ExpressionUse::Initializer && work.source != original, target);
            auto* owner = const_cast<types::ASTNode*>(work.source);
            if (auto* parameter = dynamic_cast<types::FunctionParameter*>(owner)) owner = parameter->getDeclaration();
            if (owner) owner->rewriteExpressions([&](auto* child) { return child == original ? work.expression : child; });
        }
        Checker.endAnnotation();
        Phase = SemanticPhase::Annotated;
        refreshDiagnostics();
        return Diagnostics.empty();
    }
}
