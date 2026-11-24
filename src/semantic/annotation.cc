#include "semantic_analyzer.hh"

namespace baka {
    namespace semantic {
        bool SemanticAnalyzer::annotateTypes() {
            if (Phase != SemanticPhase::Resolved) {
                return false;
            }
            Checker.beginAnnotation();
            // Deduced lambda results first: every use of a lambda is typed after its
            // signature is final. Hoisted lambdas precede their enclosing lambdas in
            // the program, so nested deductions resolve inner-first.
            std::vector<bool> annotated(Expressions.size(), false);
            for (size_t i = 0; i < Expressions.size(); ++i) {
                auto& work = Expressions[i];
                if (work.use == ExpressionUse::Return && work.context.deduced.value) {
                    annotateExpression(work);
                    deduceLambdaResult(work);
                    annotated[i] = true;
                }
            }
            for (size_t i = 0; i < Expressions.size(); ++i) {
                if (!annotated[i]) {
                    annotateExpression(Expressions[i]);
                }
            }
            for (const auto& selection : Switches) {
                const auto& condition = Expressions[selection.condition].value;
                if (TypeSystem.isIntegral(condition.type)) {
                    for (auto index : selection.cases) {
                        Expressions[index].expected = Checker.promotedType(condition.type);
                    }
                }
            }
            // Rewrite actual AST edges after all intrinsic typing facts are available.
            if (SymbolTable.diagnostics().empty()) {
                for (auto& work : Expressions) {
                    materializeExpression(work);
                }
            }
            Checker.endAnnotation();
            for (const auto& body : Bodies) {
                if (!TypeSystem.isVoid(body.context.result) && !TypeSystem.layout(body.context.result)) {
                    error("complete", "function definition requires a complete result type", body.declaration);
                }
            }
            if (SymbolTable.diagnostics().empty()) {
                buildExecutionPlans();
            }
            Phase = SemanticPhase::Annotated;
            refreshDiagnostics();
            return Diagnostics.empty();
        }

        // A lambda without a trailing return type returns the type of its returned
        // expressions (after value decay, without top-level const); all must agree.
        void SemanticAnalyzer::deduceLambdaResult(ExpressionWork& work) {
            const auto symbol = work.context.deduced;
            TypeId result = TypeSystem.builtin(Builtin::Void);
            if (work.expression) {
                if (work.value.error) {
                    return;
                }
                result = TypeSystem.unConst(work.value.type);
            }
            auto [known, inserted] = DeducedResults.emplace(symbol.value, result);
            if (!inserted) {
                if (known->second != result) {
                    error("lambda", "inconsistent deduced return types in lambda",
                          work.expression ? static_cast<const types::ASTNode*>(work.expression) : work.source);
                }
                return;
            }
            if (TypeSystem.isVoid(result)) {
                return; // the registered placeholder signature already returns void
            }
            if (TypeSystem.isAggregate(result) && TypeSystem.get(result).completion != Completion::Complete) {
                error("lambda", "deduced lambda result type is incomplete", work.expression);
                return;
            }
            const auto& old = TypeSystem.get(SymbolTable.get(symbol).type);
            auto signature = TypeSystem.function(result, old.parameters, old.variadic);
            if (!signature.value || !SymbolTable.deduceFunctionResult(symbol, signature)) {
                error("lambda", "cannot deduce the lambda's result type", work.expression);
                return;
            }
            for (auto& body : Bodies) {
                if (body.symbol == symbol) {
                    body.context.result = result;
                    body.signature = signature;
                    body.declaration->semanticInfo().Type = signature.value;
                    if (auto* function = dynamic_cast<types::FunctionNode*>(body.declaration)) {
                        if (auto* name = function->getDeclarator()->getIdentifier()) {
                            name->semanticInfo().Type = signature.value;
                        }
                    }
                }
            }
            for (auto& other : Expressions) {
                if (other.context.deduced == symbol) {
                    other.context.result = result;
                    if (other.use == ExpressionUse::Return) {
                        other.expected = result;
                    }
                }
            }
        }

        void SemanticAnalyzer::annotateExpression(ExpressionWork& work) {
            select(work.context);
            if (work.use == ExpressionUse::Initializer) {
                annotateInitializer(work);
            } else if (work.expression) {
                auto* expression = work.expression;
                // Defaults use initializer syntax, but their expected type is a
                // parameter type, not a separately registered object symbol.
                if (work.use == ExpressionUse::Default) {
                    while (auto* list = dynamic_cast<types::InitializerNode*>(expression)) {
                        if (list->expressions().size() != 1) {
                            break;
                        }
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

        void SemanticAnalyzer::annotateInitializer(ExpressionWork& work) {
            bool initialized = Checker.initialize(work.symbol, work.expression);
            if (work.expression) {
                work.expression->semanticInfo().Error = !initialized;
            }
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
        }

        void SemanticAnalyzer::materializeExpression(ExpressionWork& work) {
            if (!work.expression) {
                return;
            }
            select(work.context);
            TypeId target;
            if ((work.use == ExpressionUse::Return || work.use == ExpressionUse::Default) &&
                !TypeSystem.isVoid(work.expected) && Checker.convertible(work.value.type, work.expected, work.value)) {
                target = work.expected;
            }
            if (work.use == ExpressionUse::Condition && work.value.type.value &&
                (TypeSystem.arithmetic(work.value.type) || TypeSystem.isIntegral(work.value.type) ||
                 TypeSystem.get(TypeSystem.unConst(work.value.type)).kind == TypeKind::Pointer)) {
                target = TypeSystem.builtin(Builtin::Bool);
            }
            if (work.use == ExpressionUse::Case && TypeSystem.isIntegral(work.value.type)) {
                target = work.expected;
            }
            if (work.use == ExpressionUse::Switch && TypeSystem.isIntegral(work.value.type)) {
                target = Checker.promotedType(work.value.type);
            }
            auto* original = work.expression;
            work.expression = Checker.materializeConversions(
                original, work.use != ExpressionUse::Initializer && work.source != original, target);
            auto* owner = const_cast<types::ASTNode*>(work.source);
            if (auto* parameter = dynamic_cast<types::FunctionParameter*>(owner)) {
                owner = parameter->getDeclaration();
            }
            if (owner) {
                owner->rewriteExpressions([&](auto* child) { return child == original ? work.expression : child; });
            }
        }

        void SemanticAnalyzer::buildExecutionPlans() {
            Plans.prepare(Checker);
            for (const auto& work : Expressions) {
                if (work.use == ExpressionUse::Initializer && work.symbol.value) {
                    Plans.addInitialization(SymbolTable.get(work.symbol).type, work.expression, work.source);
                } else {
                    Plans.addExpression(work.expression);
                }
            }
            for (const auto& body : Bodies) {
                std::vector<SymbolId> active;
                for (auto id : Plans.locals({body.declaration->semanticInfo().Scope})) {
                    const auto& symbol = SymbolTable.get(id);
                    const auto& lifetime = Plans.lifetime(symbol.type);
                    if (symbol.kind == SymbolKind::Parameter &&
                        symbol.scope.value == body.declaration->semanticInfo().Scope &&
                        (lifetime.construction || lifetime.destruction)) {
                        active.push_back(id);
                    }
                }
                Plans.addBody(body.body, std::move(active));
            }
            Plans.finish();
            for (const auto& [source, message] : Plans.errors()) {
                error("execution-plan", message, source);
            }
        }

    }
}
