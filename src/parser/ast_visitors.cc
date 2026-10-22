#include "parser.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/union.hh"

namespace baka {
    namespace types {
        void FunctionNode::visitChildren(const NodeVisitor& visit) const {
            visit(getReturnTypeName());
            visit(getDeclarator());
            visit(getBody());
        }

        void CompoundStatementNode::visitChildren(const NodeVisitor& visit) const {
            for (const auto& child : getStatements()) {
                std::visit([&](auto* item) { visit(item); }, child);
            }
        }
    }
} // namespace baka::types
