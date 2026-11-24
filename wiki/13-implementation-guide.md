# 13. Implementation guide: data structures, algorithms and "how did we implement X?"

This page answers implementation questions across the whole compiler, not just codegen.
Part A describes the main data structures and algorithms in each stage. Part B is a
question-and-answer reference for specific language features. Pages 1–12 cover codegen in
more depth.

---

# Part A: data structures and algorithms by stage

## A1. Cross-cutting infrastructure

| Concern | Data structure | Notes |
|---|---|---|
| Reading the source | Memory-mapped file | The whole file is `mmap`ed and handled as a `string_view`; tokens point into it rather than copying text. |
| Line/column lookup for errors | Line index: a deque of `(start, end)` offsets per line | Filled while lexing, so a diagnostic can print the source line. |
| AST memory | **Bump-pointer arena** of fixed 1 KB chunks | Allocation rounds up to alignment and bumps an offset; a new chunk is added when one fills. Objects that need destructors are recorded in a linked list of type-erased "activation records", which the arena walks when it is destroyed. There is one arena for AST nodes and one for utilities, both global singletons. |
| Semantic entities | **Index-based arenas** (`std::deque` plus 32-bit IDs) | Types, symbols and scopes are referred to by `TypeId`, `SymbolId` and `ScopeId`. ID 0 always means "invalid/error". A deque never moves existing elements, so references stay valid as it grows. |
| Fixed target facts | `base/target.hh` | LP64: `int` 32 bits, `long` and pointers 64 bits. Shared by semantic layout and IR. |
| Arithmetic semantics | `base/numeric` | One implementation of wrapping integer and per-operation float arithmetic, shared by semantic constant evaluation, conversion folding and the IR constant folder, so compile-time and run-time results agree. Division by zero and out-of-range float→int produce "no constant" instead of being folded. |

## A2. Lexer

- **Algorithm:** a single hand-written pass over the characters, one `if`/`else` chain on
  the current character. No generated tables.
- **Identifiers and keywords:** scan the longest run of identifier characters, then look
  it up in a hash map from keyword text to token type. If it isn't found, it's an
  identifier.
- **Operators:** **maximal munch.** Start with one operator character and keep extending
  while the longer text is still a known operator in the operator hash map. That way `>>=`
  beats `>>`, which beats `>`.
- **Numbers:** decimal, hex and octal integers, floats, and suffixes (`L`, `U`, `UL`).
  `std::from_chars` parses the value and checks it fits the literal's type: an unsuffixed
  literal too large for `int` is an error, so large constants need `L` or `UL`.
- **Characters and strings:** escape sequences (simple escapes and octal up to three
  digits) are decoded during lexing.
- **Output:** two parallel arrays, tokens and their source locations.
- **Errors:** reported with a line and column, and lexing continues, so several errors can
  be shown at once.

## A3. Parser

- **Algorithm:** **recursive descent** for declarations and statements, and
  **precedence climbing** for binary expressions. A table maps each operator token to a
  precedence level, from comma (0) and assignment/ternary (1) up to `.` and `->` (15).
  Assignment and `?:` recurse at the same level, making them right-associative; other
  operators recurse at level+1, making them left-associative.
- **The typedef-name problem:** in C, `T * x;` is a declaration if `T` names a type and a
  multiplication otherwise. The parser keeps a **scoped type-name table** (a stack of hash
  maps) of names that are types (typedefs, structs, classes, unions, enums, plus the
  built-in `FILE`), and consults it while parsing. Entering a block pushes a map; leaving
  pops it.
- **AST:** a class hierarchy, allocated in the AST arena, with every node stamped with
  its source line and column. Each node has a `semanticInfo` slot that semantic analysis
  fills in later (type, value category, resolved symbol, constant value, jump target).
- **Visitors:** every concrete node kind implements `visitChildren`, which later passes
  use to walk the tree generically.

## A4. Semantic analysis

### Four phases over one AST

```
register declarations → resolve names → annotate types and normalize → final checks
```

1. **Registration** collects every top-level declaration (functions, globals, types)
   without entering function bodies, so functions can be called before they are defined.
