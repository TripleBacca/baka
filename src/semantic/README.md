# Semantic analysis and the lowering boundary

The frontend uses one parser AST, persistent type/symbol/scope arenas, and four
public phases:

```
register declarations -> resolve bindings -> annotate and normalize -> check
```

Successful analysis publishes a `CheckedProgram` view. It couples the exact AST
with the analyzer that owns its IDs. The driver passes this view to IR lowering;
the resulting module owns its types, layout, strings and instruction operands.
Keep the frontend alive while consuming the checked view. Do not edit the AST
after publishing the view.

## Implementation layers

| Layer | Implementation and published facts |
| --- | --- |
| Parser AST | Source spans, intrinsic expression metadata, stable identifier bindings, explicit conversion nodes. `visitChildren` is mandatory for every concrete AST kind; expression kinds must also enumerate direct expression edges. |
| `Types` | Arena IDs; immutable structural interning keys for pointers, qualifiers, arrays and signatures; nominal aggregate identity; completeness and checked LP64 layout. `storedLayout()` borrows completed record layout without copying offsets. |
| `Symbols` | Persistent lexical scopes, overload sets, declaration compatibility, typed declaration/default pointers and separate declaration positions. A symbol identifies an entity; each redeclaration has its own position. |
| `Declarations` | Storage/type/member rules, inheritance and cached special-member traits. |
| `ASTDeclarations` / `ASTAggregates` | Mutating AST adapters: interpret nested declarators, collect parameters/members, infer omitted array bounds, and complete aggregates. |
| `ExpressionChecker` | Intrinsic/value expression facts, initializer compatibility, overload selection and destination conversion decisions. |
| `conversions.cc` | Materializes explicit load, decay, promotion, arithmetic/pointer/boolean and selected-constructor conversions on actual AST edges. |
| `ExecutionPlans` | Immutable lowering input: initialization steps, lifetime traits, chosen default constructors, expanded call arguments, function-local indexes and ordered jump cleanup lists. |
| Final checks | Read-only validation of queued expression/control facts. Recoverable errors retain independent diagnostics. |

The analyzer files follow the phase order: `registration.cc`, `resolution.cc`,
`annotation.cc`, then `checks.cc`. `expressions.cc` handles dispatch, caching and
publication of expression facts. The rules for primary expressions, operators,
calls, members and initializers live in their corresponding source files;
`expression_types.cc` holds operand classification and type compatibility.
`array_bounds.cc` handles declaration-time inference. Execution plans are grouped
into lifetime preparation, initialization, calls and cleanup, with named recursive
methods for consuming subobjects and initializer lists.

### Registration and resolution

Registration collects function signatures and bodies without traversing bodies.
Global initializers, defaults, bounds and enumerators retain their declaration
context. Body resolution enters persistent scopes and binds each local initializer
after inserting its own declaration, before inserting the next declarator.

Resolved identifier IDs freeze lookup decisions. Saving only a scope would expose
later declarations in that scope. Global object lookup uses a declaration-position
horizon, independent of symbol identity. Function signatures support forward calls;
a default argument must have been declared by the caller's visibility horizon.
Later defaults do not retroactively make an earlier call viable.

Goto labels are function-wide. Loop/switch targets have separate numeric control
identities. Named break/continue lookup uses the active loop stack, including a
trailing do-while label. Goto and switch dispatch may not enter initialized or
nontrivial automatic-object lifetimes. Static objects use their first-use guards.

Declaration formation sometimes needs constants before ordinary annotation:
array bounds, enumerators and const-object dependencies are evaluated in their
explicit saved scope. Const-object values undergo the object's destination
conversion before being published. Omitted array bounds are inferred from source
initializer shape early; element checking and runtime plans follow during annotation.
These are intentional declaration dependencies, not a second unrestricted name pass.

### Expression facts and normalization

Original expression metadata records intrinsic, undecayed type and lvalue/rvalue
category. Raw/value caches distinguish address/sizeof uses from contextual loads
and decay. `sizeof(a)` retains an array; a value use of `a` receives array decay.
Conditional lvalue operands retain their category before contextual conversion.

The conversion writer runs after checking. Arithmetic destination types and
selected constructor conversions are retained rather than selected again while
rewriting. Scalar casts preserve converted constant facts but do not inherit the
operand's binding or call identity.

For `double d=s+1` with `short s`, normalization records a load and integral
promotion, an int addition, and an int-to-double conversion. Compound assignment
uses a saved address placeholder so `a[i++] += 2.5` evaluates its target once.

Calls preserve the distinction between argument-list commas and a parenthesized
comma-expression argument. Instance methods remain bound to their receiver;
free/static functions decay normally and function-pointer fields are loaded.
Pointer-to-member values are outside this subset.

### Execution plans

