#pragma once



#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "parser.hh"

namespace baka {
     namespace parser {

         // static int a,*b,*c[10]
         //
         //
         // declaration        := 'static'? 'const'? type-specifier declarator-list ';'
         // type-specifier      := 'int' | 'char' | ... | 'struct' IDENTIFIER
         // declarator-list     := declarator (',' declarator)*
         // declarator          := '*'* IDENTIFIER ('[' expr? ']')* ('=' initializer)?
         // initializer         := expr | '{' expr (',' expr)* '}'

         // parse_specifiers():
         //     is_static = False
         //     is_const = False

         //     if peek() == 'static':
         //         consume('static')
         //         is_static = True

         //     if peek() == 'const':
         //         consume('const')
         //         is_const = True

         //     base_type = parse_type_specifier()
         //     base_type.is_const = is_const   # attach to the type, not the declarator

         //     return is_static, base_type

         types::DeclarationList* Parser::ParseDeclarationList() {

             types::IdentifierNode* TypeName = ParseIdentifier();



         }



     }
}