2. **Resolution** enters scopes and binds every identifier to a `SymbolId`. The binding
   is frozen on the AST node, so later declarations can't change an earlier lookup.
3. **Annotation** computes each expression's type and value category, selects overloads,
   and inserts explicit conversion nodes into the AST. It then builds the **execution
   plans** that IR lowering consumes.
4. **Final checks** validate the queued facts, such as lvalue requirements and control
   flow rules.

Only after all four succeed is a "checked program" view published to IR lowering.

### The type table (`Types`)

- Every type is a record in an arena, identified by `TypeId`. Its kind is one of
  builtin, pointer, array, function, const, enum, struct, class or union.
- **Structural types are hash-consed (interned):** `pointer(T)`, `array(T, N)`,
  `const(T)` and `function(R, params, variadic)` are looked up in a hash table keyed by
  their parts, so the same type always gets the same ID. Type equality is then a single
  integer comparison.
- **Nominal types** (struct, class, union, enum) are never interned. Each declaration
  gets a fresh ID, and a typedef is just another name for that ID.
- **Definition state machine:** forward → defining → complete. A record is laid out once,
  when its definition finishes, and the layout (size, alignment, field offsets, base
  offset) is cached on the type.
- **Layout algorithm:**
  - The base class comes first, at offset 0.
  - Each field is placed at the next offset aligned to its own alignment.
  - The record's alignment is its largest field alignment, and its size is rounded up to
    that alignment.
  - Unions put every field at offset 0, with size equal to the largest field.
  - Empty records are 1 byte.
  - Static members, methods, nested types, constructors and destructors take no storage.
  - All arithmetic is overflow-checked.

### The symbol table (`Symbols`)

- **Scopes** form a tree stored in an arena. Each scope has a parent and a hash map from
  name to a *list* of symbol IDs. The list is how **overload sets** work: several
  functions share one name in one scope.
- Scope kinds: global, function, block, `for`, prototype and aggregate (class body).
- **Lookup** walks from the current scope up the parent chain.
- **Declaration points:** every declaration gets an increasing sequence number. A lookup
  of a global only sees declarations before the use point, giving C's "declare before
  use" rule even though all globals are registered up front.
- Each `Symbol` records its kind, type, scope, linkage (none, internal or external), a
  static flag, an optional compile-time constant (for enumerators and const objects),
  default-argument expressions and every redeclaration.
- Built-in library functions (`printf`, `scanf`, `atoi`, `fopen`, `fclose`, `fread`,
  `fwrite`, `read`, `write`) are pre-declared in the global scope. `FILE` is a pre-declared
  incomplete struct.

### Expressions and conversions

- An expression checker computes, for each expression node: its type, whether it is an
  lvalue or rvalue, any constant value, and the resolved symbol or call.
- Results are cached per node, with separate caches for "address" uses (`&x`,
  `sizeof x`, assignment targets) and "value" uses.
- **Conversions are made explicit in the AST.** After checking, a rewriting pass inserts
  implicit-cast nodes: lvalue-to-rvalue loads, array-to-pointer decay,
  function-to-pointer, integer promotions, arithmetic conversions, boolean conversion,
  and constructor conversions. IR lowering just follows these nodes and never re-derives
  C's conversion rules.

### Overload resolution

The policy is deliberately simple:
1. Drop candidates with the wrong number of arguments, allowing for defaults and
   variadics.
2. If exactly one candidate matches every argument exactly (ignoring top-level `const`),
   choose it.
3. Otherwise, if exactly one candidate is viable through implicit conversions, choose it.
4. Otherwise report "ambiguous" or "no viable overload".

There is no ranking of conversions. At most one user-defined constructor conversion is
allowed per argument.

### Constant evaluation

Array bounds, `case` labels, enumerators and `const` integer objects are evaluated at
compile time using the shared `numeric` library, with wrapping integer semantics.

### Execution plans: the hand-off to IR

Semantic analysis records every decision IR lowering needs, so lowering never re-runs
semantic logic:

