#pragma once
#include "ast_node.hh"



namespace baka {
namespace types {


    class StatementNode : public ASTNode {
    public:
        virtual ~StatementNode() = default;
    };

}
}
