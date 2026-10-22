# Extended audit observations

This is the second run only: 108 additional inputs. The first run remains in [../RESULTS.md](../RESULTS.md). Read the [new tasks and standard references](FINDINGS.md) for interpretation.

| Stage | Accepted | Rejected | Crashed | Timed out |
| --- | ---: | ---: | ---: | ---: |
| baka lex | 108 | 0 | 0 | 0 |
| baka parse | 92 | 16 | 0 | 0 |

There are 24 for-header variants, 29 further loop/label cases, and 55 C-review cases. Reference compilers ran on 67 inputs: the 12 ordinary for-header variants and all 55 C-review cases. Loop-label extension inputs were not submitted as standard C.

Reference command modes use GCC 16.2.1 with `-pedantic-errors -fsyntax-only`. C11/C23 reference inputs normalize implicit aggregate names to explicit struct-tag spellings; C++23 uses the original input. The saved normalized files and full logs are in `/tmp/baka-extended-audit`. Compiler acceptance is supporting evidence, not a specification of baka.

## Accepted for headers with missing label nodes

Each entry below was intended to have a suffix label L. The parser accepted it but emitted Label: nullptr. The direct AST children show the incorrect classification.

| Input | Direct children of For node |
| --- | --- |
| [for_empty_present_empty_label](programs/for_empty_present_empty_label.cpp) | `Decl: nullptr / CommaExpression( / CommaExpression( / Label: nullptr / CompoundStatement(` |
| [for_expression_empty_empty_label](programs/for_expression_empty_empty_label.cpp) | `CommaExpression( / Cond: nullptr / CommaExpression( / Label: nullptr / CompoundStatement(` |
| [for_expression_empty_present_label](programs/for_expression_empty_present_label.cpp) | `CommaExpression( / CommaExpression( / CommaExpression( / Label: nullptr / CompoundStatement(` |
| [for_expression_present_empty_label](programs/for_expression_present_empty_label.cpp) | `CommaExpression( / CommaExpression( / CommaExpression( / Label: nullptr / CompoundStatement(` |
| [for_declaration_present_empty_label](programs/for_declaration_present_empty_label.cpp) | `DeclarationList( / CommaExpression( / CommaExpression( / Label: nullptr / CompoundStatement(` |

## Every additional input

A dash indicates that the nonstandard label input was not sent to a reference mode. Accepted/rejected status does not by itself validate AST structure, overload equivalence, runtime behavior, or the chosen baka language policy.