| Plan | Contents |
|---|---|
| Initialization plan | A flat list of steps for initializing an object: store a value, copy a string, zero a range, or call a constructor, each at a byte offset. Brace initializers are fully matched to fields and elements here. |
| Lifetime plan | For each type: whether it needs construction or destruction, the chosen default constructor and its default arguments, and the ordered list of base and member subobjects. |
| Call plan | For each call: the selected function, its owner class, and the full argument list including default arguments. |
| Cleanup plan | For each `return`, `break`, `continue` and `goto`: the local objects that must be destroyed, in reverse construction order. |
| Local index | The local objects of each function. |

## A5. IR lowering

- **Output data structures:**
  - `Module`: lists of types, globals and functions.
  - `Function`: objects, temporaries, blocks.
  - `Block`: a list of `Instruction`s plus predecessor and successor lists.
  - `Instruction`: opcode, result, left and right operands, source span.
  - `Operand`: a tagged value, one of temporary, object, function, integer constant,
    float constant, or block.
- **Type import:** semantic types are translated to IR types, with records carrying their
  computed layout. A reverse map lets lowering ask semantic questions about an IR type.
- **Algorithm:** a recursive tree walk that emits into the "current block". The core
  helpers are:
  - `newBlock()`: create a block;
  - `select(b)`: make it the current block;
  - `branch(c, yes, no)` / `jump(b)`: terminate the current block;
  - `link(b)`: jump to `b` only if the current block has no terminator yet.
- **Expressions** lower through two mutually recursive functions: `value(e)` produces an
  operand holding the value, and `address(e)` produces a pointer to the lvalue.
- **Loop and switch targets** live in maps from the semantic control ID to the IR block
  for `break` and `continue`.
- **Object lifetime tracking** (see Part B) uses one boolean "active" flag per object
  that needs destruction, a stack of active cleanups for the current scope, and a list of
  pending temporaries to destroy at the end of the full expression.
- **Verification:** the verifier checks every type, operand, instruction shape, CFG edge
  and terminator. It also runs a **definite-assignment** analysis, which forward-iterates
  the set of temporaries certainly assigned on entry to each block until nothing changes.
  It is the slowest part of the compiler on large functions; see page 12.

## A6. IR analyses and optimization

- **CFG construction:** successor lists come from each block's terminator; predecessor
  lists are filled in by inverting them.
- **Liveness:** classic backward dataflow, `in = use ∪ (out − def)` and
  `out = ∪ in(successors)`, iterated over the blocks in reverse order until nothing
  changes. Sets are `std::set` of name IDs.
- **Local passes** (each confined to one block):
  - **Value numbering:** a list of "available" pure instructions. A new instruction that
    matches one, with commutative operands allowed in either order, becomes a copy of the
    earlier result. Redefining a temporary removes the facts that mention it.
  - **Memory forwarding:** a list of `address → value` facts. Loads that match a fact
    become copies. Stores add a fact, and forget facts about the same object or about any
    pointer-addressed memory. Calls forget everything.
  - **Dead code:** a backward scan starting from the block's live-out set.
  - **Unreachable blocks:** a worklist search from the entry block; anything not reached
    is deleted.

## A7. Code generation

Pages 5–10 cover this in depth. In brief:
- **Machine IR:** one record per assembly line, plus a static per-opcode table of
  reads, writes and effects.
- **Allocator:** next-use maps per instruction, register descriptors (register → names),
  and address descriptors (name → registers and an "in memory" flag), following the
  course lectures.
- **Peephole:** a bitset per instruction of registers live after it, computed by a
  backward scan, with rules applied repeatedly until nothing changes.

---

# Part B: "How did we implement…?"

### …`static` local variables?

- **Storage:** a `static` local becomes an IR **global** object, not a stack slot. It is
  zero-filled like every global and marked internal.
- **Initialization runs once, on first use:**
  - The lowerer creates a hidden boolean global, `.static_guard.<id>`, which starts as 0.
  - At the declaration it emits: load the guard; if it is set, skip ahead; otherwise run
    the initializer, register the object's destructor if it has one, and then set the
    guard to 1.
  - The guard is set only after initialization finishes.
- **Constant initializers too:** even a constant initializer runs at first use rather
  than being placed in the data section.
- **Limitations:** exceptions and thread-safe or reentrant static initialization are out
  of scope.

