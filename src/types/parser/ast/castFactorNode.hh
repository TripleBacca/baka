#pragma once
#include "declaration.hh"
#include "expression.hh"
#include "utils.hh"

namespace baka
{
	namespace types
	{
		class CastFactorNode: public types::ExpressionNode {
			types::TypeNode* Type;
			types::DeclarationIdentifierNode* Declarator;
			ExpressionNode* CastExpr;
		public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getType()); visit(getDeclarator()); visit(getExpression()); }
			CastFactorNode(types::TypeNode* Type, types::DeclarationIdentifierNode* Declarator, ExpressionNode* Expr)
				: Type(Type), Declarator(Declarator), CastExpr(Expr) {}

			types::TypeNode* getType() const { return Type; }
			types::DeclarationIdentifierNode* getDeclarator() const { return Declarator; }
			ExpressionNode* getExpression() const { return CastExpr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { CastExpr = rewrite(CastExpr); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(CastExpr); }

            void Print(size_t Tabs = 0) const override {
				INDENT(Tabs);
				std::cout << "CastFactor(" << std::endl;

				Type->Print(Tabs + 1);
				Declarator->Print(Tabs + 1);
				CastExpr->Print(Tabs + 1);

				INDENT(Tabs);
				std::cout << ")" << std::endl;
			}
		};
	}
}
