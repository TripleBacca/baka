# Lexer/parser audit results

The compiler was rebuilt from the current source with `cmake --build build --parallel 2`. Each of the 113 hand-written programs was independently run through both `--lex --verbose` and `--parse --verbose`. These are lexer/parser observations, not semantic conformance results.

| Stage | Accepted | Rejected with diagnostics | Crashed | Timed out |
| --- | ---: | ---: | ---: | ---: |
| Lexer | 113 | 0 | 0 | 0 |
| Parser | 93 | 17 | 3 | 0 |

## Confirmed findings

The subsequent [standard review and loop-label audit](extended/FINDINGS.md) adds 108 programs and a checklist of new findings. Counts in this file refer to the original run only.

Tasks below come from the audit programs and source inspection. Language-dependent behavior is listed under decisions rather than assumed to be an error. The broader implementation checklist is in [CHECKLIST.md](CHECKLIST.md).

- [ ] Implement name resolution and declaration checks.
  - [ ] Reject undeclared variables and use before declaration.
  - [ ] Reject references to variables outside their scope.
  - [ ] Reject duplicate local variables and parameter names.
  - [ ] Reject duplicate function definitions.
  - [ ] Reject incompatible function declarations.
  - [ ] Reject conflicting function and variable declarations.
  - [ ] Reject incompatible typedef redefinitions and aliases that conflict with existing aggregate names.
- [ ] Validate declared types and object storage.
  - [ ] Reject objects and arrays with void element type.
  - [ ] Reject objects whose by-value type is incomplete.
  - [ ] Reject recursive by-value aggregate members; permit self-referential pointers.
  - [ ] Reject functions that return arrays directly.
  - [ ] Reject a function body attached to a declarator that denotes an object.
  - [ ] Reject floating-point and negative array bounds.
  - [ ] Validate type-kind modifiers, including an enum modifier applied to a class name.
  - [ ] Check duplicate storage specifiers consistently on function definitions as well as variable declarations.
- [ ] Implement expression and assignment checks.
  - [ ] Reject assignment and increment on non-modifiable targets such as constants.
  - [ ] Reject whole-array assignment.
  - [ ] Reject writes to const objects, including typedef-qualified objects and members of const objects.
  - [ ] Reject dereference of non-pointer values.
  - [ ] Reject taking the address of constants.
  - [ ] Reject bitwise operations on floating-point operands.
  - [ ] Reject pointer multiplication.
  - [ ] Reject void-valued conditions.
  - [ ] Reject casts to array types; apply the chosen rules to aggregate casts.
  - [ ] Reject sizeof on void and function types.
- [ ] Implement function call and return checks.
  - [ ] Reject calls on non-callable expressions.
  - [ ] Check required and excess argument counts, including default parameters.
  - [ ] Check argument conversions against parameter types.
  - [ ] Check default argument conversions against parameter types.
  - [ ] Reject incompatible return values, including non-void values returned from void functions and floating values returned as pointers.
  - [ ] Preserve call argument nesting when counting arguments: f(a,b) has two arguments; f((a,b)) has one. Existing AST nesting already distinguishes them.
- [ ] Implement control-flow checks.
  - [ ] Reject break outside a loop or switch.
  - [ ] Reject continue without an enclosing loop, including inside a switch with no enclosing loop.
  - [ ] Reject undefined or no-longer-active named loop targets.
  - [ ] Reject undefined goto targets.
  - [ ] Reject duplicate goto labels within a function.
  - [ ] Reject case/default labels outside a switch.
  - [ ] Reject duplicate case values and multiple defaults within one switch.
  - [ ] Require constant integral case values.
  - [ ] Reject floating-point switch conditions.
- [ ] Implement member, enum, and initializer checks.
  - [ ] Reject duplicate aggregate members.
  - [ ] Reject duplicate enumerator names.
  - [ ] Reject access to nonexistent members.
  - [ ] Reject arrow access on non-pointer receivers, including aggregate objects.
  - [ ] Reject this outside an instance member context, including static methods.
  - [ ] Reject excess array initializer elements.
  - [ ] Diagnose duplicate constructor signatures and duplicate destructors after preserving their declarations.