### …`static` at file scope, `static` members and `static` methods?

- **File-scope `static` globals and functions:** they get internal linkage, so they are
  not exported (no `.globl` in the assembly). Every defined symbol already has a unique
  `name.id` assembly name, so internal linkage mostly just controls visibility.
- **Static data members:** stored as ordinary globals named `Class.member`. They take no
  space inside the object, because layout skips static members.
- **Static methods:** compiled as ordinary functions named `Class.method`. Instance
  methods get an extra first parameter, `this`; static methods don't, and semantic
  analysis rejects any use of `this` or instance members inside them.

### …global variables and their initialization?

- All globals start zero-filled: in `.bss`, or in `.data`/`.rodata` for string literals.
- Every global with an initializer, a constructor or a destructor is set up at run time.
  The lowerer collects them into one generated function, `.module_init`, in source order.
  For each one it runs the initialization plan, then registers the destructor if there is
  one. This includes constant initializers like `int g = 5;`.
- `__baka_start` calls `.module_init` before `main`.
- The backend can emit link-time initial values (bytes, and addresses of other globals as
  relocations), but the lowerer currently only uses this for string literals.

### …constructors and destructors?

- **Every constructor and destructor becomes an IR function** taking `this` as its first
  parameter, named like `Class.ctor.N` or `Class.dtor`.
- **Implicit ones are generated when needed:** `Class.default_ctor` and
  `Class.default_dtor` when a member or base needs them, and `.array_dtor.N` for arrays of
  destructible objects.
- **Construction order:** base class first, then members in declaration order, then the
  constructor body. Destruction is the exact reverse: the destructor body, then members in
  reverse, then the base.
- **Arrays:** constructed element by element in a generated loop, unrolled for 16 or
  fewer elements, and destroyed in reverse order.

### …destructors when leaving a scope by `return`, `break`, `continue` or `goto`?

- Each local that needs destruction gets a boolean **active flag**. It is initialized to
  0 at the very top of the function, so every path sees a defined value, and set to 1 once
  the object is constructed.
- Leaving a scope normally destroys active objects in reverse order.
- For `return`, `break`, `continue` and `goto`, semantic analysis precomputes exactly
  which locals are being left (the cleanup plan). Before the jump, lowering emits "if
  flag: clear flag, call destructor" for each of them.
- The flag also handles objects whose construction was skipped by a branch.
- Semantic analysis rejects a `goto` or `switch` jump *into* the lifetime of an
  initialized or non-trivial object.

### …temporary objects (e.g. a function returning a class by value)?

- A temporary that needs destruction is stored in a hidden `.temporary` local, given an
  active flag, and added to a **pending list**.
- At the end of each full expression, `finishTemps()` destroys the pending temporaries in
  reverse order.
- When a temporary's ownership moves somewhere else (it initializes a variable, is passed
  by value, or is returned), lowering **transfers** it, so it isn't destroyed twice.
- The ternary operator merges the ownership of both arms.

### …inheritance?

- Single inheritance only. The base subobject sits at offset 0 of the derived layout.
- **Member lookup** searches the class, then its bases.
- **Calling an inherited method** adds up the base offsets to adjust `this` (always 0
  with single inheritance, but computed generally).
- No virtual functions: every call is resolved statically.

### …methods and `this`?

- A method is a function with an extra first parameter, a pointer to its class.
- `obj.m(args)` passes `&obj`, and `p->m(args)` passes `p`.
- Inside the method, unqualified member names are looked up through `this`.

### …function overloading?

- Overloads are several symbols under one name in a scope.
- Resolution uses the four-step policy in A4: arity filter, unique exact match, unique
  viable conversion, otherwise an error.
- Each overload gets a unique assembly name through its symbol ID (`name.id`), so no name
  mangling scheme is needed.

### …default arguments?

- The default expressions are stored on the function's symbol.
- At each call, the call plan appends the missing defaults, and they are **evaluated
  afresh at every call**, in the caller's context.
- A default only counts if it was declared before the call site.

### …function pointers?

- A function name used as a value decays to a function pointer (an explicit
  function-to-pointer conversion node).
