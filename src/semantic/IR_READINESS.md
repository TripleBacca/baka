# Readiness for IR generation

**Start the scalar/pointer/control-flow IR now. Full lowering of every accepted
program still needs the preparation described below.** No IR generator exists yet;
this assessment is from source inspection and the structural conversion tests.
An accepted program is not currently a promise that all lifetime/ABI behavior has
been normalized for a backend.

## Input contract already available

Pass a successfully checked `ProgramNode` together with the **same live**
`SemanticAnalyzer` (or an eventual module context owning its type/symbol arenas).
Require `SemanticPhase::Checked` and no diagnostics. Do not lower a failed or
partially annotated tree. The lowering API can accept const references.

| Lowering requirement | Existing semantic information |
| --- | --- |
| Expression type and address/value category | `ASTSemanticInfo::Type`, `LValue`; load and conversion nodes make value uses explicit |
| Name identity | Resolved `SymbolId`, identifier bindings/owned resolved name; use IDs rather than source spelling |
| Object storage and linkage | `Symbols::get(id)`: type, scope, kind, `isStatic`, linkage and declaration source |
| Function result/parameter types and variadics | Function `Type`: element, parameters, variadic; declaration/parameter bindings are attached |
| Numeric signedness/width and layout | `Types::arithmetic`, `Types::layout`; fixed LP64 contract |
| Arithmetic operand and destination conversions | `ImplicitCastNode` chains, explicit source casts, annotated operator results |
| Converting constructor argument | `ImplicitCastNode(ConstructorConversion)`, converted operand and selected constructor `Construction`/`CallSignature`/`MemberOwner`/`MemberIndex` |
| Indirect calls and overload choice | Call `CallSignature`; ordinary selected function `Symbol`; callee value load/decay preserved |
| Explicit field/method access | `MemberOwner`, one-based `MemberIndex`, selected symbol when available; retain receiver AST |
| Explicit construction expressions | `Construction`, selected constructor `CallSignature` and owner/member index; implicit construction has no selected constructor index |
| Field offset | `types.get(types.unConst({MemberOwner})).objectLayout->offsets[MemberIndex-1]`; stored on successful definition completion, not recomputed per access. IR owns a copy in `Type::memberOffsets`/`baseOffset`. Inherited access follows stored base offsets. |
| Array indexing/sizeof | Element/pointee type, known bounds, layout and constant sizeof result; contextual decay is explicit |
| Loop/switch/goto identities | `Target` on targets and resolved jumps; preserve break versus continue to choose exit/header/update block |
| Conditional/short-circuit expressions | Distinct AST operators; boolean conversions on their conditions/operands |
| Compound assignment | Evaluate `CompoundAssignmentNode::Target` once and bind the address for `CompoundTargetNode` reads in Value |
| Optional default arguments | Typed default expression ASTs retained by `Symbol::defaultArguments`; actual call argument list remains unexpanded |

The member-call scenario in `member_call_tests.cc` additionally proves that
instance-method calls keep their receiver without ordinary function decay, while
function-pointer fields retain an indirect pointer load. Free/static function
callees still decay. The six conversion scenarios in `custom_tests/semantic/conversion_tests.cc` test
numeric operations, contextual decay, calls/defaults/ellipsis, single-evaluation
compound assignment, pointer conversions, and persisted member/constructor facts.
They also verify conversion type consistency and that final checks leave the
rewritten tree unchanged.

For a first milestone, lower scalar locals/parameters, expressions, explicit and
implicit conversions, ordinary direct/indirect calls with supplied arguments,
returns, pointer/index operations, `sizeof`, conditionals, loops and goto. Build
control-flow blocks for short-circuit operators and conditional expressions; an
explicit boolean cast does not authorize evaluating an unselected branch.

## Integration prerequisite: arena lifetime

`driver.cc` currently creates `SemanticAnalyzer Analyzer` inside the semantic-stage
block. Its arenas are destroyed when that block ends. The AST remains allocated,
but its IDs cannot be interpreted after that point. Invoke the future generator
inside that scope, or move the analyzer into a compilation/module object whose
lifetime includes lowering. Do not persist only the annotated AST and discard the
semantic context. Backend temporaries, stack slots, basic blocks and SSA values
belong to IR generation and do not need semantic AST fields.

## Work before supporting every accepted feature

- [ ] **Resolved default arguments per call.** Calls such as `f()` retain zero
  supplied arguments. Ordinary defaults can be read from the selected symbol and
  evaluated at each call; never evaluate/cache their runtime values at declaration
  time. Prefer a resolved argument plan (with default-expression references and
  declaration-point bindings) so IR need not rediscover defaults. Constructors
  additionally need defaults from the selected special-member declaration; they
  do not have ordinary function symbols. Also keep missing arguments distinct
  from ellipsis arguments. The present tests establish typed defaults, not
  expanded call arguments.
- [ ] **Normalized initialization destinations.** Scalar element casts exist,
  but brace elision, zero-filled omitted elements, string-array copies, base/member
  order and aggregate copies remain represented by source initializer syntax.
  Add an initialization plan mapping expressions to subobject paths/offsets and
  recording zero/copy operations, or explicitly restrict the first backend to
  simpler initialization. IR should not repeat the semantic initializer matcher.
- [ ] **Implicit construction, temporaries and destruction.** An explicit
  constructor expression records its selected declaration. Plain object
  declarations and aggregate temporaries do not yet carry a complete execution
  plan for implicit constructors/copy operations, materialized temporary storage,
  destruction and full-expression lifetimes.
- [ ] **Cleanup edges.** The checker validates jumps and scopes, and services can
  enumerate objects/subobjects leaving scopes. Final AST jumps do not contain
  cleanup lists. Add explicit cleanup plans for return/break/continue/goto and
  normal scope exit before generating nontrivial objects. Do not infer full object
  lifetime behavior merely from a numeric `Scope` or `Target` field.
- [ ] **Implicit member receiver contract.** Unqualified member expressions retain
  their owner/member identity and instance methods have synthetic `this` symbols.
  Make the receiver convention explicit in lowering (or normalize implicit member
  access to a receiver AST). Instance methods need a hidden receiver parameter;
  static methods do not. Choose names/ABI identities for constructors/destructors,
  which currently use owner/member identity rather than ordinary `SymbolId`s.

Some choices belong to the backend rather than extra semantic metadata: concrete
IR types, calling convention, hidden result storage for aggregate returns,
symbol mangling, string/global emission, stack layout and optimization policy.
Choose them against the fixed LP64 layout before mixing object files with a
system ABI. Nonconstant arrays and `new`/`delete` remain unsupported.

## Constant values are not a complete optimizer

The current evaluator covers a subset of arithmetic and uses a signed 64-bit
integer carrier. It is not a complete target-precision C/C++ evaluator, especially
for unsigned boundary arithmetic and all floating-point cases. The conversion
nodes encode runtime operations independently of that evaluator. Do not use every
existing `ConstantExpression`/constant cache as a blanket optimization oracle;
expand target-accurate folding and its tests before folding arbitrary accepted
expressions. Required compile-time contexts remain subject to the evaluator's
existing limitations, documented in TODO.md.

The starting IR milestone therefore does not require another name-resolution or
promotion pass. Full-language lowering needs the normalization/lifetime work above
and a target/ABI decision, rather than rediscovering types from source names.
