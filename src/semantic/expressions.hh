#pragma once

#include "ast_declarations.hh"
#include "aggregates.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/sizeof.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/initializer.hh"
#include <unordered_map>
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/implicitCast.hh"

namespace baka {
    namespace semantic {
        enum class ValueCategory { LValue, RValue };

        struct ExprResult {
            TypeId type;
            ValueCategory category = ValueCategory::RValue;
            bool error = false;
            bool constantExpression = false;
            IntegerResult constant;
            bool nullPointerConstant = false;
            std::optional<long double> floatingConstant;
            // An instance method designator is bound to an object, not an
            // ordinary function value that may decay to a plain function pointer.
            bool boundInstanceFunction = false;

            explicit operator bool() const {
                return !error && type.value;
            }
        };

        // Expression typing service. Declaration-time dependencies can be evaluated
        // without annotation. beginAnnotation enables the mutating type pass, which
        // consumes frozen bindings and records intrinsic types on the AST.
        class ExpressionChecker : public DeclarationEvaluator {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            ASTDeclarations& ASTResolver;

            bool Annotating = false;
            // Destination conversions are recorded only after compatibility was
            // checked. Materialization runs once typing has finished, so overload
            // selection and constant evaluation see the original syntax tree.
            std::unordered_map<const types::ExpressionNode*, TypeId> ConversionTargets;
            void planConversion(const types::ExpressionNode* expression, TypeId target);
            types::ExpressionNode* wrapConversion(types::ExpressionNode*, TypeId, types::ImplicitConversionKind);
            types::ExpressionNode* convertValue(types::ExpressionNode*, TypeId target = {});
            types::ExpressionNode* materializeImpl(types::ExpressionNode*, bool value, TypeId target = {});
            std::unordered_map<const types::ExpressionNode*, ExprResult> RawResults, ValueResults;
            ExprResult checkImpl(const types::ExpressionNode* expression, bool decay);
            std::vector<SymbolId> lookupIdentifier(const types::IdentifierNode* identifier) const;
            size_t requiredArguments(SymbolId symbol, TypeId type) const;
            struct FunctionCandidate { TypeId type; SymbolId symbol; size_t required; TypeId owner{}; uint32_t memberIndex = 0; };
            std::unordered_map<uint32_t, int64_t> ConstantObjects;

            TypeId CurrentClass;
            bool InStaticContext = false;
            bool InConstMethod = false;

            ExprResult check(const types::ExpressionNode* expression, bool decay = true);
            ExprResult checkPrimary(const types::PrimaryExpressionNode* node);
            ExprResult checkConstant(const types::ConstantNode* node);
            ExprResult checkUnary(const types::FactorNode* node);
            ExprResult checkBinary(const types::BinaryExpressionNode* node);
            ExprResult checkAssignment(const types::BinaryExpressionNode* node);
            ExprResult checkIndex(const types::IndexPostfixExpr* node);
            ExprResult checkMember(const types::ExpressionNode* base, const types::IdentifierNode* member,
                                   bool arrow, const void* source);
            ExprResult checkCall(const types::FunctionCallPostfixExpr* node);
            ExprResult checkTernary(const types::TernaryOpNode* node);
            ExprResult checkCast(const types::CastFactorNode* node);
            ExprResult checkSizeof(const types::SizeofNode* node);
            ExprResult checkInitializerExpression(const types::InitializerNode* node, TypeId target);

            ExprResult makeError();
            ExprResult makeValue(TypeId type, ValueCategory category = ValueCategory::RValue);
            ExprResult makeConstant(TypeId type, int64_t value);
            ExprResult makeBool(bool value);

            bool isInteger(TypeId type) const;
            bool isArithmetic(TypeId type) const;
            bool isPointer(TypeId type) const;
            bool isScalar(TypeId type) const;
            bool isNullPointerConstant(const ExprResult& value) const;
            bool isCompleteObject(TypeId type) const;
            TypeId decay(TypeId type) const;
            bool isBoundInstanceFunction(const types::ExpressionNode* expression) const;
            TypeId promote(TypeId type) const;
            TypeId usualArithmetic(TypeId left, TypeId right, const void* source);
            bool isAssignable(TypeId from, TypeId to, const ExprResult& value) const;
            bool isModifiableLValue(const ExprResult& value) const;
            bool checkScalarCondition(const ExprResult& value, const void* source);

            std::vector<SymbolId> resolveCallee(const types::ExpressionNode* callee, const void* source);
            std::vector<FunctionCandidate> functionCandidates(const types::ExpressionNode* callee, const void* source);
            int conversionRank(TypeId from, TypeId to, const ExprResult& value) const;
            bool checkArguments(const TypeId& functionType, const std::vector<ExprResult>& arguments,
                                const void* source, size_t required);
            std::vector<ExprResult> argumentList(const types::ExpressionNode* list);

            bool initializeScalar(TypeId target, const types::InitializerNode& initializer, const void* source);
            bool initializeAggregate(TypeId target, const types::InitializerNode& initializer, bool topLevel);
            bool initializeArray(TypeId target, const types::InitializerNode& initializer);
            bool initializeList(TypeId target, const std::vector<types::ExpressionNode*>& values,
                                size_t& cursor, bool braced, const void* source);
            bool flattenMembers(TypeId type, std::vector<TypeId>& out) const;

        public:
            ExpressionChecker(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                              ASTDeclarations& ASTResolver);

            void setFunctionContext(TypeId currentClass, bool staticContext, bool constMethod);

            void beginAnnotation() { Annotating = true; RawResults.clear(); ValueResults.clear(); ConversionTargets.clear(); }
            void endAnnotation() { Annotating = false; }
            // Called only by the mutating annotation pass, after all typing work.
            types::ExpressionNode* materializeConversions(types::ExpressionNode* expression, bool value = true,
                                                         TypeId target = {}) {
                return materializeImpl(expression, value, target);
            }
            TypeId promotedType(TypeId type) const { return promote(type); }
            const auto& annotatedExpressions() const { return RawResults; }

            IntegerResult integer(const types::ExpressionNode* expression, EvaluationContext context) override;

            ExprResult checkValue(const types::ExpressionNode* expression);

            bool convertible(TypeId from, TypeId to, const ExprResult& value) const {
                return isAssignable(from, to, value);
            }

            bool checkCondition(const types::ExpressionNode* expression, const void* source);

            // Called from ASTDeclarations::declarationList after insertion. Completes inferred
            // array bounds and validates scalar/aggregate/string initialization.
            bool initialize(SymbolId symbol, const types::ExpressionNode* initializer);

            // B's MemberSignature for ASTAggregates: classifies functions as methods/ctors/dtors.
            std::optional<Member> memberSignature(const types::ASTNode* node, TypeId owner);
        };
    }
}
