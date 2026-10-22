#include "expressions.hh"

namespace baka {
    namespace semantic {
        ExpressionChecker::ExpressionChecker(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                                             ASTDeclarations& ASTResolver)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
              ASTResolver(ASTResolver) {}

        void ExpressionChecker::beginAnnotation() {
            Annotating = true;
            RawResults.clear();
            ValueResults.clear();
            ConversionTargets.clear();
            ConstructorConversions.clear();
        }

        void ExpressionChecker::setFunctionContext(TypeId currentClass, bool staticContext, bool constMethod) {
            CurrentClass = currentClass;
            InStaticContext = staticContext;
            InConstMethod = constMethod;
        }

        ExprResult ExpressionChecker::makeError() {
            ExprResult result;
            result.error = true;
            return result;
        }

        ExprResult ExpressionChecker::makeValue(TypeId type, ValueCategory category) {
            ExprResult result;
            result.type = type;
            result.category = category;
            return result;
        }

        ExprResult ExpressionChecker::makeConstant(TypeId type, int64_t value) {
            ExprResult result;
            result.type = type;
            result.constantExpression = true;
            result.constant = {ConstantState::Value, value};
            return convertConstant(result, type);
        }

        ExprResult ExpressionChecker::makeBool(bool value) {
            return makeConstant(TypeSystem.builtin(Builtin::Bool), value ? 1 : 0);
        }

        bool ExpressionChecker::checkScalarCondition(const ExprResult& value, const types::ASTNode* source) {
            if (value.error) {
                return false;
            }
            if (!isScalar(value.type)) {
                SymbolTable.diagnose("condition", "condition requires a scalar expression", source);
                return false;
            }
            return true;
        }

        ExprResult ExpressionChecker::checkImpl(const types::ExpressionNode* expression) {
            if (!expression || expression->hasError()) {
                return makeError();
            }
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                return checkPrimary(primary);
            } else if (auto* unary = dynamic_cast<const types::FactorNode*>(expression)) {
                return checkUnary(unary);
            } else if (auto* binary = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
                return checkBinary(binary);
            } else if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
                return checkIndex(index);
            } else if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                return checkMember(member->getBase(), member->getMember(), false, expression);
            } else if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                return checkMember(arrow->getBase(), arrow->getMember(), true, expression);
            } else if (auto* call = dynamic_cast<const types::FunctionCallPostfixExpr*>(expression)) {
                return checkCall(call);
            } else if (auto* postfix = dynamic_cast<const types::UnaryPostfixExpr*>(expression)) {
                return checkPostfix(postfix);
            } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
                return checkComma(comma);
            } else if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
                return checkTernary(ternary);
            } else if (auto* cast = dynamic_cast<const types::CastFactorNode*>(expression)) {
                return checkCast(cast);
            } else if (auto* sizeofNode = dynamic_cast<const types::SizeofNode*>(expression)) {
                return checkSizeof(sizeofNode);
            } else if (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
                return checkInitializerExpression(initializer, {});
            } else {
                SymbolTable.diagnose("expression", "unsupported expression node", expression);
                return makeError();
            }
        }

        ExprResult ExpressionChecker::check(const types::ExpressionNode* expression, bool shouldDecay) {
            if (!expression || (expression->semanticInfo().Resolved && expression->semanticInfo().Error)) {
                return makeError(); // resolver already diagnosed this occurrence
            }
            auto& cache = shouldDecay ? ValueResults : RawResults;
            if (Annotating) {
                auto found = cache.find(expression);
                if (found != cache.end()) {
                    return found->second;
                }
            }
            ExprResult raw;
            auto cached = RawResults.find(expression);
            if (Annotating && cached != RawResults.end()) {
                raw = cached->second;
            } else {
                raw = checkImpl(expression);
            }
            if (Annotating) {
                RawResults[expression] = raw;
                auto& metadata = const_cast<types::ExpressionNode*>(expression)->semanticInfo();
                metadata.Type = raw.type.value;
                metadata.Annotated = true;
                metadata.Error = raw.error;
                metadata.LValue = raw.category == ValueCategory::LValue;
                metadata.ConstantExpression = raw.constantExpression;
                metadata.HasConstant = raw.constant.state == ConstantState::Value;
                metadata.Constant = raw.constant.value;
                metadata.NullPointerConstant = raw.nullPointerConstant;
                metadata.HasFloatingConstant = raw.floatingConstant.has_value();
                metadata.FloatingConstant = raw.floatingConstant.value_or(0);
            }
            auto value = raw;
            if (shouldDecay && !value.error && value.boundInstanceFunction) {
                SymbolTable.diagnose("member-function", "instance member function requires an object-bound call",
                                     expression);
                value = makeError();
                if (Annotating) {
                    const_cast<types::ExpressionNode*>(expression)->semanticInfo().Error = true;
                }
            }
            if (shouldDecay && !value.error) {
                value.type = decay(value.type);
                value.category = ValueCategory::RValue;
            }
            if (Annotating) {
                cache[expression] = value;
            }
            return value;
        }

        IntegerResult ExpressionChecker::integer(const types::ExpressionNode* expression, EvaluationContext context) {
            struct ScopeGuard {
                Symbols& symbols;
                ScopeId saved;

                ~ScopeGuard() {
                    symbols.selectScope(saved);
                }
            } guard{SymbolTable, SymbolTable.currentScope()};

            if (context.scope.value) {
                SymbolTable.selectScope(context.scope);
            }
            while (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
                if (initializer->expressions().size() != 1) {
                    return {ConstantState::Nonconstant, 0};
                }
                expression = initializer->expressions().front();
            }
            auto value = check(expression, false);
            if (context.expected.value) {
                value = convertConstant(value, context.expected);
            }
            if (value.error) {
                return {ConstantState::Error, 0};
            }
            if (!isInteger(value.type)) {
                return {ConstantState::Nonconstant, 0};
            }
            if (!value.constantExpression || value.constant.state != ConstantState::Value) {
                return {ConstantState::Nonconstant, 0};
            }
            return value.constant;
        }

        ExprResult ExpressionChecker::checkValue(const types::ExpressionNode* expression) {
            return check(expression, true);
        }

        bool ExpressionChecker::checkCondition(const types::ExpressionNode* expression, const types::ASTNode* source) {
            auto value = check(expression, true);
            if (value.error) {
                return false;
            }
            return checkScalarCondition(value, source);
        }
    }
}
