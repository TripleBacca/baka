#pragma once



#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/statement.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <variant>
namespace baka {
     namespace types {

         class Block : public ASTNode {
             std::vector<std::variant<StatementNode* , DeclarationList*>> BlockItems;

             public:
             Block(std::vector<std::variant<StatementNode* , DeclarationList*>> BlockItems) : BlockItems(BlockItems) {};

             void Print(size_t Tabs = 0) const override {
                 INDENT(Tabs);
                 std::cout << "Block(\n";

                 for (const auto& item : BlockItems) {
                     std::visit([Tabs](const auto* node) { node->Print(Tabs + 1); }, item);
                 }
                 std::cout << ")\n";
             }
         };


     }
}