Annotation publishes plans only after successful expression typing and rewriting.
The plans borrow typed AST expression pointers and canonical semantic IDs:

- Initialization steps specify value/string/zero/construction operations, types,
  byte offsets, receiver context and compact omitted-array ranges.
- Lifetime plans specify ordered base/member subobjects and construction/destruction
  traits. An unavailable or ambiguous default constructor is a semantic diagnostic
  when requested; an unused implicit operation is not emitted merely because its
  aggregate type exists.
- Call plans retain selected signature/member/symbol identity and all supplied and
  default argument expressions. Expressions are evaluated afresh at each call.
- Cleanup plans list automatic objects leaving on return, break, continue or goto,
  in reverse construction order. Lowering supplies storage and runtime activation
  flags, not a second lifetime-validity analysis.
- Local-object indexes avoid a complete symbol-table scan for each emitted function;
  prototype parameter symbols are excluded from runtime storage.

Initializer checking remains responsible for compatibility; the published plan is
the authoritative runtime subobject mapping. Lowering does not match braces,
rediscover defaults or perform overload selection.

### Public phase API and errors

`registerDeclarations`, `resolveAndRename`, `annotateTypes` and `check` expose
intermediate inspection. Each requires its preceding phase. Their boolean result
reports success; a phase value alone does not authorize lowering. Invalid top-level
registration stops analysis. Recoverable local errors allow later independent work.

`checkedProgram()` returns a view only after successful final checks. The compatibility
lowering API also checks AST identity, phase and diagnostics, so an analyzer cannot
accidentally be used to lower another tree.

## Target arithmetic and language policies

`base/target.hh` defines the fixed LP64 storage/width contract. `base/numeric.hh`
declares numeric values and operations; `base/numeric.cc` implements conversion and
evaluation shared by semantic
constants, conversion annotations and IR folding. Integer arithmetic wraps at its
width, including signed arithmetic; boolean conversion tests truth rather than
truncating to one bit. Floating operations round at each declared-width operation.
Unsupported folds, division by zero and out-of-range floating-to-integer conversions
remain unavailable constants rather than invoking undefined host operations.

Other intentional subset rules:

- Array bounds are positive integral constants; VLAs are unsupported.
- Globals are zero-filled, then all explicit global initializers run in source order
  before entry, including constant initializers. Local statics initialize on first use.
- Static guards are ordinary objects; exceptions and concurrent/reentrant static
  initialization are outside the current runtime contract.
- Overload selection filters count, collapses repeated callable identities, tries a
  unique exact match, then accepts only a unique viable conversion candidate.
  There is no promotion preference or per-argument ranking. Constructor argument
  conversions permit one user-defined conversion, not constructor chains.
- Constructor conversions are supported in call arguments; this does not implicitly
  introduce the same conversion policy into assignment and return checking.
- Nonempty braced construction of a class with user constructors is diagnosed;
  use `Type(arguments)`. Nontrivial union members, including arrays, are rejected.
- Parameters require names. Local aggregate definitions, `main(void)`, `long long`
  declarations and references remain outside the implemented syntax.
- `new(p, args...)` takes a pointer variable as a type witness. Its value is not
  read: `T* p = new(p, args...)` allocates and constructs one `T`. For a fixed
  array pointer `T (*p)[N]`, it constructs `N` elements and evaluates the
  remaining arguments for each element in order. `delete(p)` destroys them in
  reverse order and releases storage; deleting null does nothing.
  Allocation failure returns null and skips constructor arguments. These calls
  are compiler-provided; dynamic array lengths, placement allocation, custom
  allocators and exceptions are not implemented. The `__baka_allocate` and
  `__baka_deallocate` runtime names are reserved.
- `main` is an ordinary function for semantic checking. Native entry-point and ABI
  rules belong to a future backend. Existing fallthrough lowering produces a zero
  result; no all-paths-return analysis is implied.

Function declarations and unused function-pointer signatures may mention opaque
record parameters/results. Definitions and actual construction/calls require complete
value types; incomplete results are diagnosed before publishing a checked view.

## Validation

```
cmake -S src -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 custom_tests/semantic/run.py
python3 custom_tests/ir/run.py
```

The semantic suite keeps representative acceptance and rejection programs, with
short diagnostic fragments in `custom_tests/semantic/cases.json`. Related features
are exercised together in the IR fixtures: arithmetic, calls, arrays, aggregate
values, runtime initialization and object lifetimes.

Each IR fixture checks its printed TAC and expected execution results in one run.
The independent interpreter executes both optimized and unoptimized modules,
poisons unwritten automatic storage and zeroes static storage. A small verifier
test checks rejection of missing terminators and undefined temporaries. CTest and
`custom_test_hook` invoke these same stage runners.

See [IR_READINESS.md](IR_READINESS.md) for the consumer contract and
[../../ir_context.md](../../ir_context.md) for the current TAC model.
