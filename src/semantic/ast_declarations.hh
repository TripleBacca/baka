#pragma once

#include "declarations.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/typenode.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/enums.hh"

namespace baka {
    namespace semantic {
        struct EvaluationContext {
            enum class Purpose { ArrayBound, Enumerator } purpose;
            ScopeId scope;
            TypeId expected;
        };

        // B implements this interface. Evaluation runs in the live declaration environment.
        class DeclarationEvaluator {
        public:
            virtual ~DeclarationEvaluator() = default;
            virtual IntegerResult integer(const types::ExpressionNode*, EvaluationContext) = 0;
        };

        class ASTDeclarations {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            DeclarationEvaluator* Evaluator;
            std::function<void(types::ExpressionNode*)> PrepareExpression;

            IntegerResult EvaluateInteger(const types::ExpressionNode* expression,
                                          EvaluationContext::Purpose purpose);

        public:
            ASTDeclarations(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                            DeclarationEvaluator* Evaluator = nullptr);

            void setEvaluator(DeclarationEvaluator* evaluator);
            void setExpressionResolver(std::function<void(types::ExpressionNode*)> resolver) {
                PrepareExpression = std::move(resolver);
            }

            TypeId specifier(const types::IdentifierNode* name, types::TypeSpecifierModifier modifier, bool constant,
                             const void* source = nullptr);

            TypeId typeName(const types::TypeNode& node);

            // Build outward suffixes before recursing inward: int (*p)[3] is pointer-to-array.
            TypeId declarator(TypeId base, const types::DeclarationIdentifierNode* node);

            // C enters one Function scope, calls this, then visits the outer body in that SAME scope.
            // Nested blocks use Block scopes, so only genuine inner-block shadowing is legal.
            std::vector<SymbolId> parameters(const types::FunctionParameterList& params);

            // B's initializer callback runs AFTER insertion; each declarator's bound runs BEFORE.
            // Thus int x=x; binds the new x, while int x[sizeof(x)]; sees the prior x.
            using InitializerCheck = std::function<void(SymbolId, const types::ExpressionNode*)>;

            std::vector<SymbolId> declarationList(const types::DeclarationList& node,
                                                  InitializerCheck initialize = {});

            // Infer storage shape from initializer syntax before later declarations
            // ask sizeof(array). Element conversion checks remain in annotation.
            bool inferArrayBound(SymbolId symbol, const types::ExpressionNode* initializer);

            std::vector<SymbolId> aliases(const types::TypedefNode& node);

            TypeId enumeration(const types::EnumNode& node);
        };
    }
}
