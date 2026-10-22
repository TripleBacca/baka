#pragma once

#include "declarations.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/typenode.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/enums.hh"

namespace baka {
    namespace semantic {
        struct EvaluationContext {
            enum class Purpose { ArrayBound, Enumerator, ConstantInitializer } purpose;
            ScopeId scope;
            TypeId expected; // optional destination conversion, before publishing a constant
        };

        // Evaluation runs in the live declaration environment.
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

            IntegerResult EvaluateInteger(types::ExpressionNode* expression, EvaluationContext::Purpose purpose);
            void consumeArrayElement(TypeId type, const std::vector<types::ExpressionNode*>& values, size_t& cursor);
            bool isAggregateValue(const types::ExpressionNode* expression, TypeId target) const;

        public:
            ASTDeclarations(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                            DeclarationEvaluator* Evaluator = nullptr);

            void setEvaluator(DeclarationEvaluator* evaluator);

            void setExpressionResolver(std::function<void(types::ExpressionNode*)> resolver) {
                PrepareExpression = std::move(resolver);
            }

            TypeId specifier(types::IdentifierNode* name, types::TypeSpecifierModifier modifier, bool constant,
                             const types::ASTNode* source = nullptr);

            TypeId typeName(types::TypeNode& node);

            // Build outward suffixes before recursing inward: int (*p)[3] is pointer-to-array.
            TypeId declarator(TypeId base, types::DeclarationIdentifierNode* node);

            // Resolution enters one Function scope and visits the outer body in that same scope.
            // Nested blocks use Block scopes, so only genuine inner-block shadowing is legal.
            std::vector<SymbolId> parameters(types::FunctionParameterList& params);

            // Initializer checking runs after insertion; each declarator's bound runs before.
            // Thus int x=x; binds the new x, while int x[sizeof(x)]; sees the prior x.
            using InitializerCheck = std::function<void(SymbolId, types::ExpressionNode*)>;

            std::vector<SymbolId> declarationList(types::DeclarationList& node, InitializerCheck initialize = {});

            // Infer storage shape from initializer syntax before later declarations
            // ask sizeof(array). Element conversion checks remain in annotation.
            bool inferArrayBound(SymbolId symbol, const types::ExpressionNode* initializer);

            std::vector<SymbolId> aliases(types::TypedefNode& node);

            TypeId enumeration(types::EnumNode& node);
        };
    }
}
