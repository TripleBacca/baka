# Additional findings: C constraints and baka loop labels

The second audit adds **108 programs**: 24 for-header combinations, 29 additional label/control-flow cases, and 55 C-review cases. All 108 lex successfully. Parsing accepts 92 and rejects 16, with no crashes or timeouts. Together with the earlier audit, there are **221 programs**. Acceptance still means parser acceptance, not semantic validity.

I consulted public WG14 drafts, using C11 as the comparison baseline and C23 to check changed rules. Baka describes itself as C++-like, so C-only constraints are conditional choices. GCC in strict C11, C23, and C++23 modes supplies additional observations, not a replacement specification. References use explicit `struct` tags where baka supplies an implicit type name; the comparison script saves the exact normalized reference sources.

## Newly confirmed parser tasks

These are concrete additions to the old checklist, or stronger reproductions of previously suspected parser problems. They cannot be repaired by type checking an already incorrect AST.

- [ ] Fix for-header delimiter ownership.
  - [ ] Reject no valid ordinary header just because its initializer is an expression and its condition is omitted. `for(i=0;;)` and `for(i=0;;i++)` are currently rejected; their strict-C reference inputs are accepted.
  - [ ] Consume each clause separator once. Both the expression-initializer branch and present-condition branch currently have extra semicolon matches.
  - [ ] Preserve all three clauses and the optional label independently. Five of twelve labeled matrix inputs parse successfully with `Label: nullptr`.
    - [ ] Preserve the label in `for(;i<3;;L)`.
    - [ ] Preserve the label in `for(i=0;;;L)`.
    - [ ] Preserve the condition/update positions and label in `for(i=0;;i++;L)`.
    - [ ] Preserve the label in `for(i=0;i<3;;L)`.
    - [ ] Preserve the label in `for(int j=0;i<3;;L)`.
  - [ ] Verify the AST shape, not only exit status: affected inputs place `L` in the update expression, and one moves `i++` into the condition.
- [ ] Make the loop-label suffix grammar consistent.
  - [ ] Require the separator in `while(condition; Label)` and `do ... while(condition; Label);`. Both parsers currently accept `while(1 L)`.
  - [ ] Require an identifier after a consumed label separator. `while(1;)`, `do ... while(1;);`, and `for(;;;)` currently succeed.
  - [ ] Apply one type-name collision policy across all loop kinds. `do ... while(1; T);` accepts a typedef name that matching while/for forms reject.
  - [ ] Keep rejecting numeric and multiple label suffixes; those controls already produce parser errors.
- [ ] Repair label ownership in statement bodies. This refines the earlier null-label-body warning with confirmed AST evidence.
  - [ ] Attach the statement after `again:` to that label node. `while(i<3; L) again: i++;` currently places `i++` outside the loop in the AST.
  - [ ] Handle case/default labels as actual labeled substatements, including unbraced loop bodies within switches.
  - [ ] Parse `do again: i++; while(i<3; L);` as a single do-loop; it currently fails.
  - [ ] Preserve an if body's labeled statement so `if(1) L: return 1; else return 0;` parses; it currently fails.
- [ ] Fix two additional expression/statement grammar omissions.
  - [ ] Allow the middle operand in `a ? 2,3 : 4` to contain a comma expression; the current parser rejects it.
  - [ ] Parse a general switch body statement instead of always calling `CompoundStatement()`. `switch(1) case 1: return 1;` currently fails.

The last two inputs have accepted strict-C controls; their grammar is described in [C99 draft N1256, §§6.5.15 and 6.8.4](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf). The missing-separator suffix cases are baka extension rules inferred from README's `while(condition; Label)` example and the loop AST/parser, not rules supplied by the C standard.

## New semantic details missing from the previous checklist

These make previously broad topics explicit. Adopt C-specific rules only if baka chooses that behavior.

- [ ] Add recursive aggregate modifiability checks, including const fields inside nested members.
- [ ] Allow incomplete parameter types in prototypes; require completeness in definitions.
- [ ] Reject initialized function declarators.
- [ ] Validate incomplete array elements even beneath pointer declarators.
- [ ] Validate VLA initialization, static VLA objects, VM scope/linkage, and repeated VM typedefs.
- [ ] Check goto/switch entry into variably modified scopes.
- [ ] Handle evaluated VLA sizeof bounds and retain declaration-time extents.

These distinctions come from [C11 draft N1570, §§6.3.2.1, 6.5.3.4, 6.7, 6.7.6, 6.7.9, 6.8.4.2, and 6.8.6.1](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf). Associated inputs include `nested_const_field_assignment`, `prototype_incomplete_parameter`, `function_initializer`, `pointer_array_incomplete_element`, `vm_pointer_file_scope`, `typedef_vla_redefinition`, and `sizeof_vla_bound_side_effect`.

- [ ] Normalize adjusted parameter types; ignore top-level parameter qualifiers for signature compatibility, retaining pointee qualifiers.
- [ ] Recognize eligible zero integral constant expressions as null constants, not ordinary zero-valued variables.
- [ ] Keep integer-constant-expression validity distinct from runtime foldability.
- [ ] For a C11-style for clause, restrict declared entities/storage classes; do not apply that restriction to every C version.

