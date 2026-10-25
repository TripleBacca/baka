# Checked frontend to IR contract

Typed TAC generation is implemented in `src/ir`. This file describes its current
input contract; it is not a proposal for a future generator.

## Entry point and lifetime

```
SemanticAnalyzer analyzer;
if (analyzer.analyze(program)) {
    auto checked = analyzer.checkedProgram();
    ir::Lowerer lowerer(*checked);
    if (lowerer.lower()) {
        auto module = lowerer.takeModule();
    }
}
```

The checked view couples the exact tree and its analyzer. Both must live through
lowering. A completed IR module contains no borrowed AST pointers or semantic IDs
and remains usable after the frontend is destroyed. Lowering is single-use.

## Semantic input

| Requirement | Authoritative fact |
| --- | --- |
| Intrinsic type/category | AST semantic metadata; explicit load/decay nodes specify value edges |
| Identifier identity | Resolved symbol ID; generated source names are diagnostic conveniences |
| Arithmetic | Explicit checked conversions plus the shared fixed-target numeric contract |
| Calls | `ExecutionPlans::call`: selected callable and complete argument-expression list |
| Default construction | `LifetimePlan::defaultConstructor`: availability, signature/member and arguments |
| Initialization | `InitializationPlan`: typed operations, offsets, receiver context and compact ranges |
| Field/base access | Completed stored semantic layout and field/base types, copied into the owned IR record type |
| Local storage | Published per-function symbol index |
| Jumps | Resolved target IDs and ordered `ExecutionPlans::cleanup` obligations |
| Normal scope exit | Lexical object ownership, with published lifetime traits |

The lowerer must not select constructors, resolve missing names, expand defaults,
match source brace elision or accept jumps entering object lifetimes. Those decisions
are completed before a checked view is published. Invalid default construction and
unsupported braced/union construction produce semantic diagnostics.

## Lowering responsibilities

The lowerer imports representation types, erases source qualifiers after checking,
creates objects/temporaries/basic blocks, and emits expressions directly at the
current insertion point. A fresh function context owns receiver overrides, pending
temporaries, construction destinations, jump blocks and activation flags. There are
no disconnected expression-region queues or AST-pointer evaluation cursors.

Calls capture the callee and all argument values before emitting contiguous `param`
instructions followed by `call` with only a callee and optional result. Nested calls finish
before the outer call's params are staged; param sequences never cross block boundaries.
Aggregate values remain typed snapshots; native register/memory ABI classification
and hidden return storage are deliberately deferred. Constructors receive explicit
object pointers. Only requested implicit constructors are emitted. Destructor
registration and first-use static guards preserve the documented runtime order.

Cleanup lists choose which objects leave an edge; runtime flags determine whether
their construction completed on that path. Arrays use small unrolling or bounded
IR loops. Bulk zero operations represent omitted ranges without scalar expansion.

## Verification and numeric behavior

Lowering verifies the module before and after optional optimization. The verifier
checks type graphs, operand/instruction shapes, call arguments, CFG consistency and
definite assignment of mutable temporaries. Malformed operand kinds and type IDs
are diagnosed before opcode-specific table accesses.

Compiler constant evaluation and optimization share width-aware arithmetic and
conversion operations. The test interpreter is independent and uses poisoned
automatic storage. Source fixtures cover arithmetic, calls, aggregate values,
initialization and cleanup; semantic rejection cases cover invalid types, bindings,
construction and control flow.

## Deliberate limits

The current target is fixed LP64 and has no native code generator, platform ABI,
exception unwinding, concurrent static initialization or pointer-to-member values.
Language subset policies and diagnostic behavior are listed in [README.md](README.md).