- Calling through a pointer is an IR `call` whose callee is a temporary instead of a
  named function. In MIPS, the pointer is moved to `$t9` and called with `jalr`.

### …`if`/`else if`/`else`?

- Each arm evaluates its condition and branches to its body or to the next arm's test.
- Every body jumps to one shared join block.

### …`switch`?

- Before lowering the body, the lowerer collects every `case` and `default` label, even
  ones nested inside other statements, giving each its own block.
- Dispatch is a **linear chain of compare-and-branch**: `if (x == c1) goto L1;
  if (x == c2) goto L2; …`, then go to `default`, or to the exit if there is none. There
  is no jump table or binary search.
- The body is then lowered normally. Falling through between cases happens naturally
  because one case's block runs into the next. `break` jumps to the exit block.
- Duplicate and non-constant case labels are rejected during semantic analysis.

### …loops, `break` and `continue`?

- **`for`:** separate condition, body, update and exit blocks. `continue` goes to the
  update block, so the step always runs.
- **`while`:** `continue` goes to the condition block.
- **`do`/`while`:** the body comes first, then the condition.
- **Targets:** semantic analysis gives each loop and switch a numeric control ID and
  records the target on each `break` and `continue`. Lowering keeps a map from control ID
  to block.

### …`goto` and labels?

- Labels are function-wide. Each label gets a block, created on first mention, so forward
  `goto`s work.
- A `goto` runs the cleanups for any scopes it leaves, then jumps.

### …`&&`, `||` and `?:`?

- **Short-circuit:** `a && b` evaluates `a`, then branches either to a block that sets
  the result to 0 or to a block that evaluates `b`. Both meet at a join block. `||` is the
  mirror image.
- **Ternary:** branches to one of two arms, each of which copies its value into a shared
  temporary. When the ternary is used as an lvalue, each arm produces an address instead.
- **Comma:** evaluate the left side, discard it, and use the right side.

### …assignment, compound assignment and `++`/`--`?

- **Plain assignment:** compute the target's address, then the value, then store.
- **Compound assignment (`a[i++] += x`):** the target address is computed **once** and
  saved, then load, operate and store all go through it, so side effects in the target
  happen once.
- **`++` and `--`:** load, add or subtract 1, store. Pointers step by the pointee size.
  The postfix forms return the old value.

### …arrays, pointer arithmetic and multi-dimensional arrays?

- **Decay:** an array used as a value becomes a pointer to its first element, through an
  explicit array-to-pointer conversion node.
- **Indexing:** `a[i]` becomes `address + i * sizeof(element)` (an IR address-offset)
  followed by a load or store.
- **Multi-dimensional arrays** are arrays of arrays, so `m[i][j]` simply indexes twice.
- **Pointer differences** are divided by the element size.
- **Omitted array bounds** (`int a[] = {1,2,3}`) are inferred from the initializer during
  declaration processing.

### …`sizeof`?

- It is evaluated entirely at compile time from the cached layout and becomes an integer
  constant.
- `sizeof(array)` keeps the array type, because it is an "address use", so the array does
  not decay.

### …string and character literals?

- **Character literals** are integer constants, with escapes decoded by the lexer.
- **String literals** become read-only, internal global objects named `.string`, holding
  the bytes plus a terminating 0. They are emitted to `.rodata`, and the expression's value
  is a pointer to them.
- `char s[] = "hi";` copies the bytes into the array through an initialization plan step.

### …enums?

- An enum is a nominal type that behaves like a signed 32-bit integer in arithmetic.
- Each enumerator is a symbol holding a compile-time constant. A value without an
  initializer is the previous one plus 1.
- Uses of an enumerator become integer constants.

### …`typedef`?

- At parse time, the name goes into the scoped type-name table so the parser can tell
  declarations from expressions.
- In semantic analysis, it becomes an **alias symbol** pointing at an existing `TypeId`.
  No new type is created.

### …unions?

- Laid out with every member at offset 0, and size equal to the largest member, rounded
  to alignment.
- Members with non-trivial constructors or destructors are rejected.

### …structs and classes passed or returned by value?

- **In the IR,** records are first-class values: they can be loaded, stored, copied,
  passed and returned.
