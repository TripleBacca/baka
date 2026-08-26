#pragma once

#include "types/parser/ast/statement.hh"
namespace baka {
namespace types {

    class DefaultLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        DefaultLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~DefaultLabelStatement() = default;
    };


    class CaseLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        CaseLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~CaseLabelStatement() = default;
    };

    class GOTOLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        GOTOLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~GOTOLabelStatement() = default;
    };

}
}