- [ ] Fix parser and AST gaps needed by semantic analysis.
  - [ ] Parse bare return statements with an optional expression.
  - [ ] Parse standalone empty statements.
  - [ ] Parse and retain omitted array bounds where supported.
  - [ ] Parse explicit enum initializer expressions if included in the language.
  - [ ] Retain the variadic marker in function parameter lists.
  - [ ] Preserve constructor/destructor declarations instead of overwriting earlier nodes.
  - [ ] Retain union names in the AST for semantic type registration.
- [ ] Replace crashes with compiler diagnostics.
  - [ ] Handle unsigned float declarations without asserting.
  - [ ] Handle unsigned applied to a typedef name without asserting; diagnose or support it according to the language rule.
  - [ ] Handle conflicting enum/struct declarations without an uncaught type-state exception.
- [ ] Decide language-dependent behavior and implement the chosen rules.
  - [ ] Define whether nonconstant array bounds are supported and in which contexts.
  - [ ] Define constant versus dynamic initialization for globals and local statics.
  - [ ] Define duplicate active loop-label behavior.
  - [ ] Define initialization requirements for const objects.
  - [ ] Define permitted casts and implicit type conversions consistently.
  - [ ] Define whether goto may bypass initialized declarations.
  - [ ] Support nested function definitions deliberately or reject them semantically.
  - [ ] Decide whether to implement long long declarations and main(void) syntax.
  - [ ] Keep unnamed parameters explicitly unsupported unless the README's scope changes.
  - [ ] Implement new/delete parsing and semantics if desired, or document their unsupported status despite keyword recognition.
- [ ] Turn audit programs into regression checks as the corresponding rules are implemented.
  - [ ] Assert expected diagnostics for invalid programs and verify that errors do not crash the compiler.
  - [ ] Retain valid controls for recursion, local shadowing, forward goto, self-referential pointers, and forward-declared aggregate pointers.
  - [ ] Retain valid controls for function pointers, member access, inheritance, default arguments, and compatible repeated typedefs.
  - [ ] Retain valid controls for labeled break/continue, empty for clauses, and sizeof of incomplete-type pointers.
  - [ ] Retain distinct argument-count checks for comma-separated arguments and parenthesized comma expressions.

## Reproduce

```bash
cmake --build build --parallel 2
python3 semantic_audit/run.py
```

The runner writes full token tables, ASTs, diagnostics, and JSON to `/tmp/baka-semantic-audit-results` by default. Override with `--output PATH` or `--compiler PATH`. `results.json` alongside this file is the saved audit snapshot; the runner does not overwrite it automatically.

## Every program

All lexer invocations accepted. Parser outcomes follow. Inputs named `valid_` are positive controls; names describe intent and are not a machine-checked semantic oracle.