See [C99 draft N1256, §§6.3.2.3, 6.6, 6.7.5.3, and 6.8.5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf). These refine the previous generic conversion, prototype, constant-evaluation, and storage checks. GCC C11 rejects the static-object, typedef, and function declarations in for clauses; GCC C23 accepts them.

## Corrections and positive controls to add

- [ ] Correct the old blanket statement that sizeof operands never execute. The main checklist now links to the VLA exception above. Parsing can establish that a type is represented; execution semantics remain untestable until code generation exists.
- [ ] Separate static VLA objects from block-scope static pointers to VLA types. `static_pointer_to_vla` is accepted by the C11 reference while `vla_static_storage` is rejected.
- [ ] Compare compatible signatures after adjustment: retain controls for top-level parameter const, array-to-pointer adjustment, and changed inner array dimensions.
- [ ] Permit assignable aggregates that contain array members. Whole-array assignment being invalid must not prohibit that aggregate copy.
- [ ] Keep unevaluated-expression name/type checking separate from evaluation. `0 && unknown` and `sizeof(unknown)` still need resolution.
- [ ] Keep pointer equality and ordering checks separate. The reference accepts `p==0`, rejects `p<0`, and rejects ordering function pointers.
- [ ] Keep same-valued enumerators distinct from duplicate enumerator names when explicit values become parsable.
- [ ] Make declaration-point tests include array bounds: the outer `n` is relevant in `int n=2; ... int n[n];` under the C comparison rules.

These are refinements and safeguards within existing checklist topics; they are not all newly absent top-level passes. Their concrete inputs and compiler outcomes are in [RESULTS.md](RESULTS.md). GCC observations above describe the saved local runs.

## Version and language decisions exposed by the probes

- [ ] Choose C11/C23 behavior for empty function parameter lists.
- [ ] Choose whether labels before declarations or at block ends are supported.
- [ ] Choose whether empty brace initialization is supported.
- [ ] Choose whether to retain the older for-declaration restriction.

These cases changed between the compared drafts; consult [C23 draft N3096, §§6.7.6.3, 6.7.10, 6.8.1–6.8.2, 6.8.5, and Annex M](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf). The probe results agree with the changes; treating every C11 rejection as a baka defect would be incorrect.

- [ ] Resolve C11-versus-baka policy for void-valued return expressions, const-local case values, zero-expression null constants, and for-body redeclaration of the initializer variable. Strict reference modes give different outcomes.
- [ ] Decide whether flexible array members, bit-fields, and designated initializers belong in scope. The probes confirm parser support is absent. If enabled, they require their own AST forms and checks; do not silently describe them as already supported by generic aggregate checks.
- [ ] State whether methods in structs are intentional extensions; a C function-member constraint must not accidentally reject baka's method declarations.
- [ ] Do not classify compatible C declarations as conflicts solely because GCC's C++ mode interprets the same spelling as distinct overloads. Reference compiler acceptance is not proof of semantic equivalence.

## Loop-label semantic contract and regression tasks

This is the proposed consistent grammar, based on the existing extension. `init` denotes the ordinary initializer/declaration clause, with its usual delimiter handling.

```text
while ( condition [ ; Label ] ) statement
for ( init ; condition? ; update? [ ; Label ] ) statement
do statement while ( condition [ ; Label ] ) ;
break [ Label ] ;
continue [ Label ] ;
```

- [ ] Register a loop's attached label before traversing its body, including a do-loop whose suffix occurs later in source order.
- [ ] Resolve loop targets using the active loop stack, independently from ordinary value-name and goto-label lookup.
- [ ] Define collisions between variable names, goto labels, type names, and loop labels. Decide duplicate active-label behavior separately from sibling reuse.
- [ ] Reject jumps to future/exited siblings, to another function's loop, and from a nested function to an outer function's loop.
- [ ] Do not let `goto L` resolve only because a loop suffix defines `L`; do not let `break L` resolve only because an ordinary `L:` label exists.
- [ ] Lower named continue to the selected loop's update/test, and named break to that loop's exit, even through nested switches.
- [ ] Keep the suffix out of runtime expression evaluation; a label spelling must never become an ordinary variable read/update clause.
- [ ] Add execution checks once lowering exists for outer-for continue, outer-do continue, outer-loop break through a switch, and scope cleanup.

The old checklist already requested active enclosing targets and correct continue destinations. The additions here are the trailing do-label prebinding requirement, explicit namespace contract, malformed-header checks, AST invariants, and concrete cross-context probes. Current parsing accepts the invalid named-target examples because semantic analysis is absent; it cannot establish correct runtime jump behavior.

## Reproduce

```bash
python3 semantic_audit/run.py --programs semantic_audit/extended/programs --output /tmp/baka-extended-audit
python3 semantic_audit/extended/compare_c.py
```

`manifest.json` records each probe's purpose. `results.json` is the saved lexer/parser snapshot; `comparison.json` includes reference diagnostics and for-label AST inspections. Full logs and normalized reference sources are in `/tmp/baka-extended-audit`. Only audit artifacts and the checklist were changed; the compiler implementation was not modified.

Implementation assignments are in the [three-person task split](../TASK_SPLIT.md); every checkbox here has one primary owner in its linked ownership map.
