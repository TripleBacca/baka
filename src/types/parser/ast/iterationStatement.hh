#pragma once
#include "expression.hh"
#include "statement.hh"
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
            std::variant<types::ExpressionNode*, types::DeclarationList*> Decl;
            types::ExpressionNode* Cond;
            types::ExpressionNode* Upd;
            types::IdentifierNode* Label;

            StatementNode* Body;

        public:
            ForBlockStatementNode(std::variant<types::ExpressionNode*, types::DeclarationList*> decl, types::ExpressionNode* cond, types::ExpressionNode* upd, types::IdentifierNode* label, StatementNode* body) :
                Decl(decl), Cond(cond), Upd(upd), Label(label), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "For(" << "\n";

                std::visit([&](auto&& decl) {
                    if(decl)
                        decl->Print(Tabs + 1);
                    else
                        INDENT(Tabs + 1) << "nullptr" << std::endl;
                }, Decl);

                if (Cond)
                    Cond->Print(Tabs + 1);
                else
                    INDENT(Tabs + 1) << "nullptr" << std::endl;

                if (Upd)
                    Upd->Print(Tabs + 1);
                else
                    INDENT(Tabs + 1) << "nullptr" << std::endl;

                if (Label)
                    Label->Print(Tabs + 1);
                else
                    INDENT(Tabs + 1) << "nullptr" << std::endl;

                if (Body)
                    Body->Print(Tabs + 1);
                else
                    INDENT(Tabs + 1) << "nullptr" << std::endl;

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class WhileBlockStatementNode : public IterationStatementNode {
            ExpressionNode* Cond;
            IdentifierNode* Label;
            StatementNode* Body;

        public:
            WhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "While(" << "\n";

                Cond->Print(Tabs + 1);

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
            DoWhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Do While(" << "\n";

                if (Body) Body->Print(Tabs + 1);

                Cond->Print(Tabs + 1);
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