| Input | baka parse | GCC C11 | GCC C23 | GCC C++23 |
| --- | --- | --- | --- | --- |
| [aggregate_condition](programs/aggregate_condition.cpp) | Accepted | Rejected | Rejected | Rejected |
| [aggregate_function_member](programs/aggregate_function_member.cpp) | Accepted | Rejected | Rejected | Accepted |
| [array_size_self_scope](programs/array_size_self_scope.cpp) | Accepted | Accepted | Accepted | Rejected |
| [assign_aggregate_with_array_member](programs/assign_aggregate_with_array_member.cpp) | Accepted | Accepted | Accepted | Accepted |
| [bitfield_member](programs/bitfield_member.cpp) | Rejected | Accepted | Accepted | Accepted |
| [break_goto_label_only](programs/break_goto_label_only.cpp) | Accepted | — | — | — |
| [break_outer_while_from_switch](programs/break_outer_while_from_switch.cpp) | Accepted | — | — | — |
| [case_comma_not_ice](programs/case_comma_not_ice.cpp) | Accepted | Rejected | Rejected | Accepted |
| [case_unevaluated_call](programs/case_unevaluated_call.cpp) | Accepted | Rejected | Rejected | Accepted |
| [conditional_void_pointer_qualification](programs/conditional_void_pointer_qualification.cpp) | Accepted | Accepted | Accepted | Accepted |
| [const_field_whole_assignment](programs/const_field_whole_assignment.cpp) | Accepted | Rejected | Rejected | Rejected |
| [const_local_not_c_ice](programs/const_local_not_c_ice.cpp) | Accepted | Rejected | Rejected | Accepted |
| [continue_outer_do](programs/continue_outer_do.cpp) | Accepted | — | — | — |
| [continue_outer_for_from_switch](programs/continue_outer_for_from_switch.cpp) | Accepted | — | — | — |
| [dangling_else_after_label](programs/dangling_else_after_label.cpp) | Rejected | — | — | — |
| [definition_incomplete_parameter](programs/definition_incomplete_parameter.cpp) | Accepted | Rejected | Rejected | Rejected |
| [designated_initializer](programs/designated_initializer.cpp) | Rejected | Accepted | Accepted | Accepted |
| [do_empty_label](programs/do_empty_label.cpp) | Accepted | — | — | — |
| [do_label_after_body](programs/do_label_after_body.cpp) | Accepted | — | — | — |
| [do_label_after_labeled_body](programs/do_label_after_labeled_body.cpp) | Rejected | — | — | — |
| [do_missing_separator](programs/do_missing_separator.cpp) | Accepted | — | — | — |
| [do_typedef_label](programs/do_typedef_label.cpp) | Accepted | — | — | — |
| [duplicate_enum_values_valid](programs/duplicate_enum_values_valid.cpp) | Rejected | Accepted | Accepted | Accepted |
| [empty_aggregate_initializer](programs/empty_aggregate_initializer.cpp) | Accepted | Rejected | Accepted | Accepted |
| [flexible_array_member](programs/flexible_array_member.cpp) | Rejected | Accepted | Accepted | Rejected |
| [for_body_shadow_initializer](programs/for_body_shadow_initializer.cpp) | Accepted | Accepted | Accepted | Rejected |
| [for_body_typedef_leak](programs/for_body_typedef_leak.cpp) | Rejected | Rejected | Rejected | Rejected |
| [for_declaration_empty_empty](programs/for_declaration_empty_empty.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_declaration_empty_empty_label](programs/for_declaration_empty_empty_label.cpp) | Accepted | — | — | — |
| [for_declaration_empty_present](programs/for_declaration_empty_present.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_declaration_empty_present_label](programs/for_declaration_empty_present_label.cpp) | Accepted | — | — | — |
| [for_declaration_present_empty](programs/for_declaration_present_empty.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_declaration_present_empty_label](programs/for_declaration_present_empty_label.cpp) | Accepted | — | — | — |
| [for_declaration_present_present](programs/for_declaration_present_present.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_declaration_present_present_label](programs/for_declaration_present_present_label.cpp) | Accepted | — | — | — |
| [for_empty_empty_empty](programs/for_empty_empty_empty.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_empty_empty_empty_label](programs/for_empty_empty_empty_label.cpp) | Accepted | — | — | — |
| [for_empty_empty_present](programs/for_empty_empty_present.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_empty_empty_present_label](programs/for_empty_empty_present_label.cpp) | Accepted | — | — | — |
| [for_empty_label](programs/for_empty_label.cpp) | Accepted | — | — | — |
| [for_empty_present_empty](programs/for_empty_present_empty.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_empty_present_empty_label](programs/for_empty_present_empty_label.cpp) | Accepted | — | — | — |
| [for_empty_present_present](programs/for_empty_present_present.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_empty_present_present_label](programs/for_empty_present_present_label.cpp) | Accepted | — | — | — |
| [for_expression_empty_empty](programs/for_expression_empty_empty.cpp) | Rejected | Accepted | Accepted | Accepted |
| [for_expression_empty_empty_label](programs/for_expression_empty_empty_label.cpp) | Accepted | — | — | — |
| [for_expression_empty_present](programs/for_expression_empty_present.cpp) | Rejected | Accepted | Accepted | Accepted |
| [for_expression_empty_present_label](programs/for_expression_empty_present_label.cpp) | Accepted | — | — | — |
| [for_expression_present_empty](programs/for_expression_present_empty.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_expression_present_empty_label](programs/for_expression_present_empty_label.cpp) | Accepted | — | — | — |
| [for_expression_present_present](programs/for_expression_present_present.cpp) | Accepted | Accepted | Accepted | Accepted |
| [for_expression_present_present_label](programs/for_expression_present_present_label.cpp) | Accepted | — | — | — |
| [for_function_declaration](programs/for_function_declaration.cpp) | Accepted | Rejected | Accepted | Accepted |
| [for_static_object](programs/for_static_object.cpp) | Accepted | Rejected | Accepted | Accepted |
| [for_typedef_declaration](programs/for_typedef_declaration.cpp) | Accepted | Rejected | Accepted | Accepted |
| [for_typedef_label](programs/for_typedef_label.cpp) | Rejected | — | — | — |
| [function_empty_list_argument](programs/function_empty_list_argument.cpp) | Accepted | Accepted | Rejected | Rejected |
| [function_initializer](programs/function_initializer.cpp) | Accepted | Rejected | Rejected | Rejected |
| [function_pointer_relational](programs/function_pointer_relational.cpp) | Accepted | Rejected | Rejected | Rejected |
| [goto_into_vla_scope](programs/goto_into_vla_scope.cpp) | Accepted | Rejected | Rejected | Rejected |
| [goto_loop_label_only](programs/goto_loop_label_only.cpp) | Accepted | — | — | — |
| [label_at_block_end](programs/label_at_block_end.cpp) | Accepted | Rejected | Accepted | Accepted |
| [label_before_declaration](programs/label_before_declaration.cpp) | Accepted | Rejected | Accepted | Accepted |
| [label_value_same_name](programs/label_value_same_name.cpp) | Accepted | Accepted | Accepted | Accepted |
| [loop_goto_same_name](programs/loop_goto_same_name.cpp) | Accepted | — | — | — |
| [loop_label_duplicate_nested](programs/loop_label_duplicate_nested.cpp) | Accepted | — | — | — |
| [loop_label_exited_sibling](programs/loop_label_exited_sibling.cpp) | Accepted | — | — | — |
| [loop_label_forward_sibling](programs/loop_label_forward_sibling.cpp) | Accepted | — | — | — |
| [loop_label_nested_function](programs/loop_label_nested_function.cpp) | Accepted | — | — | — |
| [loop_label_other_function](programs/loop_label_other_function.cpp) | Accepted | — | — | — |
| [loop_label_reuse_siblings](programs/loop_label_reuse_siblings.cpp) | Accepted | — | — | — |
| [loop_variable_same_name](programs/loop_variable_same_name.cpp) | Accepted | — | — | — |
| [main_bad_parameter](programs/main_bad_parameter.cpp) | Accepted | Rejected | Rejected | Rejected |
| [main_float_return](programs/main_float_return.cpp) | Accepted | Rejected | Rejected | Rejected |
| [nested_const_field_assignment](programs/nested_const_field_assignment.cpp) | Accepted | Rejected | Rejected | Rejected |
| [parameter_array_pointer_compatibility](programs/parameter_array_pointer_compatibility.cpp) | Accepted | Accepted | Accepted | Accepted |
| [parameter_inner_bound_conflict](programs/parameter_inner_bound_conflict.cpp) | Accepted | Rejected | Rejected | Accepted |
| [parameter_pointee_const_conflict](programs/parameter_pointee_const_conflict.cpp) | Accepted | Rejected | Rejected | Accepted |
| [parameter_top_const_compatibility](programs/parameter_top_const_compatibility.cpp) | Accepted | Accepted | Accepted | Accepted |
| [pointer_array_incomplete_element](programs/pointer_array_incomplete_element.cpp) | Accepted | Rejected | Rejected | Accepted |
| [pointer_equality_null](programs/pointer_equality_null.cpp) | Accepted | Accepted | Accepted | Accepted |
| [pointer_relational_null](programs/pointer_relational_null.cpp) | Accepted | Rejected | Rejected | Rejected |
| [prototype_incomplete_parameter](programs/prototype_incomplete_parameter.cpp) | Accepted | Accepted | Accepted | Accepted |
| [prototype_incomplete_return](programs/prototype_incomplete_return.cpp) | Accepted | Accepted | Accepted | Accepted |
| [short_circuit_typecheck](programs/short_circuit_typecheck.cpp) | Accepted | Rejected | Rejected | Rejected |
| [sizeof_typechecks_identifier](programs/sizeof_typechecks_identifier.cpp) | Accepted | Rejected | Rejected | Rejected |
| [sizeof_vla_bound_side_effect](programs/sizeof_vla_bound_side_effect.cpp) | Accepted | Accepted | Accepted | Rejected |
| [sizeof_vla_frozen_extent](programs/sizeof_vla_frozen_extent.cpp) | Accepted | Accepted | Accepted | Rejected |
| [static_pointer_to_vla](programs/static_pointer_to_vla.cpp) | Accepted | Accepted | Accepted | Rejected |
| [switch_enters_vla_scope](programs/switch_enters_vla_scope.cpp) | Accepted | Rejected | Rejected | Rejected |
| [ternary_middle_comma](programs/ternary_middle_comma.cpp) | Rejected | Accepted | Accepted | Accepted |
| [typedef_vla_redefinition](programs/typedef_vla_redefinition.cpp) | Accepted | Rejected | Rejected | Rejected |
| [unbraced_case_loop_body](programs/unbraced_case_loop_body.cpp) | Accepted | — | — | — |
| [unbraced_goto_label_loop_body](programs/unbraced_goto_label_loop_body.cpp) | Accepted | — | — | — |
| [unbraced_switch](programs/unbraced_switch.cpp) | Rejected | Accepted | Accepted | Accepted |
| [vla_initializer](programs/vla_initializer.cpp) | Accepted | Rejected | Rejected | Rejected |
| [vla_static_storage](programs/vla_static_storage.cpp) | Accepted | Rejected | Rejected | Rejected |
| [vla_struct_member](programs/vla_struct_member.cpp) | Rejected | Rejected | Rejected | Rejected |
| [vm_pointer_file_scope](programs/vm_pointer_file_scope.cpp) | Accepted | Rejected | Rejected | Rejected |
| [void_return_void_expression](programs/void_return_void_expression.cpp) | Accepted | Rejected | Rejected | Accepted |
| [while_const_condition_no_semicolon](programs/while_const_condition_no_semicolon.cpp) | Accepted | — | — | — |
| [while_empty_label](programs/while_empty_label.cpp) | Accepted | — | — | — |
| [while_missing_separator](programs/while_missing_separator.cpp) | Accepted | — | — | — |
| [while_multiple_labels](programs/while_multiple_labels.cpp) | Rejected | — | — | — |
| [while_numeric_label](programs/while_numeric_label.cpp) | Rejected | — | — | — |
| [while_typedef_label](programs/while_typedef_label.cpp) | Rejected | — | — | — |
| [zero_null_expression](programs/zero_null_expression.cpp) | Accepted | Accepted | Accepted | Rejected |
| [zero_variable_not_null_constant](programs/zero_variable_not_null_constant.cpp) | Accepted | Rejected | Rejected | Rejected |