- **In MIPS** (page 7), a record argument is passed as a pointer to a caller-owned copy,
  which the callee copies into its own parameter. A record result is written through a
  hidden pointer passed in `$a0`.
- There is no return-value optimization: returning a local copies it, then destroys the
  local.

### …`new` and `delete`?

- **Syntax:** `T* p = new(p, args...)` uses `p` only as a type witness; its value is not
  read.
- **`new`:**
  - Lowering calls `__baka_allocate(sizeof(T), alignof(T))`.
  - If the result is non-null, it constructs `T` in place with the selected constructor,
    or element by element for a fixed-size array type.
  - On failure it returns null and skips construction, including evaluating the
    constructor arguments.
- **`delete(p)`:** if `p` is non-null, run the destructor(s) in reverse order, then call
  `__baka_deallocate`.
- **Run-time support:** the backend emits `__baka_allocate` (`malloc`, or `posix_memalign`
  for alignments over 16) and `__baka_deallocate` (`free`). The IR interpreter has
  matching built-in versions.

### …`printf`, `scanf`, `atoi` and other library calls?

- Pre-declared as built-in symbols with C signatures. `printf` and `scanf` are variadic.
- Calls lower to ordinary IR calls to *declared-only* functions. In assembly they keep
  their plain names and the static linker resolves them in glibc.
- Variadic arguments get C's default promotions (`float` → `double`, small integers →
  `int`) during semantic analysis.
- The IR interpreter can run `atoi` but no other library function, so fixtures that call
  `printf` or use files are checked only by the codegen suite.

### …`main` and program input?

- **Allowed signatures:** `main()` or `main(int argc, char** argv)`, returning an integer
  type or `void`. Semantic analysis rejects any other non-static `main`.
- **Input:** programs read their input with `int n = atoi(argv[1]);`.
- **Start-up:** the generated weak C `main` jumps to `__baka_start(argc, argv)`, which
  runs `.module_init` and then the user's `main`.
- **Interpreter:** the IR interpreter calls `main` as if run as `prog <n>`, with
  `argv = {"prog", "<n>", null}`.
- **Test harness:** the harness supplies its own `main` that calls `__baka_start` with the
  real `argc`/`argv`. See page 9.

### …`const`?

- `const` is a qualifier type wrapping another type.
- Assigning through a const lvalue, or initializing a non-const pointer from a const one,
  is rejected.
- Const globals are marked read-only in the IR. When they need run-time initialization,
  they are still written once by `.module_init`.
- Known gap: there are no `const` methods, so calling a method on a const object is not
  checked (page 12).

### …implicit conversions and integer promotion?

- Semantic analysis inserts explicit cast nodes for every conversion, so
  `double d = s + 1;` with `short s` becomes: load `s`, promote it to `int`, add as `int`,
  convert to `double`.
- Lowering emits an IR `cast` for each node, and the backend picks the machine
  instructions for each type pair.

### …integer overflow and other arithmetic edge cases?

- Integers **wrap** at their width in the language definition, in constant folding (the
  shared `numeric` library) and in generated code (MIPS 32-bit ops for `int`, plus
  re-normalization for narrower types).
- Constant folding refuses to fold anything that would be undefined on the host (division
  by zero, out-of-range float→int), leaving it to run time.

### …scopes, shadowing and declare-before-use?

- Scopes are nested in the scope tree, and an inner declaration hides an outer one of the
  same name.
- A local's initializer is resolved after its own name is inserted (so `int x = x + 1`
  refers to the new `x`) but before the next declarator in the same declaration.
- Globals obey declare-before-use through declaration points. Functions can be called
  before their definition because registration collects all signatures first.

### …register allocation, spilling and calls in the backend?

See page 6. In short:
- registers are chosen one block at a time, using next-use distances and descriptors;
- values still needed after a call prefer callee-saved registers;
- spills go to per-temporary stack slots created on demand;
- every live value is written back to memory at the end of each block.

### …floating-point constants in the backend?

- Each distinct bit pattern is stored once in a read-only literal pool and loaded by
  label.
- `0.0` comes straight from `$zero`.
