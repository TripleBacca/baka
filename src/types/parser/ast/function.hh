#pragma once
#include "ast_node.hh"
#include <iostream>
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
namespace types {

    class StatementNode;

    template <typename T>
    void PrintStatementBody(T* Body, size_t Tabs) {
        Body->Print(Tabs);
    }

    class FunctionNode : public ASTNode {
        // todo use DeclarationIdentifierNode
        IdentifierNode* BoundReturnType;

        DeclarationIdentifierNode* FuncType;

        StatementNode* Body;

        bool IsStatic = false, IsConst = false;
        TypeSpecifierModifier Modifier = TypeSpecifierModifier::NONE;

    public:
        void visitChildren(const NodeVisitor& visit) const override;
        FunctionNode(IdentifierNode* BindedReturnType, DeclarationIdentifierNode* funcType, StatementNode* body, bool IsStatic, bool IsConst, TypeSpecifierModifier modifier) :
            BoundReturnType(BindedReturnType), FuncType(funcType), Body(body), IsStatic(IsStatic), IsConst(IsConst), Modifier(modifier) {
        }

        ~FunctionNode() override = default;

        IdentifierNode* getReturnTypeName() const { return BoundReturnType; }
        DeclarationIdentifierNode* getDeclarator() const { return FuncType; }
        StatementNode* getBody() const { return Body; }
        bool getIsStatic() const { return IsStatic; }
        bool getIsConst() const { return IsConst; }
        TypeSpecifierModifier getModifier() const { return Modifier; }

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "Function(\n";

            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << '\n';

            INDENT(Tabs+1);
            std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";

            INDENT(Tabs+1);
            std::cout << "FuncType: " << '\n';
            FuncType->Print(Tabs + 1);

            INDENT(Tabs+1);
            std::cout << "ReturnType: " << '\n';
            BoundReturnType->Print(Tabs + 1);

            INDENT(Tabs + 1);
            std::cout << "FunctionBody:" << '\n';
            PrintStatementBody(Body, Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