| Program | Parser outcome |
| --- | --- |
| [address_constant.cpp](programs/address_constant.cpp) | Accepted |
| [array_assignment.cpp](programs/array_assignment.cpp) | Accepted |
| [bad_cast_aggregate.cpp](programs/bad_cast_aggregate.cpp) | Accepted |
| [bad_cast_array.cpp](programs/bad_cast_array.cpp) | Accepted |
| [bare_return.cpp](programs/bare_return.cpp) | Rejected |
| [break_outside_loop.cpp](programs/break_outside_loop.cpp) | Accepted |
| [call_one_comma_argument.cpp](programs/call_one_comma_argument.cpp) | Accepted |
| [call_two_arguments.cpp](programs/call_two_arguments.cpp) | Accepted |
| [case_outside_switch.cpp](programs/case_outside_switch.cpp) | Accepted |
| [checked_default_order.cpp](programs/checked_default_order.cpp) | Rejected |
| [checked_destructor_name.cpp](programs/checked_destructor_name.cpp) | Rejected |
| [checked_duplicate_const.cpp](programs/checked_duplicate_const.cpp) | Rejected |
| [checked_duplicate_static.cpp](programs/checked_duplicate_static.cpp) | Rejected |
| [checked_incomplete_base.cpp](programs/checked_incomplete_base.cpp) | Rejected |
| [checked_loop_typename.cpp](programs/checked_loop_typename.cpp) | Rejected |
| [checked_struct_redefinition.cpp](programs/checked_struct_redefinition.cpp) | Rejected |
| [checked_unknown_type.cpp](programs/checked_unknown_type.cpp) | Rejected |
| [comma_call_three.cpp](programs/comma_call_three.cpp) | Accepted |
| [comma_call_two.cpp](programs/comma_call_two.cpp) | Accepted |
| [conflicting_alias_to_base.cpp](programs/conflicting_alias_to_base.cpp) | Accepted |
| [conflicting_prototypes.cpp](programs/conflicting_prototypes.cpp) | Accepted |
| [conflicting_typedef.cpp](programs/conflicting_typedef.cpp) | Accepted |
| [const_assignment.cpp](programs/const_assignment.cpp) | Accepted |
| [const_member_write.cpp](programs/const_member_write.cpp) | Accepted |
| [const_object_no_initializer.cpp](programs/const_object_no_initializer.cpp) | Accepted |
| [const_typedef_write.cpp](programs/const_typedef_write.cpp) | Accepted |
| [continue_in_switch.cpp](programs/continue_in_switch.cpp) | Accepted |
| [continue_outer_label.cpp](programs/continue_outer_label.cpp) | Accepted |
| [continue_outside_loop.cpp](programs/continue_outside_loop.cpp) | Accepted |
| [default_argument_type.cpp](programs/default_argument_type.cpp) | Accepted |
| [delete_expression.cpp](programs/delete_expression.cpp) | Rejected |
| [deref_nonpointer.cpp](programs/deref_nonpointer.cpp) | Accepted |
| [duplicate_active_label_continue.cpp](programs/duplicate_active_label_continue.cpp) | Accepted |
| [duplicate_active_loop_label.cpp](programs/duplicate_active_loop_label.cpp) | Accepted |
| [duplicate_case.cpp](programs/duplicate_case.cpp) | Accepted |
| [duplicate_constructor.cpp](programs/duplicate_constructor.cpp) | Accepted |
| [duplicate_default.cpp](programs/duplicate_default.cpp) | Accepted |
| [duplicate_destructor.cpp](programs/duplicate_destructor.cpp) | Accepted |
| [duplicate_enumerator.cpp](programs/duplicate_enumerator.cpp) | Accepted |
| [duplicate_function_static.cpp](programs/duplicate_function_static.cpp) | Accepted |
| [duplicate_goto_label.cpp](programs/duplicate_goto_label.cpp) | Accepted |
| [duplicate_local.cpp](programs/duplicate_local.cpp) | Accepted |
| [duplicate_member.cpp](programs/duplicate_member.cpp) | Accepted |
| [duplicate_parameter.cpp](programs/duplicate_parameter.cpp) | Accepted |
| [empty_statement.cpp](programs/empty_statement.cpp) | Rejected |
| [explicit_enum_value.cpp](programs/explicit_enum_value.cpp) | Rejected |
| [float_bitwise.cpp](programs/float_bitwise.cpp) | Accepted |
| [float_switch.cpp](programs/float_switch.cpp) | Accepted |
| [floating_array_bound.cpp](programs/floating_array_bound.cpp) | Accepted |
| [forward_struct_pointer.cpp](programs/forward_struct_pointer.cpp) | Accepted |
| [function_redefinition.cpp](programs/function_redefinition.cpp) | Accepted |
| [function_returns_array.cpp](programs/function_returns_array.cpp) | Accepted |
| [function_variable_conflict.cpp](programs/function_variable_conflict.cpp) | Accepted |
| [goto_bypasses_initialization.cpp](programs/goto_bypasses_initialization.cpp) | Accepted |
| [inactive_loop_label.cpp](programs/inactive_loop_label.cpp) | Accepted |
| [incomplete_object.cpp](programs/incomplete_object.cpp) | Accepted |
| [increment_constant.cpp](programs/increment_constant.cpp) | Accepted |
| [invalid_lvalue.cpp](programs/invalid_lvalue.cpp) | Accepted |
| [invalid_static_initializer.cpp](programs/invalid_static_initializer.cpp) | Accepted |
| [invalid_this.cpp](programs/invalid_this.cpp) | Accepted |
| [local_static_initialization.cpp](programs/local_static_initialization.cpp) | Accepted |
| [long_long_declaration.cpp](programs/long_long_declaration.cpp) | Rejected |
| [missing_member.cpp](programs/missing_member.cpp) | Accepted |
| [negative_array_bound.cpp](programs/negative_array_bound.cpp) | Accepted |
| [nested_function.cpp](programs/nested_function.cpp) | Accepted |
| [new_expression.cpp](programs/new_expression.cpp) | Rejected |
| [noncallable.cpp](programs/noncallable.cpp) | Accepted |
| [nonconstant_array_bound.cpp](programs/nonconstant_array_bound.cpp) | Accepted |
| [nonconstant_case.cpp](programs/nonconstant_case.cpp) | Accepted |
| [object_with_body.cpp](programs/object_with_body.cpp) | Accepted |
| [out_of_scope.cpp](programs/out_of_scope.cpp) | Accepted |
| [oversized_initializer.cpp](programs/oversized_initializer.cpp) | Accepted |
| [pointer_multiplication.cpp](programs/pointer_multiplication.cpp) | Accepted |
| [recursive_member.cpp](programs/recursive_member.cpp) | Accepted |
| [sizeof_function.cpp](programs/sizeof_function.cpp) | Accepted |
| [sizeof_void.cpp](programs/sizeof_void.cpp) | Accepted |
| [static_method_this.cpp](programs/static_method_this.cpp) | Accepted |
| [undeclared_variable.cpp](programs/undeclared_variable.cpp) | Accepted |
| [undefined_goto.cpp](programs/undefined_goto.cpp) | Accepted |
| [undefined_loop_label.cpp](programs/undefined_loop_label.cpp) | Accepted |
| [unnamed_parameter.cpp](programs/unnamed_parameter.cpp) | Rejected |
| [unsigned_alias.cpp](programs/unsigned_alias.cpp) | Crashed (signal 6) |
| [unsigned_float.cpp](programs/unsigned_float.cpp) | Crashed (signal 6) |
| [unspecified_array_bound.cpp](programs/unspecified_array_bound.cpp) | Rejected |
| [use_before_declaration.cpp](programs/use_before_declaration.cpp) | Accepted |
| [valid_array.cpp](programs/valid_array.cpp) | Accepted |
| [valid_default_argument.cpp](programs/valid_default_argument.cpp) | Accepted |
| [valid_empty_for.cpp](programs/valid_empty_for.cpp) | Accepted |
| [valid_forward_goto.cpp](programs/valid_forward_goto.cpp) | Accepted |
| [valid_function_pointer.cpp](programs/valid_function_pointer.cpp) | Accepted |
| [valid_incomplete_pointer_size.cpp](programs/valid_incomplete_pointer_size.cpp) | Accepted |
| [valid_inheritance.cpp](programs/valid_inheritance.cpp) | Accepted |
| [valid_labeled_loop.cpp](programs/valid_labeled_loop.cpp) | Accepted |
| [valid_member.cpp](programs/valid_member.cpp) | Accepted |
| [valid_recursion.cpp](programs/valid_recursion.cpp) | Accepted |
| [valid_scalar.cpp](programs/valid_scalar.cpp) | Accepted |
| [valid_self_pointer.cpp](programs/valid_self_pointer.cpp) | Accepted |
| [valid_shadow.cpp](programs/valid_shadow.cpp) | Accepted |
| [valid_typedef_repeat.cpp](programs/valid_typedef_repeat.cpp) | Accepted |
| [variadic.cpp](programs/variadic.cpp) | Accepted |
| [void_array.cpp](programs/void_array.cpp) | Accepted |
| [void_condition.cpp](programs/void_condition.cpp) | Accepted |
| [void_object.cpp](programs/void_object.cpp) | Accepted |
| [void_parameter_list.cpp](programs/void_parameter_list.cpp) | Rejected |
| [void_return_value.cpp](programs/void_return_value.cpp) | Accepted |
| [wrong_argument_count.cpp](programs/wrong_argument_count.cpp) | Accepted |
| [wrong_argument_type.cpp](programs/wrong_argument_type.cpp) | Accepted |
| [wrong_arrow.cpp](programs/wrong_arrow.cpp) | Accepted |
| [wrong_arrow_on_struct.cpp](programs/wrong_arrow_on_struct.cpp) | Accepted |
| [wrong_default_arity.cpp](programs/wrong_default_arity.cpp) | Accepted |
| [wrong_enum_modifier.cpp](programs/wrong_enum_modifier.cpp) | Accepted |
| [wrong_pointer_return.cpp](programs/wrong_pointer_return.cpp) | Accepted |
| [wrong_type_kind.cpp](programs/wrong_type_kind.cpp) | Crashed (signal 6) |
