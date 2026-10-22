#pragma once
#include "expression.hh"
#include "statement.hh"
#include "types/parser/ast/typedef.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace types {
        class IterationStatementNode : public StatementNode {
        public:
            virtual ~IterationStatementNode() = default;
        };

        class ForBlockStatementNode : public IterationStatementNode {
            //TODO Change to declaration?
            // we should probably not support typedefs here
            std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> Decl;
            types::ExpressionNode* Cond;
            types::ExpressionNode* Upd;
            types::IdentifierNode* Label;

            StatementNode* Body;

        public:
        void visitChildren(const NodeVisitor& visit) const override { std::visit([&](auto* child) { visit(child); }, getInitializer()); visit(getCondition()); visit(getUpdate()); visit(getBody()); visit(getLabel()); }
            ForBlockStatementNode(std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> decl, types::ExpressionNode* cond, types::ExpressionNode* upd, types::IdentifierNode* label, StatementNode* body) :
                Decl(decl), Cond(cond), Upd(upd), Label(label), Body(body) {
            }

            using InitVariant = std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*>;
            const InitVariant& getInitializer() const { return Decl; }
            types::ExpressionNode* getCondition() const { return Cond; }
            types::ExpressionNode* getUpdate() const { return Upd; }
            types::IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { if (auto* expr = std::get_if<ExpressionNode*>(&Decl)) *expr = rewrite(*expr); Cond = rewrite(Cond); Upd = rewrite(Upd); }
            void visitExpressions(const ExpressionVisitor& visit) const override { if (auto* expr = std::get_if<ExpressionNode*>(&Decl)) visit(*expr); visit(Cond); visit(Upd); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "For(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                std::visit([&](auto&& decl) {
                    if(decl)
                        decl->Print(Tabs + 1);
                    else {
                        INDENT(Tabs + 1);
                        std::cout << "Decl: nullptr" << std::endl;
                    }
                }, Decl);

                if (Cond)
                    Cond->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }

                if (Upd)
                    Upd->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Upd: nullptr" << std::endl;
                }

                if (Label)
                    Label->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Label: nullptr" << std::endl;
                }

                if (Body)
                    Body->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Body: nullptr" << std::endl;
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class WhileBlockStatementNode : public IterationStatementNode {
            ExpressionNode* Cond;
            IdentifierNode* Label;
            StatementNode* Body;

        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getCondition()); visit(getBody()); visit(getLabel()); }
            WhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            ExpressionNode* getCondition() const { return Cond; }
            IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Cond = rewrite(Cond); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Cond); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "While(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Cond) {
                    Cond->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }

                if (Label) {
                    INDENT(Tabs);
                    std::cout << ";\n";
                    Label->Print(Tabs + 1);
                }
                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class DoWhileBlockStatementNode : public IterationStatementNode {
            ExpressionNode* Cond;
            IdentifierNode* Label;
            StatementNode* Body;

        public:
        void visitChildren(const NodeVisitor& visit) const override { visit(getCondition()); visit(getBody()); visit(getLabel()); }
            DoWhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            ExpressionNode* getCondition() const { return Cond; }
            IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Cond = rewrite(Cond); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Cond); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Do While(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Body) Body->Print(Tabs + 1);

                if (Cond) {
                    Cond->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }
                if (Label) {
                    INDENT(Tabs);
                    std::cout << ";\n";
                    Label->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
