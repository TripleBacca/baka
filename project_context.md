# Project context for three-address-code IR generation

## Scope and evidence

- Snapshot: current dirty working tree at HEAD `d10af29`, not just the committed compiler. Repository root: `/home/saturn/CLionProjects/bakabaka`.
- Purpose: self-contained implementation context for a typed three-address-code (TAC) lowerer. No compiler changes were made during this reconnaissance.
- **Verified** means inspected implementation or an execution recorded below. **Historical** means a repository document describes an older state. **Unverified / decision needed** is stated explicitly.
- All quoted definitions are copied from the current working tree, with paths. Appendix A includes **all 32 AST headers verbatim**, including inactive legacy nodes. Appendices contain the actual token definitions, parser implementation, semantic interfaces, and relevant design documents so a reader does not need repository access.
- Build/test/probe artifacts were written under `/tmp/baka-project-context-z90b9935`, not into this repository. Python was run with `-B` to prevent bytecode-cache writes. Only this report is an intended repository modification.
- Local credential/assistant/IDE contents were not inspected or reproduced. No environment variables, tokens or credentials are included.

## 1. Repository overview

### 1.1 Git state at reconnaissance start

These outputs predate creation of this report. `git diff HEAD --stat` excludes untracked content; important semantic implementation files are untracked. Therefore HEAD alone is **not** a reproducible copy of the compiler described here.

Command: `git status`

```text
On branch saturn/slop
Your branch is up to date with 'origin/saturn/slop'.

Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git restore <file>..." to discard changes in working directory)
	modified:   Makefile
	modified:   README.md
	modified:   custom_test_hook
	modified:   custom_tests/parse/00_control_flow.expected
	modified:   custom_tests/parse/00_control_flow_sub.expected
	modified:   custom_tests/parse/03_expressions.expected
	modified:   custom_tests/parse/03_expressions_sub.expected
	modified:   custom_tests/parse/04_structures.expected
	modified:   custom_tests/parse/04_structures_sub.expected
	modified:   custom_tests/parse/05_classes_oop.expected
	modified:   custom_tests/parse/05_classes_oop_sub.expected
	modified:   custom_tests/parse/06_unions_enums.expected
	modified:   custom_tests/parse/06_unions_enums_sub.expected
	modified:   custom_tests/parse/07_top_level_program.expected
	modified:   custom_tests/parse/07_top_level_program_sub.expected
	modified:   custom_tests/parse/08_class_destructors.expected
	modified:   custom_tests/parse/08_class_destructors_sub.expected
	modified:   custom_tests/parse/18_invalid_do_while.expected
	modified:   custom_tests/parse/20_invalid_while_label_typename.expected
	modified:   custom_tests/parse/26_invalid_cascade_case_colon.expected
	modified:   custom_tests/parse/37_invalid_mixed_calculator.expected
	modified:   custom_tests/parse/37_invalid_mixed_calculator_sub.expected
	modified:   custom_tests/parse/40_invalid_mixed_goto_loops.expected
	modified:   custom_tests/parse/40_invalid_mixed_goto_loops_sub.expected
	modified:   source_code
	modified:   src/CMakeLists.txt
	modified:   src/driver/CMakeLists.txt
	modified:   src/driver/args.cc
	modified:   src/driver/driver.cc
	modified:   src/driver/gctx.hh
	modified:   src/parser/class.cc
	modified:   src/parser/compoundStatement.cc
	modified:   src/parser/constructor.cc
	modified:   src/parser/declaration.cc
	modified:   src/parser/destructor.cc
	modified:   src/parser/enums.cc
	modified:   src/parser/expressionStatement.cc
	modified:   src/parser/expressions.cc
	modified:   src/parser/function.cc
	modified:   src/parser/identifier.cc
	modified:   src/parser/initializer.cc
	modified:   src/parser/iterationStatement.cc
	modified:   src/parser/jumpStatement.cc
	modified:   src/parser/labelStatement.cc
	modified:   src/parser/parser.cc
	modified:   src/parser/parser.hh
	modified:   src/parser/selectionStatement.cc
	modified:   src/parser/sizeof.cc
	modified:   src/parser/statement.cc
	modified:   src/parser/struct.cc
	modified:   src/parser/structBody.cc
	modified:   src/parser/type.cc
	modified:   src/parser/typedef.cc
	modified:   src/parser/typename.cc
	modified:   src/parser/union.cc
	modified:   src/semantic/aggregates.hh
	modified:   src/semantic/ast_declarations.hh
	modified:   src/semantic/declarations.hh
	modified:   src/semantic/symbols.hh
	modified:   src/semantic/types.hh
	modified:   src/types/driver/defs.hh
	modified:   src/types/parser/ast/ast_node.hh
	modified:   src/types/parser/ast/castFactorNode.hh
	modified:   src/types/parser/ast/constant.hh
	modified:   src/types/parser/ast/constructor.hh
	modified:   src/types/parser/ast/declaration.hh
	modified:   src/types/parser/ast/destructor.hh
	modified:   src/types/parser/ast/enums.hh
	modified:   src/types/parser/ast/expression.hh
	modified:   src/types/parser/ast/function.hh
	modified:   src/types/parser/ast/identifier.hh
	modified:   src/types/parser/ast/initializer.hh
	modified:   src/types/parser/ast/iterationStatement.hh
	modified:   src/types/parser/ast/jump.hh
	modified:   src/types/parser/ast/labelStatement.hh
	modified:   src/types/parser/ast/program.hh
	modified:   src/types/parser/ast/selectionStatement.hh
	modified:   src/types/parser/ast/sizeof.hh
	modified:   src/types/parser/ast/statement.hh
	modified:   src/types/parser/ast/structBody.hh
	modified:   src/types/parser/ast/ternary.hh
	modified:   src/types/parser/ast/utils.hh

Untracked files:
  (use "git add <file>..." to include in what will be committed)
	.codex/
	context_B.md
	context_C.md
	custom_tests/semantic/
	semantic_audit/
	src/semantic/CMakeLists.txt
	src/semantic/IR_READINESS.md
	src/semantic/README.md
	src/semantic/TODO.md
	src/semantic/aggregates.cc
	src/semantic/annotation.cc
	src/semantic/ast_declarations.cc
	src/semantic/checks.cc
	src/semantic/conversions.cc
	src/semantic/declarations.cc
	src/semantic/expressions.cc
	src/semantic/expressions.hh
	src/semantic/registration.cc
	src/semantic/resolution.cc
	src/semantic/semantic_analyzer.cc
	src/semantic/semantic_analyzer.hh
	src/semantic/symbols.cc
	src/semantic/types.cc
	src/types/parser/ast/implicitCast.hh

no changes added to commit (use "git add" and/or "git commit -a")
```

Command: `git status --short` (complete collapsed-directory listing)

```text
 M Makefile
 M README.md
 M custom_test_hook
 M custom_tests/parse/00_control_flow.expected
 M custom_tests/parse/00_control_flow_sub.expected
 M custom_tests/parse/03_expressions.expected
 M custom_tests/parse/03_expressions_sub.expected
 M custom_tests/parse/04_structures.expected
 M custom_tests/parse/04_structures_sub.expected
 M custom_tests/parse/05_classes_oop.expected
 M custom_tests/parse/05_classes_oop_sub.expected
 M custom_tests/parse/06_unions_enums.expected
 M custom_tests/parse/06_unions_enums_sub.expected
 M custom_tests/parse/07_top_level_program.expected
 M custom_tests/parse/07_top_level_program_sub.expected
 M custom_tests/parse/08_class_destructors.expected
 M custom_tests/parse/08_class_destructors_sub.expected
 M custom_tests/parse/18_invalid_do_while.expected
 M custom_tests/parse/20_invalid_while_label_typename.expected
 M custom_tests/parse/26_invalid_cascade_case_colon.expected
 M custom_tests/parse/37_invalid_mixed_calculator.expected
 M custom_tests/parse/37_invalid_mixed_calculator_sub.expected
 M custom_tests/parse/40_invalid_mixed_goto_loops.expected
 M custom_tests/parse/40_invalid_mixed_goto_loops_sub.expected
 M source_code
 M src/CMakeLists.txt
 M src/driver/CMakeLists.txt
 M src/driver/args.cc
 M src/driver/driver.cc
 M src/driver/gctx.hh
 M src/parser/class.cc
 M src/parser/compoundStatement.cc
 M src/parser/constructor.cc
 M src/parser/declaration.cc
 M src/parser/destructor.cc
 M src/parser/enums.cc
 M src/parser/expressionStatement.cc
 M src/parser/expressions.cc
 M src/parser/function.cc
 M src/parser/identifier.cc
 M src/parser/initializer.cc
 M src/parser/iterationStatement.cc
 M src/parser/jumpStatement.cc
 M src/parser/labelStatement.cc
 M src/parser/parser.cc
 M src/parser/parser.hh
 M src/parser/selectionStatement.cc
 M src/parser/sizeof.cc
 M src/parser/statement.cc
 M src/parser/struct.cc
 M src/parser/structBody.cc
 M src/parser/type.cc
 M src/parser/typedef.cc
 M src/parser/typename.cc
 M src/parser/union.cc
 M src/semantic/aggregates.hh
 M src/semantic/ast_declarations.hh
 M src/semantic/declarations.hh
 M src/semantic/symbols.hh
 M src/semantic/types.hh
 M src/types/driver/defs.hh
 M src/types/parser/ast/ast_node.hh
 M src/types/parser/ast/castFactorNode.hh
 M src/types/parser/ast/constant.hh
 M src/types/parser/ast/constructor.hh
 M src/types/parser/ast/declaration.hh
 M src/types/parser/ast/destructor.hh
 M src/types/parser/ast/enums.hh
 M src/types/parser/ast/expression.hh
 M src/types/parser/ast/function.hh
 M src/types/parser/ast/identifier.hh
 M src/types/parser/ast/initializer.hh
 M src/types/parser/ast/iterationStatement.hh
 M src/types/parser/ast/jump.hh
 M src/types/parser/ast/labelStatement.hh
 M src/types/parser/ast/program.hh
 M src/types/parser/ast/selectionStatement.hh
 M src/types/parser/ast/sizeof.hh
 M src/types/parser/ast/statement.hh
 M src/types/parser/ast/structBody.hh
 M src/types/parser/ast/ternary.hh
 M src/types/parser/ast/utils.hh
?? .codex/
?? context_B.md
?? context_C.md
?? custom_tests/semantic/
?? semantic_audit/
?? src/semantic/CMakeLists.txt
?? src/semantic/IR_READINESS.md
?? src/semantic/README.md
?? src/semantic/TODO.md
?? src/semantic/aggregates.cc
?? src/semantic/annotation.cc
?? src/semantic/ast_declarations.cc
?? src/semantic/checks.cc
?? src/semantic/conversions.cc
?? src/semantic/declarations.cc
?? src/semantic/expressions.cc
?? src/semantic/expressions.hh
?? src/semantic/registration.cc
?? src/semantic/resolution.cc
?? src/semantic/semantic_analyzer.cc
?? src/semantic/semantic_analyzer.hh
?? src/semantic/symbols.cc
?? src/semantic/types.cc
?? src/types/parser/ast/implicitCast.hh
```

Command: `git log -10 --oneline`

```text
d10af29 Add semantic type/symbol/declaration services and parser repairs
b6f9a26 format testcases
f547e32 chore: spellchecks
639876d Hotfix
73486f4 trigger submission gen on tag push instead of checkpoint PR
dd964c3 remove cached build files produced by clion
7b0b43e fix make help recipe
f784fd6 update Makefile, README for assignment 2
19eaee6 fix xor shift
6637a39 Update Compiler Explorer link in README
```

Command: `git diff HEAD --stat`

```text
 Makefile                                           |   8 +-
 README.md                                          |   7 +-
 custom_test_hook                                   |   8 +-
 custom_tests/parse/00_control_flow.expected        |  94 ++--
 custom_tests/parse/00_control_flow_sub.expected    |  94 ++--
 custom_tests/parse/03_expressions.expected         |   6 +-
 custom_tests/parse/03_expressions_sub.expected     |   6 +-
 custom_tests/parse/04_structures.expected          |   6 +-
 custom_tests/parse/04_structures_sub.expected      |   6 +-
 custom_tests/parse/05_classes_oop.expected         |  76 +--
 custom_tests/parse/05_classes_oop_sub.expected     |  76 +--
 custom_tests/parse/06_unions_enums.expected        |   2 +
 custom_tests/parse/06_unions_enums_sub.expected    |   2 +
 custom_tests/parse/07_top_level_program.expected   |  39 +-
 .../parse/07_top_level_program_sub.expected        |  39 +-
 custom_tests/parse/08_class_destructors.expected   | 148 +++---
 .../parse/08_class_destructors_sub.expected        | 148 +++---
 custom_tests/parse/18_invalid_do_while.expected    |   8 +-
 .../parse/20_invalid_while_label_typename.expected |  13 +-
 .../parse/26_invalid_cascade_case_colon.expected   |  28 +-
 .../parse/37_invalid_mixed_calculator.expected     |  42 +-
 .../parse/37_invalid_mixed_calculator_sub.expected |  42 +-
 .../parse/40_invalid_mixed_goto_loops.expected     |  50 +-
 .../parse/40_invalid_mixed_goto_loops_sub.expected |  50 +-
 source_code                                        |  14 +-
 src/CMakeLists.txt                                 |  26 +
 src/driver/CMakeLists.txt                          |   1 +
 src/driver/args.cc                                 |   5 +-
 src/driver/driver.cc                               |  65 ++-
 src/driver/gctx.hh                                 |   2 +-
 src/parser/class.cc                                |  13 +-
 src/parser/compoundStatement.cc                    |  74 ++-
 src/parser/constructor.cc                          |   3 +-
 src/parser/declaration.cc                          |  26 +-
 src/parser/destructor.cc                           |   7 +-
 src/parser/enums.cc                                |   3 +-
 src/parser/expressionStatement.cc                  |   3 +-
 src/parser/expressions.cc                          |  55 +-
 src/parser/function.cc                             |  21 +-
 src/parser/identifier.cc                           |   5 +-
 src/parser/initializer.cc                          |   4 +-
 src/parser/iterationStatement.cc                   | 105 ++--
 src/parser/jumpStatement.cc                        |  17 +-
 src/parser/labelStatement.cc                       |  31 +-
 src/parser/parser.cc                               |  49 +-
 src/parser/parser.hh                               |  20 +
 src/parser/selectionStatement.cc                   |  18 +-
 src/parser/sizeof.cc                               |  18 +-
 src/parser/statement.cc                            |  12 +-
 src/parser/struct.cc                               |  13 +-
 src/parser/structBody.cc                           |   5 +-
 src/parser/type.cc                                 |   7 +-
 src/parser/typedef.cc                              |   3 +-
 src/parser/typename.cc                             |   3 +-
 src/parser/union.cc                                |   5 +-
 src/semantic/aggregates.hh                         |  98 +---
 src/semantic/ast_declarations.hh                   | 201 +-------
 src/semantic/declarations.hh                       | 332 +-----------
 src/semantic/symbols.hh                            | 252 ++-------
 src/semantic/types.hh                              | 566 ++-------------------
 src/types/driver/defs.hh                           |   5 +-
 src/types/parser/ast/ast_node.hh                   |  46 ++
 src/types/parser/ast/castFactorNode.hh             |   8 +-
 src/types/parser/ast/constant.hh                   |   5 +
 src/types/parser/ast/constructor.hh                |   3 +
 src/types/parser/ast/declaration.hh                |   4 +-
 src/types/parser/ast/destructor.hh                 |   2 +
 src/types/parser/ast/enums.hh                      |   2 +
 src/types/parser/ast/expression.hh                 |  60 ++-
 src/types/parser/ast/function.hh                   |  11 +-
 src/types/parser/ast/identifier.hh                 |  14 +
 src/types/parser/ast/initializer.hh                |   9 +-
 src/types/parser/ast/iterationStatement.hh         |  24 +
 src/types/parser/ast/jump.hh                       |  13 +
 src/types/parser/ast/labelStatement.hh             |  11 +
 src/types/parser/ast/program.hh                    |   4 +
 src/types/parser/ast/selectionStatement.hh         |  22 +
 src/types/parser/ast/sizeof.hh                     |   6 +-
 src/types/parser/ast/statement.hh                  |  46 +-
 src/types/parser/ast/structBody.hh                 |   2 +
 src/types/parser/ast/ternary.hh                    |   8 +-
 src/types/parser/ast/utils.hh                      |  12 +
 82 files changed, 1394 insertions(+), 2003 deletions(-)
```

### 1.2 Directory tree and purposes

Depth is 2–3 levels; larger test corpora are grouped. Generated build directories, Python caches, Git internals and local credentials are excluded. Key AST paths are expanded in Appendix A.

```text
.
├── Makefile                         # build/run/custom-test/submission commands
├── Justfile                         # alternate build/test recipes; also invokes book test_hook
├── README.md                        # project features, assumptions, commands; some broad claims exceed implementation
├── source_code                      # tracked, currently modified example input; also matches .gitignore
├── context_A.md                     # historical type/symbol/declaration handoff
├── context_B.md                     # historical expression/call/initialization handoff
├── context_C.md                     # historical semantic-driver/control-flow handoff
├── todo                             # short historical error-handling TODO
├── src/
│   ├── CMakeLists.txt               # C++23 project, compiler + libraries + 18 CTest entries
│   ├── main.cc                      # forwards to baka::driver::run
│   ├── .clangd                      # compilation database and C++23 editor flags
│   ├── base/                        # POSIX mapped file, line index, ASCII helpers, memory/scopes
│   │   ├── memory/                  # arena allocation and literal-string storage
│   │   └── symbol_table/            # parser-only scoped name table, distinct from semantic Symbols
│   ├── driver/                      # CLI, global context, diagnostics, frontend orchestration
│   │   ├── args.cc                  # textually included by driver.cc, not a separate CMake TU
│   │   ├── driver.cc                # lexer → parser → semantic pipeline; no backend
│   │   └── gctx.hh / gctx.cc        # global options, mapped source and accumulated diagnostics
│   ├── lexer/                       # handwritten tokenizer, keyword/operator maps
│   │   ├── lexer.hh / lexer.cc      # Lexed Tokenize(std::string_view)
│   │   └── definitions.txt          # informal planning vocabulary, not an implemented grammar
│   ├── parser/                      # handwritten recursive descent + precedence climbing
│   │   ├── parser.hh / parser.cc    # parser state/API, root dispatch, constants, recovery
│   │   ├── expressions.cc           # primary/unary/postfix/binary/comma/conditional syntax
│   │   ├── declaration.cc           # pointer/array/function declarator construction
│   │   ├── function.cc              # parameter lists and speculative function definitions
│   │   ├── *Statement.cc            # compounds, labels, loops, selections and jumps
│   │   └── ReferenceGrammerANSIC2011 # bundled C99 reference despite filename; not baka grammar
│   ├── semantic/                    # compiled semantic library; many files untracked
│   │   ├── semantic_analyzer.hh/.cc # owned arenas, saved contexts, pass API/work queues
│   │   ├── registration.cc          # top-level signatures/declarations and work collection
│   │   ├── resolution.cc            # lexical names/scopes/renames and control targets
│   │   ├── annotation.cc            # mutating expression typing + conversion materialization
│   │   ├── checks.cc                # final read-only checks and diagnostic collection
│   │   ├── expressions.hh/.cc       # expression typing, calls, constants, initialization
│   │   ├── conversions.cc           # actual implicit-cast and compound-assignment AST rewriting
│   │   ├── types.hh/.cc             # canonical/nominal type arena and fixed LP64 layout
│   │   ├── symbols.hh/.cc           # stable symbol/scope arenas and declaration compatibility
│   │   ├── declarations.hh/.cc      # language type/storage/member constraints
│   │   ├── ast_declarations.hh/.cc  # declarator/type-name interpretation in current scope
│   │   ├── aggregates.hh/.cc        # aggregates, member signatures and inherited lookup
│   │   ├── README.md / TODO.md      # current pass contracts and grouped acceptance checklist
│   │   ├── IR_READINESS.md          # current lowering boundary and known normalization gaps
│   │   └── .clang-format            # module-local formatting policy
│   └── types/
│       ├── token/                   # TokenType, Token, positions and token printers
│       ├── parser/                  # ast/ contains all syntax + semantic-only node definitions
│       ├── driver/                  # Stage and BuildType enums
│       └── exceptions.hh            # diagnostic/driver exception classes and rendering macros
├── custom_tests/
│   ├── lex/                        # 10 lexer source/snapshot pairs
│   ├── parse/                      # 57 parser source/snapshot pairs
│   └── semantic/
│       ├── cases/                  # 155 positive/negative source programs
│       ├── diagnostics/            # 32 multiline diagnostic/recovery source programs
│       ├── cases.json              # expected exit code + diagnostic stage
│       ├── diagnostics.json        # exact message/line/column/note expectations
│       ├── pass_tests.cc           # 11 structural semantic-phase scenarios
│       ├── conversion_tests.cc     # 6 structural conversion/metadata scenarios
│       ├── member_call_tests.cc    # bound-method vs function-pointer structural scenario
│       └── run.py                  # semantic regression runner (205 checks)
├── custom_test_hook                 # Python golden-output harness + semantic runner delegation
├── test_compiler                    # entry point to book-style Python unittest framework
├── test_hook                        # runs chapter/stage pairs from active_tests
├── active_tests                     # currently only a comment: test_hook selects no chapters
├── expected_results.json            # book test-result metadata, not proof of current compiler correctness
├── test_properties.json             # book test properties and platform expectations
├── test_framework/
│   ├── runner.py / basic.py         # chapter selection and compile/link/run test machinery
│   ├── parser/                     # parser/model for inspecting emitted assembly; NOT compiler AST/IR
│   ├── tacky/                      # optimization/assembly expectations; NOT a TAC generator
│   ├── regalloc.py                 # register-allocation testing helpers
│   └── test_tests/                 # tests of the test framework itself
├── tests/
│   ├── chapter_1 ... chapter_20/    # external-style C compiler corpus, not all supported by baka
│   ├── chapter_18/                 # aggregate/member/layout/ABI test documentation
│   └── chapter_19/                 # constant folding, copy propagation, unreachable/dead-store tests
├── templates/
│   ├── includes/                   # harness assembly/C templates
│   └── chapter_20_templates/       # register-allocation harness fixtures, not backend emitters
├── semantic_audit/
│   ├── CHECKLIST.md / TASK_SPLIT.md # historical comprehensive audit and three-person work allocation
│   ├── RESULTS.md / programs/      # historical lexer/parser audit evidence
│   ├── extended/                   # C-reference/loop-label audit notes and programs
│   └── teammate_a/                 # type policy, two strict C++ tests and parser regression runner
├── submission_gen/                 # assignment_1_lexer.sh and assignment_2_parser.sh packaging scripts
├── gen_submissions                  # runs packaging scripts, writes submissions/; not used in reconnaissance
└── .github/workflows/               # build/test CI and submission archive automation
```

Local `.agents/`, `.aws/`, `.codex/`, `.commandcode/` and `.idea/` are omitted from the tree's contents. Their configuration/credential values were not inspected. Build/dependency/cache directories were not expanded.

### 1.3 Build system, versions and dependencies

- Implementation: C++23 with GNU extensions (`-std=gnu++23` in the fresh build), namespace `baka`; CMake ≥3.10, Make recipes. Project version is `1.0`.
- Fresh environment: GCC `16.2.1 20260810`, CMake `4.4.4`, Python `3.14.7`; clang-format/clang-tidy LLVM `23.1.1` are installed.
- CMake has no `find_package`, dependency fetch, parser generator, LLVM link, or third-party C++ library dependency. Uses the C++ standard library and Linux/POSIX APIs (`mmap`, `fcntl`, `unistd`, `syscall.h`). Other OS portability is **unverified**. The GCC build passes; the Clang frontend used by clang-tidy rejects incomplete aggregate-pointer casts in `statement.hh` (see verification addendum). A full Clang build was not attempted.
- Python custom tests use the standard library. The broader chapter framework expects system C compilation/linking and platform tools when running backend tests; its full setup was not validated here.
- `-Wall -Wextra -Wpedantic -O2` is applied to the **baka executable target**, not uniformly to every library. `--build_type` updates Gctx; it does not reconfigure CMake optimization/debug flags.

The driver unconditionally prints the lexer token table, including without `--verbose`; successful semantic analysis also prints the annotated AST. Stdout is therefore mixed diagnostic/debug output, not a machine-readable IR stream.

Normal repository commands, verified against Makefile/CLI:

```bash
make build
./build/baka path/to/file.cpp                  # defaults to semantic analysis, prints annotated AST on success
./build/baka --lex path/to/file.cpp
./build/baka --parse --verbose path/to/file.cpp
./build/baka --semantic path/to/file.cpp
make semantic file=path/to/file.cpp
./custom_test_hook lex
./custom_test_hook parse
./custom_test_hook semantic
./custom_test_hook                            # all available custom suites
make test-semantic                            # rebuild, then semantic suite
make test                                    # rebuild, then custom hook (not the book corpus)
ctest --test-dir build --output-on-failure
python3 -B semantic_audit/teammate_a/run.py --compiler ./build/baka
```

`make parse-folder` writes `.out` files beside its inputs; `bless` replaces expected snapshots; `make submit` and `make clean` are mutating operations. None was used here.

### 1.4 Commands actually executed and results

Fresh build and all outputs were outside the repository:

```bash
cmake -S src -B /tmp/baka-project-context-z90b9935/build -DBUILD_TESTING=ON
cmake --build /tmp/baka-project-context-z90b9935/build --parallel 2
python3 -B /tmp/baka-project-context-z90b9935/run_readonly_tests.py
ctest --test-dir /tmp/baka-project-context-z90b9935/build --output-on-failure
python3 -B semantic_audit/teammate_a/run.py --compiler /tmp/baka-project-context-z90b9935/build/baka
```

```python
from pathlib import Path
import contextlib,json
repo=Path('/home/saturn/CLionProjects/bakabaka')
out=Path('/tmp/baka-project-context-z90b9935')
results={}
ns={'__file__':str(repo/'custom_test_hook'),'__name__':'recon_hook'}
exec(compile((repo/'custom_test_hook').read_text(),ns['__file__'],'exec'),ns)
ns['COMPILER']=out/'build/baka'
with (out/'custom_tests.log').open('w') as log,contextlib.redirect_stdout(log):
 for name in ('lex','parse','codegen'):
  directory,flags=ns['SUITES'][name]
  tests=sorted(directory.glob('*.cpp'))
  status=ns['verify'](tests,flags)
  results[name]={'checks':len(tests),'exit':status}
 sem={'__file__':str(repo/'custom_tests/semantic/run.py'),'__name__':'recon_semantic'}
 exec(compile(Path(sem['__file__']).read_text(),sem['__file__'],'exec'),sem)
 sem['ROOT']=out
 results['semantic']={'exit':sem['main']()}
text=(out/'custom_tests.log').read_text()
results['total_pass']=sum(line.startswith('PASS ') for line in text.splitlines())
results['total_fail']=sum(line.startswith('FAIL ') for line in text.splitlines())
results['semantic_summary']=[line for line in text.splitlines() if 'checks;' in line]
(out/'test_results.json').write_text(json.dumps(results,indent=2))
print(json.dumps(results,indent=2))
raise SystemExit(bool(results['total_fail']))
```

The in-memory rebinding above uses the unmodified repository test implementations against the temporary binary. `HERE` still points to real fixtures; only semantic `ROOT` is rebound because that runner uses it for build paths. No `.expected` files were blessed.

| Verification | Observed result |
| --- | --- |
| Fresh CMake configure/build | exit 0; baka and three structural test executables built |
| Custom lexer snapshots | 10/10 passed |
| Custom parser snapshots | 57/57 passed |
| Semantic runner | 205/205 passed, including 155 source cases, 32 diagnostic cases, 18 structural scenarios |
| Entire custom harness | **272 passed, 0 failed** |
| CTest | **18/18 passed**; overlaps structural checks already counted above |
| Legacy semantic/AST services + parser runner | **17/17 passed**; two strict C++ service tests and 15 parser checks |
| Custom codegen suite | **0 cases**, not backend verification |
| Book execution/backend corpus | not run; no backend exists; active_tests currently selects no chapters |

The legacy runner explicitly excludes `valid_aggregate_shadow.cpp`. Its printed claim that the dispatcher does not route local aggregates is stale: probes below show parsing succeeds, but semantic registration/resolution rejects them.

```json
{
  "lex": {
    "checks": 10,
    "exit": 0
  },
  "parse": {
    "checks": 57,
    "exit": 0
  },
  "codegen": {
    "checks": 0,
    "exit": 0
  },
  "semantic": {
    "exit": 0
  },
  "total_pass": 272,
  "total_fail": 0,
  "semantic_summary": [
    "205 checks; 0 failures"
  ]
}
```

```text
Test project /tmp/baka-project-context-z90b9935/build
      Start  1: semantic_member_calls
 1/18 Test  #1: semantic_member_calls ..............   Passed    0.01 sec
      Start  2: semantic_conversions_numeric
 2/18 Test  #2: semantic_conversions_numeric .......   Passed    0.00 sec
      Start  3: semantic_conversions_contexts
 3/18 Test  #3: semantic_conversions_contexts ......   Passed    0.00 sec
      Start  4: semantic_conversions_calls
 4/18 Test  #4: semantic_conversions_calls .........   Passed    0.00 sec
      Start  5: semantic_conversions_compound
 5/18 Test  #5: semantic_conversions_compound ......   Passed    0.00 sec
      Start  6: semantic_conversions_pointers
 6/18 Test  #6: semantic_conversions_pointers ......   Passed    0.01 sec
      Start  7: semantic_conversions_metadata
 7/18 Test  #7: semantic_conversions_metadata ......   Passed    0.00 sec
      Start  8: semantic_registration
 8/18 Test  #8: semantic_registration ..............   Passed    0.00 sec
      Start  9: semantic_registration_error
 9/18 Test  #9: semantic_registration_error ........   Passed    0.00 sec
      Start 10: semantic_resolution
10/18 Test #10: semantic_resolution ................   Passed    0.01 sec
      Start 11: semantic_resolution_error
11/18 Test #11: semantic_resolution_error ..........   Passed    0.00 sec
      Start 12: semantic_annotation
12/18 Test #12: semantic_annotation ................   Passed    0.01 sec
      Start 13: semantic_annotation_error
13/18 Test #13: semantic_annotation_error ..........   Passed    0.00 sec
      Start 14: semantic_checks
14/18 Test #14: semantic_checks ....................   Passed    0.00 sec
      Start 15: semantic_checks_error
15/18 Test #15: semantic_checks_error ..............   Passed    0.00 sec
      Start 16: semantic_resolution_loop_targets
16/18 Test #16: semantic_resolution_loop_targets ...   Passed    0.01 sec
      Start 17: semantic_resolution_this
17/18 Test #17: semantic_resolution_this ...........   Passed    0.00 sec
      Start 18: semantic_annotation_scoped_type
18/18 Test #18: semantic_annotation_scoped_type ....   Passed    0.00 sec

100% tests passed out of 18

Total Test time (real) =   0.10 sec
```

```text
Teammate A semantic services: all checks passed
Teammate A AST services: all checks passed
15 parser regression checks passed
Known C integration gap (excluded from passing checks): valid_aggregate_shadow.cpp
{"checks": 17, "passed": 17}
```

## 2. Source language

### 2.1 Identity and examples

This is a project-specific **C/C++-like subset named baka**, not a complete implementation of either language. Classes, typedef-based type names, default arguments, overload selection and `this` coexist with C-style declarations and custom loop suffix labels. No source standard/version or executable ABI is selected in code; C++23 is the compiler's implementation language, not a claim about the compiled language.

Representative accepted semantic fixtures, copied verbatim:

#### `custom_tests/semantic/cases/valid_scalar.cpp`

```cpp
int main(){int x=1; return x+2;}
```

#### `custom_tests/semantic/cases/valid_function_pointer.cpp`

```cpp
int f(int x){return x;} int main(){int (*p)(int x)=f; return p(1);}
```

#### `custom_tests/semantic/cases/valid_loop_reuse.cpp`

```cpp
int main(){while(1;L){break L;}do{continue L;}while(0;L);return 0;}
```

#### `custom_tests/semantic/cases/valid_bound_instance_method.cpp`

```cpp
class A {
    int b;
    int c;
    int balls();
};
int main(int a) {
    A d;
    d.balls();
}
```

#### `custom_tests/semantic/cases/valid_inheritance.cpp`

```cpp
class A{int x;};class B:A{int y;};
```

### 2.2 Implemented feature inventory

| Feature | Verified implementation / limit |
| --- | --- |
| Scalar types | void, bool, char, short, int, long, float, double; unsigned char/short/int/long; fixed LP64 layout |
| Declarations | static/const, typedefs, named parameters, pointer/array/function declarators, prototypes and definitions |
| Functions | recursion, top-level forward calls, overloads, variadics, trailing defaults; caller defaults remain unexpanded AST references |
| Entry point | `main` is an ordinary function; no special signature or mandatory-main check; verified `double main(float x)` and a file with only `f` |
| Statements | compounds, expression/empty, if/else-if/else, switch/case/default, for/while/do, goto/return/break/continue |
| Custom loop labels | `for(init;cond;update;L)`, `while(cond;L)`, `do ... while(cond;L);`, `break L;`, `continue L;`; duplicate active labels rejected, sibling reuse allowed |
| Expressions | unary/binary/assignment/compound assignment, casts, ternary, comma, calls, indexing, dot/arrow, sizeof, postfix/prefix increments |
| Arrays/pointers | multidimensional and function-pointer declarators; positive integral constant bounds; selected omitted-bound inference; no VLA representation |
| Aggregates | named structs/classes, unions, enums, explicit enum initializers, fields/methods, one base type, constructors/destructors retained |
| Instance calls | bound receiver/member preserved, no ordinary FunctionToPointer decay; static/free functions may decay; pointer fields load their pointer |
| Strings | decoded literal byte strings in SLString/string-view nodes; string-to-char-array initialization; no adjacent literal concatenation (probe rejected) |
| Initialization | `= expr`, `= { ... }`, nested braces, scalar braces, selected brace elision/inferred array size; no normalized execution plan |
| Library names | only printf/scanf installed automatically; user declarations can describe other externals; no headers/preprocessor or FILE layout |
| Global/local static init | constant-only policy; dynamic `f()` initializer rejected |
| Missing returns/uninitialized reads | accepted by the observed checker; no all-path return, uninitialized-use or unreachable-code analysis |
| Nested/local type/function definitions | parser can retain them in compounds; semantic resolver rejects them rather than registering them |
| Unsupported | preprocessor, namespaces/`::`, access labels (all fields public by policy), signed spelling, long long declarations, references, const-pointer declarators, unnamed parameters including `(void)`, multiple inheritance, enum class, new/delete expressions, lambdas/closures, templates, exceptions and pointer-to-member types |

Keyword/token recognition alone does not establish parser or semantic support: new/delete/namespace/using are recognized lexically, for example. Root README's file-manipulation claim is broader than the installed-library implementation. C99 reference syntax, book tests and lexer-only tests likewise include unsupported constructs.

### 2.3 Grammar provenance and inferred operational grammar

A grammar file **does exist**: `src/parser/ReferenceGrammerANSIC2011`. Its own heading says “Browsable C99 Grammar”, cites the 2007 C99 TC3 draft, and carries CC-BY attribution. It is reproduced **verbatim in Appendix E**. It describes neither this C++-like subset nor its loop-label extensions. No normative, complete baka grammar or generated parser specification was found.

The following EBNF is **inferred from the successful branches of the actual parser**, not a repository-authored formal specification. Error recovery and type-name-dependent backtracking remain operational; Appendix C reproduces the parser implementation so details are not guessed. `type-name-token` means a builtin or identifier recognized by the parser's scoped type table.

```ebnf
program          ::= external* EOF
external         ::= aggregate-forward | aggregate-definition | enum-definition
                   | typedef-declaration | function-definition | declaration
storage-prefix   ::= ("static" | "const")*
modifier         ::= "unsigned" | "struct" | "class" | "enum" | "union"
type-specifier   ::= storage-prefix modifier? type-name-token
parameter-type   ::= "const"? modifier? type-name-token
function-definition ::= type-specifier declarator compound
                     # TryParseFunction probes and rolls back when there is no '{'
declaration      ::= type-specifier single-declaration ("," single-declaration)* ";"
single-declaration ::= declarator ("=" initializer)?
declarator       ::= "*"* (identifier | "(" declarator ")") function-suffix? array-suffix*
function-suffix  ::= "(" parameters? ")"
array-suffix     ::= "[" assignment-expression? "]"
parameters       ::= parameter ("," parameter)* ("," "...")? | "..."
parameter        ::= parameter-type single-declaration
                     # names are required; defaults must be trailing
abstract-declarator ::= "*"* ("(" abstract-declarator ")")? function-suffix? array-suffix*
                     # see declaration.cc for grouping probes and stopping rules
type-name        ::= "const"? modifier? type-name-token
initializer      ::= assignment-expression
                   | "{" (initializer ("," initializer)* ","?)? "}"
typedef-declaration ::= "typedef" "const"? modifier? type-name-token declarator ("," declarator)* ";"
aggregate-forward ::= ("struct" | "class" | "union") identifier ";"
aggregate-definition ::= ("struct" | "class") identifier (":" identifier)? member-body ";"
                       | "union" identifier? member-body ";"
member-body      ::= "{" member* "}"
member           ::= declaration | function-definition | typedef-declaration
                   | constructor-definition | destructor-definition
constructor-definition ::= owner-name "(" parameters? ")" compound
destructor-definition  ::= "~" owner-name "(" ")" (compound | ";")
                         # delimiter checks/fallbacks are in destructor.cc
enum-definition  ::= "enum" identifier? "{" enumerator ("," enumerator)* ","? "}" ";"
enumerator       ::= identifier ("=" assignment-expression)?
compound         ::= "{" block-item* "}"
block-item       ::= statement | declaration | typedef-declaration
                   | function-definition | aggregate-forward | aggregate-definition | enum-definition
                     # parser acceptance here exceeds semantic support
statement        ::= compound | ";" | expression ";" | selection | iteration | jump | label
selection        ::= "if" "(" expression ")" statement ("else" "if" "(" expression ")" statement)* ("else" statement)?
                   | "switch" "(" expression ")" statement
iteration        ::= "for" "(" for-init expression? ";" expression? loop-suffix? ")" statement
                   | "while" "(" expression loop-suffix? ")" statement
                   | "do" statement "while" "(" expression loop-suffix? ")" ";"
for-init         ::= ";" | expression ";" | declaration | typedef-declaration
loop-suffix      ::= ";" identifier
jump             ::= "return" expression? ";" | "goto" identifier ";"
                   | "break" identifier? ";" | "continue" identifier? ";"
label            ::= identifier ":" statement? | "case" expression ":" statement? | "default" ":" statement?
                     # statement omitted only before another case/default, '}', or EOF
expression       ::= assignment-expression ("," assignment-expression)*
assignment-expression ::= precedence-climbing-expression
primary          ::= literal | identifier | "this" | "(" expression ")"
postfix          ::= primary ("[" expression "]" | "(" expression? ")" | "." identifier | "->" identifier | "++" | "--")*
factor           ::= postfix | unary-op factor | "(" type-name abstract-declarator ")" factor | sizeof-expression
unary-op         ::= "+" | "-" | "~" | "!" | "++" | "--" | "*" | "&"
sizeof-expression ::= "sizeof" factor | "sizeof" "(" (type-name abstract-declarator | expression) ")"
```

Important inferred-grammar qualifications:

- No arbitrary suffix alternation is promised: named declarators parse an optional function suffix **before** the array suffix loop; parentheses represent deeper binding.
- `for(;;;a)` is an empty init, empty condition, empty update, then suffix label `a`. While/do conditions are not optional.
- Class/struct definitions require names; union/enum names may be absent. Enum forward declarations are not implemented. Direct declarations following an aggregate definition are not specified by the parser's aggregate-definition branch.
- Constructor/destructor grammar is intentionally read from the source rather than assumed from C++; a bare constructor prototype is not established by the grammar above. Unsupported/recovery spellings must not become lowering inputs.
- Exact precedence/associativity is in `operators.hh` and `precedence_climbing.hh` (Appendix A). In particular, conditional `?` and assignments both have table precedence 1; comma is parsed separately. Do not substitute a standard-language precedence table.
- `TryParseTypeName`, `TryParseFunction`, `AtDeclarationStart` and `AtAggregateDefinition` are context-sensitive probes; their actual source is included in Appendix C.

## 3. Lexer

### 3.1 Files and API

`src/lexer/lexer.hh`, `lexer.cc`, `keywords.hh`, `operators.hh`; token data/printers live under `src/types/token/`. The exact TokenType enum, Token payload, numeric-kind enum and TokenSourceLocation definitions are in Appendix B **without omissions**.

#### `src/lexer/lexer.hh`

```cpp
#pragma once
#include "types/token/all.hh"
#include <vector>

namespace baka {
    namespace lexer {
        struct Lexed {
            std::vector<types::Token> Tokens;
            std::vector<types::TokenSourceLocation> Locations;
        };

        Lexed Tokenize(std::string_view SourceCode);
    } // namespace lexer
} // namespace baka
```

### 3.2 Positions, ownership and diagnostics

- `Lexed::Tokens` and `Lexed::Locations` are parallel vectors. Token itself has no span field; location records give file path, **1-based line and column**, and no end offset/length.
- Identifier/keyword lexemes use `std::string_view` into the input. Numeric payloads are host integer/FP variant alternatives; char and string literals have dedicated payloads. `SLString` uses a singleton literal arena; its AST specialization retains a string view, including length.
- Source text must outlive all borrowed names/lexemes. FilePath views also depend on Gctx's source-path storage. Preserve mapped source and LineIndex through diagnostics/lowering; passing an ephemeral string to Tokenize is unsafe for later name use.
- Tokenize assumes Gctx already has a nonnull LineIndex (`GetLineIndex()->startLine(0)`). Driver attaches mapped source and index first; structural tests explicitly attach an index.
- Emits EOF_TOKEN, continues after some errors with UNKNOWN tokens, and accumulates diagnostics via Gctx. `LEX_ERROR`/`LEX_WARN` macros are real call sites, not exceptions returned by Tokenize.
- Character/column scanning operates on char/string_view bytes. Unicode/grapheme or multibyte semantic support is **unverified**.

Source: `src/lexer/lexer.cc` (verbatim excerpt).

```cpp
#define LEX_ERROR(STR, IDX) driver::Gctx::GenerateLineError(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)
#define LEX_WARN(STR, IDX) driver::Gctx::GenerateLineWarning(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)

#define EMIT_TOKEN(TT, VAL) \
    TokenLocations.emplace_back(driver::Gctx::GetSourceFilePath(), LineNo, ColNo); \
    Tokens.emplace_back((TT), (VAL))

#define EMIT_TOKEN_AT(TT, VAL, COL) \
    TokenLocations.emplace_back(driver::Gctx::GetSourceFilePath(), LineNo, (COL)); \
    Tokens.emplace_back((TT), (VAL))

```

## 4. Parser and semantic boundary

### 4.1 Parsing technique and API

Handwritten recursive descent for declarations/statements, precedence climbing for infix expressions, and explicit speculative rollback for type names/functions. Not generated; no visitor generator or parser-generator dependency. `Parser` consumes mutable spans of tokens and their parallel locations. Public interface and all other parser definitions/signatures are reproduced in Appendix C; the normal entry is:

```cpp
        Parser(std::span<types::Token> Tokens, std::span<types::TokenSourceLocation> TokenSourceLocations) : Tokens(Tokens), TokenSourceLocations(TokenSourceLocations) {};
        types::ASTNode* Parse();
```

`Parse()` returns a ProgramNode as ASTNode*. Prototypes are ordinary DeclarationList/SingleDeclarationNode declarators; FunctionNode means a function **with a parsed body**, and that declarator still requires semantic classification. Top-level ProgramNode stores heterogeneous ASTNode* entries.

### 4.2 AST shape and traversal

Appendix A is the authoritative full AST definition set. AST nodes are polymorphic, raw-pointer-linked and arena allocated; ASTNode has a virtual destructor and Print, not a dispatch visitor. Traversals use dynamic_cast, std::variant/std::visit and node-specific getters. `rewriteExpressions` only enumerates **direct expression slots**; it does not traverse declarations or statements on its own.

Real traversal excerpt:

Source: `src/semantic/resolution.cc` (verbatim excerpt).

```cpp
        if (auto* block = dynamic_cast<types::CompoundStatementNode*>(node)) {
            if (!outerBody) SymbolTable.enter(ScopeKind::Block);
            auto initializedCount = Initialized.size();
            for (const auto& item : block->getStatements()) std::visit([&](auto* child) {
                using T = std::remove_pointer_t<decltype(child)>;
                if constexpr (std::is_same_v<T, types::StatementNode>) resolveStatement(child);
                else if constexpr (std::is_same_v<T, types::DeclarationList>) registerLocal(child);
                else if constexpr (std::is_same_v<T, types::TypedefNode>) ASTResolver.aliases(*child);
                else InvalidNodes.push_back(child); // nested definitions deliberately unsupported
            }, item);
            Initialized.resize(initializedCount);
            if (!outerBody) SymbolTable.leave();
```

Lowering-critical AST conventions:

- Program/compound/member lists preserve source order. StructBodyNode::getMembers() includes fields, aliases, methods and every constructor/destructor; its category-specific vectors are supplementary.
- DeclarationIdentifierNode stores pointer count, a recursive inner declaration/name, array-size expression vector and optional ASTNode* parameter list; a null bound means `[]`. Use attached semantic TypeIds after checking rather than reconstructing binding from text.
- FunctionParameterList retains `isVariadic()`. Parameter object types can differ from canonical function signature types because arrays/functions adjust to pointers and top-level const is normalized for signatures.
- A transparent CommaExpressionNode commonly wraps even one expression. An outer call argument-list comma vector separates arguments; `f((a,b))` has an outer list with **one nested comma-expression argument**. Do not flatten it.
- InitializerNode exists for both braces and unbraced syntax. `isBraced()` is essential; transparent single-child wrappers are not brace boundaries.
- Labels own the statement they prefix; they are not separate flat “label followed by sibling statement” records. They can have null children at permitted boundaries.
- Empty statements are actual EmptyStatementNode; absent for clauses and bare return expressions are null pointers.
- `block.hh`'s Block and `labeled_st.hh`'s old label classes have no parser construction sites found; they are included verbatim for completeness, but active compounds/labels are the *Node classes in statement.hh/labelStatement.hh.

### 4.3 Parser errors and source locations

Parser ReportError stores Gctx diagnostics and increments ReportedErrorCount. It does not throw a parser-result object. Functions can return nullptr, optional failure, partially built nodes marked `hasError`, or resynchronize with SkipTo. Driver stops semantic analysis if parsing produced errors. Existing assert/throw guards also remain; their complete elimination is not established.

Source: `src/parser/parser.cc` (verbatim excerpt).

```cpp
        void Parser::ReportError(std::string Message, bool ReportPreviousToken) {
            ReportedErrorCount++;

            if (TokenSourceLocations.empty()) return;

            size_t idx = current;
            if(ReportPreviousToken) idx--;

            idx = std::clamp(idx, 0uz, TokenSourceLocations.size() - 1);


            const auto& Loc = TokenSourceLocations[idx];
            base::LineCtx LineCtx_v{Loc.LineNo - 1, driver::Gctx::GetLineIndex().get()};
            driver::Gctx::GenerateLineError(Loc.LineNo, Loc.Col, LineCtx_v, std::move(Message), driver::Stage::PARSE, ReportPreviousToken);
        }

```

Source: `src/parser/parser.hh` (verbatim excerpt).

```cpp
        // Composite nodes are usually allocated AFTER their children/body were
        // parsed. Callers must capture the introducing token before descending;
        // using current-1 here would point diagnostics at ')' or a closing brace.
        // Expression builders may instead pass their operator token explicitly.
        template <class T, class... Args>
        T* AllocNodeAt(size_t TokenIndex, Args&&... Arguments) {
            T* Node = ASTALLOC.Alloc<T>(std::forward<Args>(Arguments)...);
            if (!TokenSourceLocations.empty()) {
                auto Index = std::min(TokenIndex, TokenSourceLocations.size() - 1);
                Node->setSourceSpan(TokenSourceLocations[Index].LineNo, TokenSourceLocations[Index].Col);
            }
            return Node;
        }

```

ASTSourceSpan is a start-only `{Line, Col, Valid}` record, not an end-to-end range. Composite statement/declaration allocations capture their introducing token before descending. Binary/postfix builders use operator anchors. Semantic casts inherit operand/computation spans. Gctx requires LineIndex to turn these into displayed source lines.

### 4.4 Existing semantic analysis

Semantic analysis is implemented and connected **by default**. Exact interfaces/data structures are in Appendix D. Parser TypeLookup and semantic Symbols are **different** scoped tables; do not use the parser table for IR bindings.

| Pass | Current implementation and output |
| --- | --- |
| Declaration registration | Only program-root collection pass; signatures, nominal types, globals, member definitions, defaults; fills TopLevel/Bodies queues |
| Resolve/rename | Lexical body walk; persistent scope/symbol IDs; declaration-point bindings; owned unique names; contextual Expressions/Flows/Switches work |
| Type annotation | Intrinsic expression types/categories; selected calls/members; initialization rules; actual implicit-cast/compound AST rewriting |
| Read-only checks | Conditions/returns/defaults/storage/switch cases/control flow using queued facts; does not create types or modify AST |

Top-level registration errors stop the chain. Recoverable local resolution/type failures continue through independent work; poisoned expressions suppress cascades. A phase value of Checked alone does **not** prove success: require diagnostics().empty() and a successful analyze() result. Conversion materialization is skipped when annotation has errors. Diagnostics added during final checks still cause overall failure/no success AST.

Types/Symbols/Scope IDs are uint32_t wrappers with zero reserved for invalid. Derived types are interned structurally; aggregate/enum types are nominal and retain identity while completing. Aliases refer to the underlying type. Types owns a deque arena and an interning set referencing itself, so it is non-copyable; Symbols keeps persistent deque arenas for symbols/scopes. Parser scopes, unlike semantic scopes, are removed on exit.

Fixed sizes/alignments in Types::ComputeLayout (bytes): bool/char 1, short 2, int/uint/float/enum 4, long/ulong/double/pointers 8. char is signed in ArithmeticInfo; bool has 1 arithmetic bit but 1 storage byte. Arrays use element stride/alignment, unions overlap fields, single bases have baseOffset 0, non-storage members have null optional offsets. No backend target selector or ABI classifier exists.

Constants: IntegerResult carries ConstantState plus int64_t, and ExprResult/AST metadata have supported floating values. The evaluator is partial and must not be treated as a complete target-precision optimizer.

Source: `src/semantic/semantic_analyzer.hh` (verbatim excerpt).

```cpp
    public:
        SemanticAnalyzer();
        bool registerDeclarations(types::ProgramNode& program);
        bool resolveAndRename();
        bool annotateTypes();
        bool check();
        bool analyze(types::ProgramNode& program);
        SemanticPhase phase() const { return Phase; }
        const std::vector<Diagnostic>& diagnostics() const { return Diagnostics; }
        const Types& types() const { return TypeSystem; }
        const Symbols& symbols() const { return SymbolTable; }
        size_t topLevelWorkCount() const { return TopLevel.size(); }
        size_t bodyWorkCount() const { return Bodies.size(); }
        size_t expressionWorkCount() const { return Expressions.size(); }
    };
}
```

### 4.5 Exact IR input contract

- Keep ProgramNode, the **same live SemanticAnalyzer**, mapped source/LineIndex and relevant string arenas alive through lowering. `analyzer.types()`/`symbols()` expose const views.
- The driver currently destroys its local Analyzer at the end of the semantic-stage block. Append IR generation inside that lifetime or refactor ownership into a module context first. Parser AST arena lifetime alone does not preserve the type/symbol arenas.
- Original expression nodes retain intrinsic undecayed Type/LValue. ImplicitCastNode explicitly carries source/destination IDs and conversion kind; its result is an rvalue. Source CastFactorNode remains distinct.
- All ImplicitConversionKind variants must be handled: load, array/function decay, promotion/numeric/boolean/null/pointer/qualification conversion. A qualification conversion may be a no-op in TAC; derived-base pointer conversion may need address adjustment from type/layout info.
- Instance calls retain bound receiver/member identity; do not manufacture an ordinary function pointer. Free/static methods can decay. Function-pointer members remain indirect pointer loads; CallSignature holds the selected function type separately from callee's pointer type.
- MemberOwner and **one-based** MemberIndex identify the member: `types.get(owner).members[index-1]`; `layout(owner).offsets[index-1]` is optional. Inherited receivers can require walking base layout to reach this owner.
- Symbol identifies a selected direct callable when available; zero can mean an indirect call or constructor. Construction + owner/index distinguish construction; an implicit construction may have no selected constructor index/signature.
- Target is a control identity defined on loops/switches/goto labels and referenced by jumps. It is not a TypeId/SymbolId or a preexisting basic block address. Named jump identifiers mirror it; the printer displays it.
- CompoundAssignmentNode is semantic-only: evaluate Target's address **once**, bind that address for nested CompoundTargetNode reads, evaluate Value (including load/promotions/store conversion), then store. Its existing result category is an lvalue. Do not traverse a placeholder as another evaluation of the original target.
- Prefix/postfix mutation stays primitive; preserve one evaluation of the operand address and the correct old/new value result.
- Existing semantic work queues are private; only counts are public. A new pass can traverse the checked AST once or add a reviewed worklist/module interface; it cannot access BodyWork directly through the present public API.

## 5. Conventions and tests

### 5.1 Names/modules/errors/comments

- Headers use `.hh`, sources `.cc`, mostly `#pragma once`. Namespaces: baka::types, parser, lexer, semantic, driver, base. Classes generally PascalCase; many old members/local variables PascalCase, newer semantic methods lowerCamelCase. Style is mixed; do not claim a repository-wide enforced convention.
- Libraries are exposed by explicit CMake add_library/target_link_libraries and PUBLIC include directories. Semantic phases follow the parser's split-file structure, not one monolithic pass file.
- `driver.cc` textually includes `args.cc`; adding it as another compiled TU would duplicate definitions.
- Most diagnostics are accumulated as data; DriverError is thrown for CLI/file errors and runtime_error/invalid_argument/bad_alloc/assert guards exist in support services. src/main.cc only forwards to driver::run; no general top-level catch is present there.
- Semantic Diagnostic holds code/message and opaque const void* source/previous. Driver casts those back to ASTNode*. Any IR diagnostics reusing this interface must pass actual live AST pointers, not unrelated instruction pointers.
- Runtime diagnostic format is `file:line:column: error: message (STAGE)`, then a tab-prefixed source line and caret, with ANSI coloring. Previous-declaration notes are rendered as warnings. Driver sorts gathered diagnostics by source location and prints through stdout. See verbatim exceptions/Gctx/driver code in Appendix F.
- Comments include ordinary `//` implementation notes, informal TODOs and Doxygen-free explanatory blocks. Current semantic README/TODO/IR_READINESS are Markdown contracts; older terse comments can conflict with current implementation.

Source: `src/semantic/symbols.hh` (verbatim excerpt).

```cpp
        struct Diagnostic {
            std::string code, message;
            const void* source = nullptr;
            const void* previous = nullptr;
        };

```

Representative actual comment and namespace/API style:

Source: `src/semantic/semantic_analyzer.cc` (verbatim excerpt).

```cpp
    bool SemanticAnalyzer::analyze(types::ProgramNode& program) {
        // Invalid top-level signatures/types can invalidate later work entirely.
        // Once registration succeeds, local resolution/type errors are recoverable:
        // error-valued expressions suppress cascades, while independent work still
        // receives diagnostics (e.g. a bad call after a duplicate local).
        if (!registerDeclarations(program)) return false;
        resolveAndRename();
        annotateTypes();
        check();
        return Diagnostics.empty();
    }
}
```

### 5.2 Representative lexer and parser tests

Lexer tests intentionally contain constructs not supported by the parser. Golden snapshots are `.expected` siblings. The custom lexer/parser hook compares **stdout only**, not the exit status; semantic tests instead check expected exit/stage, and diagnostic cases also exact location/text. C++ structural tests use require/throw, not a third-party testing framework, and CTest launches scenarios as separate processes.

#### `custom_tests/lex/03_identifiers.cpp`

```cpp

namespace compiler_core_v2 {

class LexicalAnalyzerEngine {
private:
  int tokenCount;
  int _internal_state_flag;
  int max_lookahead_limit_99;

public:
  void initialize_pipeline() {
    tokenCount = 0;
    _internal_state_flag = 1;
    max_lookahead_limit_99 = 1024;
  }

  int get_token_count() { return tokenCount; }
};

const int MAX_BUFFER_SIZE_1024 = 1024;
const int DEFAULT_TIMEOUT_MS = 5000;
} // namespace compiler_core_v2

int process_data_stream_32(int input_stream_id, int batch_size_limit) {
  int local_accumulator_temp = 0;
  int _var1 = input_stream_id;
  int _2nd_var_with_digits = batch_size_limit;
  int a, b, c, x1, x2, y1, y2;

  local_accumulator_temp = _var1 + _2nd_var_with_digits;
  return local_accumulator_temp;
}

int main() {
  compiler_core_v2::LexicalAnalyzerEngine myEngineInstance;
  myEngineInstance.initialize_pipeline();
  int result = process_data_stream_32(1, 2);
  return result;
}
```

#### `custom_tests/parse/00_control_flow.cpp`

```cpp
int classify(int x) {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }
}

int count_until(int limit) {
    int sum = 0;
    for (int i = 0; i < limit; i = i + 1) {
        if (sum >= 100) {
            break;
        }
        sum = sum + i;
    }
    return sum;
}

int skip_evens(int limit) {
    int total = 0;
    int n = 0;
    while (n < limit) {
        n = n + 1;
        if (n % 2 == 0) {
            continue;
        }
        total = total + n;
    }
    return total;
}

int poll_until(int target) {
    int value = 0;
    do {
        value = value + 1;
    } while (value < target);
    return value;
}

int switch_demo(int key) {
    int result = 0;
    switch (key) {
    case 1:
        result = 1000;
        break;
    case 2:
        result = 2000;
    case 3:
        result = result + 5;
        break;
    default:
        result = -1;
    }
    return result;
}

int goto_demo(int x) {
    int y = 0;
    if (x == 0) {
        goto zero;
    }
    y = 1;
zero:
    return y;
}

int main() {
    int a = classify(10);
    int b = classify(-5);
    int c = classify(0);
    int d = count_until(50);
    int e = skip_evens(10);
    int f = poll_until(7);
    int g = switch_demo(2);
    int h = switch_demo(9);
    int i = goto_demo(0);
    return a + b + c + d + e + f + g + h + i;
}
```

Run just one golden test without writing a snapshot (same flags as hook):

```bash
python3 -B - <<'ONE'
from pathlib import Path
import subprocess
for stage, name in [('lex', '03_identifiers'), ('parse', '00_control_flow')]:
    source = Path(f'custom_tests/{stage}/{name}.cpp')
    result = subprocess.run(['./build/baka', '--verbose', '--build_type', 'debug',
                             '--' + stage, str(source)], capture_output=True, text=True)
    assert result.stdout == source.with_suffix('.expected').read_text()
    print('PASS', source, 'exit', result.returncode)
ONE
./build/semantic_pass_tests resolution_loop_targets
./build/semantic_conversion_tests contexts
./build/semantic_member_call_tests
ctest --test-dir build -R '^semantic_member_calls$' --output-on-failure
./build/baka --semantic custom_tests/semantic/cases/valid_bound_instance_method.cpp
```

Book harness commands exist, but backend execution is not verified:
`python3 -B test_compiler ./build/baka --chapter 1 --stage lex --latest-only`.
The harness also advertises validate/tacky/codegen/run, whereas this CLI implements only lex/parse/semantic/codegen, and codegen has no backend. In later chapters it requests assembly/link options and optimizations that this CLI does not implement. Do not count the whole tests/ tree as supported or passing.

### 5.3 Formatting/lint configuration

- Only repository clang-format configuration found: `src/semantic/.clang-format` (verbatim below). No repository `.clang-tidy` policy was found in inspected, non-private paths.
- `src/.clangd` points to `../build/` compile_commands.json and adds C++23 flags. CMake exports compile_commands.json.
- Read-only formatting check executed: `clang-format --dry-run --Werror src/semantic/semantic_analyzer.hh`; **exit 1**, existing formatting violations. No formatting was applied.
- Default `clang-tidy src/semantic/conversions.cc -p /tmp/baka-project-context-z90b9935/build --quiet` returned **exit 1: no checks enabled** in this environment. Explicit-check result is recorded in the verification addendum below. There is no verified project-wide lint-clean guarantee.
- To check without modifying: `clang-tidy src/semantic/conversions.cc -p build --checks='clang-analyzer-*' --quiet`. To format in future implementation work, `clang-format -i src/semantic/<file>` is mutating and was not used here.

#### `src/semantic/.clang-format`

```yaml
BasedOnStyle: LLVM
IndentWidth: 4
NamespaceIndentation: All
CompactNamespaces: false
BreakBeforeBraces: Attach
PointerAlignment: Left
ReferenceAlignment: Left
ColumnLimit: 120
AccessModifierOffset: -4
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AllowShortBlocksOnASingleLine: Never
InsertBraces: true
FixNamespaceComments: false
SortIncludes: Never
SeparateDefinitionBlocks: Always
```

#### `src/.clangd`

```yaml
CompileFlags:
  Add: ["-std=c++23", "-I."]
  CompilationDatabase: ../build/
```

```text
src/semantic/semantic_analyzer.hh:33:11: error: code should be clang-formatted [-Wclang-format-violations]
        };
          ^
src/semantic/semantic_analyzer.hh:35:99: error: code should be clang-formatted [-Wclang-format-violations]
        enum class ExpressionUse { Value, Condition, Initializer, Default, Return, Switch, Case };
                                                                                                  ^
src/semantic/semantic_analyzer.hh:45:11: error: code should be clang-formatted [-Wclang-format-violations]
        };
          ^
```

## 6. Existing IR/backend work and documentation

### 6.1 What exists

- **No compiler TAC instruction/value/basic-block/module type, TAC emitter, interpreter, VM, assembler emitter, register allocator or backend pass was found under src/.** Full source listing, CMake wiring and driver control flow were inspected.
- Stage::CODEGEN and `--codegen` are placeholders. A fresh probe `--codegen` on `int main(){return 0;}` returns 0 and prints the semantic AST; no codegen block or output-file writer follows semantic checking.
- `test_framework/tacky/` is Python **test logic**, mostly inspecting generated assembly and program behavior; it is not a source-language-to-TAC implementation. `test_framework/parser/asm.py` contains assembly model definitions used only by tests; Appendix G reproduces them verbatim to prevent confusion.
- `.jinja` templates are harness C/assembly fixtures. Their presence does not establish compiler code generation. No compiler TAC textual serialization contract was found.

#### `src/types/driver/defs.hh`

```cpp
#pragma once
#include <cassert>
#include <string_view>


namespace baka {
    namespace driver {
        enum class Stage {
            LEX = 0,
            PARSE = 1,
            SEMANTIC = 2,
            CODEGEN = 3,
        };

        constexpr std::string_view to_string_view(Stage stage) {
            switch (stage) {
            case Stage::LEX:
                return "LEX";
            case Stage::PARSE:
                return "PARSE";
            case Stage::SEMANTIC:
                return "SEMANTIC";
            case Stage::CODEGEN:
                return "CODEGEN";
            }

            assert(false && "add more cases");
        }


        enum class BuildType {
            DEBUG,
            RELEASE,
        };


        constexpr std::string_view to_string_view(BuildType stage) {
            switch (stage) {
            case BuildType::DEBUG:
                return "DEBUG";
            case BuildType::RELEASE:
                return "RELEASE";
            }

            assert(false && "add more cases");
        }
    }
}
```

Source: `src/driver/args.cc` (verbatim excerpt).

```cpp
                else if (curr == "--codegen") {
                    UptoStage = Stage::CODEGEN;
                }
```

Source: `src/driver/driver.cc` (verbatim excerpt).

```cpp
        // semantic checking
        if (Gctx::TillStage() < Stage::SEMANTIC) {
            return 0;
        }

        auto* Program = dynamic_cast<types::ProgramNode*>(Root);
        if (!Program) {
            Gctx::GenerateGlobalError("semantic stage requires a program AST", Stage::SEMANTIC);
        } else {
            semantic::SemanticAnalyzer Analyzer;
            bool Analyzed = Analyzer.analyze(*Program);
            if (Analyzed) {
                std::cout << "Semantic::AST: " << '\n';
                Program->Print();
                std::cout << std::endl;
            }
            if (!Analyzed && Analyzer.diagnostics().empty()) {
                Gctx::GenerateGlobalError("semantic analysis failed", Stage::SEMANTIC);
            }

            for (const auto& Entry : Analyzer.diagnostics()) {
                auto* Source = reinterpret_cast<const types::ASTNode*>(Entry.source);
                auto Span = Source ? Source->getSourceSpan() : types::ASTSourceSpan{};
                size_t LineNo = Span.Valid ? Span.Line : 1;
                size_t ColNo = Span.Valid ? Span.Col : 1;
                base::LineCtx Context{LineNo - 1, Gctx::GetLineIndex().get()};
                Gctx::GenerateLineError(LineNo, ColNo, Context, Entry.message, Stage::SEMANTIC);
                auto* Previous = reinterpret_cast<const types::ASTNode*>(Entry.previous);
                if (Previous) {
                    auto PreviousSpan = Previous->getSourceSpan();
                    if (PreviousSpan.Valid) {
                        base::LineCtx PreviousContext{PreviousSpan.Line - 1, Gctx::GetLineIndex().get()};
                        Gctx::GenerateLineWarning(PreviousSpan.Line, PreviousSpan.Col, PreviousContext,
                                                  "previous declaration is here", Stage::SEMANTIC);
                    }
                }
            }
        }
    }

    {
        // post semantic
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Semantic: Checking stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    return 0;
}
```

### 6.2 Documentation inventory and reliability

| File(s) | Summary / status |
| --- | --- |
| README.md | Project usage, examples, assumptions and stage checklist. Current commands are useful; feature claims such as file APIs exceed installed semantics. Full text in Appendix H. |
| context_A.md | Historical type/symbol/declaration handoff with interfaces and design choices. Claims header-only/no semantic driver are obsolete. Exact interface/design sections and the entire file are quoted in Appendix H, labeled historical. |
| context_B.md | Historical expression contracts/package handoff. “Never rewrites AST” and older ExprResult definition are obsolete; current headers/conversions are authoritative. Entire file quoted. |
| context_C.md | Historical integration/control-flow handoff. Default-PARSE and special-main-signature claims are obsolete; default is SEMANTIC and main is ordinary. Entire file quoted. |
| src/semantic/README.md | Current four-pass API, queues, ownership, conversions, diagnostic recovery and bound calls; quoted. Local-aggregate “parser gap” wording is stale: inspected parser accepts the syntax, resolver rejects it. |
| src/semantic/TODO.md | Aggregated architectural and behavioral acceptance items; unchecked does not mean no code exists. IR normalization/arena lifetime/lifetime planning remain open; quoted. |
| src/semantic/IR_READINESS.md | Current checked-AST contract and blocker list for full lowering; quoted in full. |
| semantic_audit/teammate_a/POLICY.md | Fixed LP64 and identity/layout/declaration contracts; quoted. Treat historical support claims against current implementation/probes. |
| semantic_audit/CHECKLIST.md | Historical comprehensive semantic audit work; useful requirements inventory, not current completion evidence. |
| semantic_audit/TASK_SPLIT.md | Historical three-person package/ownership/work allocation and interface agreements; quoted in full. |
| semantic_audit/RESULTS.md | Historical 113-program lexer/parser classification. No current TAC interface. |
| semantic_audit/extended/FINDINGS.md | Historical extra C-reference and loop-label findings, including bugs later repaired. No current TAC instruction definitions; selected design text quoted. |
| semantic_audit/extended/RESULTS.md | Historical 108-program lexer/parser/reference comparison; stale dropped-label findings must not be treated as current. |
| semantic_audit/TEAMMATE_A.md | Short historical integration handoff, now obsolete about missing driver; quoted. |
| src/lexer/definitions.txt | Informal token/feature wish list; mentions preprocessor even though unsupported. Quoted as planning notes. |
| todo | Historical two-line error-handling TODO; quoted. |
| tests/chapter_18/**/README.md | Three aggregate/union testing guides; contain test grouping/ABI expectations, not current compiler implementations. Full text quoted. |
| tests/chapter_19/**/README.md | Five optimization-testing guides; inspect generated assembly for expected transformations, not a TAC serialization definition. Full text quoted. |

Current source and fresh execution take precedence over all historical prose. No normative target TAC or complete source-language specification was discovered.

## 7. Risks and gaps before adding TAC passes

### 7.1 Verified probes (temporary input files; no repository edits)

Exit 0 means frontend acceptance, **not program execution or correct code generation**. The table lists probes added only in /tmp; exact source and errors follow.

| Probe | Stage | Exit | Observation |
| --- | --- | ---: | --- |
| local_aggregate | --parse | 0 | accepted; semantic AST False |
| local_aggregate | --semantic | 1 | unknown or hidden type name 'S' (SEMANTIC) |
| nested_function | --parse | 0 | accepted; semantic AST False |
| nested_function | --semantic | 1 | undeclared name 'inner' (SEMANTIC) |
| main_void_parameters | --semantic | 1 | expected identifier (PARSE) |
| long_long_declaration | --semantic | 1 | expected identifier (PARSE) |
| new_delete | --semantic | 1 | expected expression (PARSE) |
| array_address | --semantic | 1 | initializer is not convertible to the declared type (SEMANTIC) |
| pointer_to_array_control | --semantic | 0 | accepted; semantic AST True |
| static_dynamic_init | --semantic | 1 | static storage initializer must be a constant expression (SEMANTIC) |
| main_ordinary | --semantic | 0 | accepted; semantic AST True |
| namespace | --semantic | 1 | expected identifier or '(' (PARSE) |
| access_specifier | --semantic | 1 | unknown type name 'public' (PARSE) |
| adjacent_strings | --semantic | 1 | expected ',' or ';' in declaration (PARSE) |
| void_pointer_assignment | --semantic | 0 | accepted; semantic AST True |
| void_pointer_reverse | --semantic | 1 | initializer is not convertible to the declared type (SEMANTIC) |
| array_index_reversed | --semantic | 1 | subscript requires a pointer or array (SEMANTIC) |
| bound_method | --semantic | 0 | accepted; semantic AST True |
| missing_return | --semantic | 0 | accepted; semantic AST True |
| uninitialized_read | --semantic | 0 | accepted; semantic AST True |
| signed_keyword | --semantic | 1 | expected identifier or '(' (PARSE) |
| entry_without_main | --semantic | 0 | accepted; semantic AST True |
| codegen_no_backend | --codegen | 0 | accepted; semantic AST True |

Probe `local_aggregate` (verbatim temporary input):

```cpp
int main(){struct S{int x;};S s;return 0;}
```

Probe `array_address` (verbatim temporary input):

```cpp
int main(){int a[3];int (*p)[3]=&a;return 0;}
```

Probe `pointer_to_array_control` (verbatim temporary input):

```cpp
int main(){int (*p)[3];return sizeof(p);}
```

Probe `missing_return` (verbatim temporary input):

```cpp
int f(){}
```

Probe `uninitialized_read` (verbatim temporary input):

```cpp
int main(){int a;return a;}
```

Probe `codegen_no_backend` (verbatim temporary input):

```cpp
int main(){return 0;}
```

### 7.2 Concrete lowering blockers/quirks

1. **Arena/source lifetime:** driver Analyzer is scoped to semantic analysis; its IDs become unusable after that block. AST and borrowed name/string data have different owners. Do not pass a detached AST alone to lowering.
2. **Not all accepted constructs are execution-normalized:** defaults remain missing actual arguments; brace elision/string copies/zero filling/base/member initializer destinations are not explicit plans; constructors/temporary storage/copy/destruction and jump cleanup are not fully lowered. Existing validation is not an execution plan.
3. **Hidden receiver ABI:** bound calls carry member identity and explicit receiver AST when spelled. Implicit calls/field uses rely on current method context and synthetic `this` symbol. No normalized explicit receiver operand or pointer-to-member IR type exists.
4. **Control labels versus blocks:** Target IDs are sufficient identities, but a continue needs the correct loop update/condition block, break needs exit, and goto labels need addressable blocks. Case/default ownership/dispatch tables are not exported; collect cases while excluding nested switches and preserve source fallthrough/owned statements.
5. **Scope metadata is not a cleanup plan:** resolveStatement assigns a block's Scope before entering a new nested Block scope. Thus that AST record can denote the surrounding scope; declarations carry their own persistent scopes. A for node also begins in an enclosing scope before its For scope is entered. Do not blindly treat every node Scope as its body's own scope or a cleanup destination.
6. **Real address-of-array bug:** pointer-to-array declaration/control is accepted, but `int a[3]; int (*p)[3]=&a;` is rejected because checkUnary returns pointer-to-element for address-of an array. Do not silently reinterpret this as correct C/C++ typing in a backend; decide whether to repair frontend semantics first.
7. **Incomplete constant evaluation:** int64 carrier and partial floating folding do not establish target-correct unsigned wrap, shifts, overflow, all floating formats or all explicit conversions. Preserve source/destination signedness/width from types and avoid treating every cached ConstantExpression fact as a full optimizer oracle.
8. **No all-path return or initialization analysis:** missing-return and uninitialized-read probes both pass. There is no implicit-return/zero-initialization plan for ordinary function fallthrough. main is not special. An IR generator needs a defined terminator policy rather than inventing behavior.
9. **Parser-vs-semantic feature gaps:** local aggregates and nested function definitions can parse yet are rejected by the resolver. Legacy `Block`/old label classes are not active outputs. No closures/capture environment or nested-function lowering should be inferred.
10. **Raw/heterogeneous representation:** no exhaustive visitor, node-kind enum for all AST nodes, common child iterator or public semantic body-worklist. Prototype/function-pointer declarators share shape. Initializer containers and comma wrappers must be handled deliberately, not flattened.
11. **No backend contracts:** TAC value/instruction/type representation, textual output, temporary naming, function/method mangling, calling convention, aggregate-return classification, target platform and optimization requirements are unspecified. CODEGEN currently gives a misleading success exit without generating code.
12. **Front-end mutable/global state:** Gctx has process-global source/diagnostics/options with no compilation-reset API established. Parser token spans/source views and singleton arenas assume retained owners. Multi-file/module support, concurrent compilation, reset/reuse and serialization are **unverified**.
13. **Test limits:** lexer/parser golden tests compare only stdout; book C corpus exceeds parser grammar; no runtime/assembly equivalence tests exist for a generated backend. The positive frontend tests do not prove complete arithmetic/ABI/lifetime correctness.
14. **Dirty-tree dependency:** a checkout of HEAD lacks much of this implementation; preserve the untracked files and current modifications before another engineer works against it.

Source: `src/semantic/expressions.cc` (verbatim excerpt).

```cpp
                if (kind == TypeKind::Array) {
                    return makeValue(TypeSystem.pointer(TypeSystem.get(operand.type).element));
                }
```

Scope evidence (verbatim):

Source: `src/semantic/resolution.cc` (verbatim excerpt).

```cpp
    void SemanticAnalyzer::resolveStatement(types::StatementNode* node, bool outerBody) {
        if (!node) return;
        node->semanticInfo().Scope = SymbolTable.currentScope().value;
        node->semanticInfo().Resolved = true;
        if (auto* block = dynamic_cast<types::CompoundStatementNode*>(node)) {
            if (!outerBody) SymbolTable.enter(ScopeKind::Block);
```

### 7.3 Practical first-TAC milestone (recommendation, not existing implementation)

- Consume only successful checked AST + live semantic tables; give TAC its own module/functions/typed temporaries/address operands/labels.
- Start with scalar locals/parameters, literal values, explicit/implicit casts, unary/binary operations, load/store/address/index/member offset, returns, ordinary direct/indirect calls with all actual arguments supplied, branches, short circuit, ternary, loops and goto.
- Lower lvalues as addresses and rvalues as values. Preserve short-circuit/conditional control flow and single evaluation of mutation targets. Map bound instance calls to a callee identity plus receiver, rather than an ordinary function-pointer cast.
- Give all ImplicitConversionKind variants explicit treatment; retain target widths/signedness. Boolean conversion of a scalar is a truth test, not arbitrary truncation.
- Add default-argument expansion/initialization plans and lifetime/cleanup normalization before claiming full support for aggregates/defaults/nontrivial objects. Treat source-only sizeof operands as unevaluated.
- Do not add assembly/register/SSA metadata to semantic AST merely to start TAC; those are backend-owned. If typed addresses need types not interned by the frontend, choose a backend address-type representation instead of mutating const semantic tables accidentally.
- Add structural TAC tests for side effects, nested comma arguments, short circuit, for-continue update edges, switch fallthrough, function-pointer fields and derived-base addresses, then execution tests once an emitter/interpreter exists.

### 7.4 Questions the repository cannot answer

1. What exact typed TAC instruction set and textual/output format is required, and should `--tacky`/`--ir` become a new CLI stage?
2. Which target ABI/OS/calling convention and method/constructor/destructor mangling should TAC prepare for?
3. What evaluation order, signed-overflow/narrowing/FP semantics and non-void fallthrough behavior does baka promise? Parser/checker traversal order is not a language specification.
4. Should IR match current frontend quirks (notably `&array`) or should those be repaired before supporting those programs?
5. Is the first backend restricted to scalar/plain-data code, or must it immediately support defaults, aggregate parameters/returns, implicit construction, destructors and cleanup edges?
6. Should main/external entry requirements be introduced later, and at which target/link stage? The current frontend intentionally has no special main signature.
7. How should multi-file compilation, externally declared methods/static members, global initialization, symbols and library integration work?
8. Which book/backend tests are acceptance criteria, given that many require intentionally unsupported syntax or source-standard behavior?
9. May a future module/semantic result expose immutable worklists and own arenas/source buffers, or must lowering use the present Analyzer + ProgramNode interface unchanged?

These are explicitly unresolved design choices, not inferred answers.

# Appendix A — Every AST header, verbatim

Complete current files, including comments, methods, nested types, helpers and legacy nodes. No fields or constructors are replaced by ellipses. These are source definitions, not a proposed IR schema.

## `src/types/parser/ast/ast_node.hh`

```cpp
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>


namespace baka {
namespace types {

class ExpressionNode;
using ExpressionRewriter = std::function<ExpressionNode*(ExpressionNode*)>;

struct ASTSourceSpan {
    size_t Line = 0;
    size_t Col = 0;
    bool Valid = false;
};

// IDs belong to the SemanticAnalyzer that analyzed this tree. The parser leaves
// them zeroed. Declaration/type resolution fills bindings; annotation fills the
// expression fields. Checks and later lowering only read this record.
struct ASTSemanticInfo {
    uint32_t Type = 0, Symbol = 0, Scope = 0, Target = 0;
    bool Resolved = false, Annotated = false, Error = false, LValue = false;
    bool ConstantExpression = false, HasConstant = false, NullPointerConstant = false;
    int64_t Constant = 0;
    bool HasFloatingConstant = false;
    long double FloatingConstant = 0;
    // CallSignature is the selected function type, also for indirect calls and
    // constructors. MemberIndex is one-based (zero means no selected member).
    // layout(MemberOwner).offsets[MemberIndex-1] supplies a field's byte offset.
    uint32_t CallSignature = 0, MemberOwner = 0, MemberIndex = 0;
    bool Construction = false;
    bool operator==(const ASTSemanticInfo&) const = default;
};

class ASTNode {
    bool hasError_ = false;
    ASTSourceSpan Source;
    ASTSemanticInfo Semantic;
public:
    ASTNode() = default;
    virtual ~ASTNode() = default;

    virtual void Print(size_t Tabs = 0) const = 0;

    // Rewrites direct expression slots only. Semantic annotation supplies the
    // recursion and context; declaration/statement discovery is not repeated.
    virtual void rewriteExpressions(const ExpressionRewriter&) {}

    virtual void setHasError() {
        hasError_ = true;
    }

    bool hasError() const {
        return hasError_;
    }

    void setSourceSpan(size_t Line, size_t Col) {
        Source = {Line, Col, true};
    }

    const ASTSourceSpan& getSourceSpan() const {
        return Source;
    }

    ASTSemanticInfo& semanticInfo() { return Semantic; }
    const ASTSemanticInfo& semanticInfo() const { return Semantic; }

};
}
}
```

## `src/types/parser/ast/block.hh`

```cpp
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
```

## `src/types/parser/ast/castFactorNode.hh`

```cpp
#pragma once
#include "declaration.hh"
#include "expression.hh"
#include "utils.hh"

namespace baka
{
	namespace types
	{
		class CastFactorNode: public types::ExpressionNode {
			types::TypeNode* Type;
			types::DeclarationIdentifierNode* Declarator;
			ExpressionNode* CastExpr;
		public:
			CastFactorNode(types::TypeNode* Type, types::DeclarationIdentifierNode* Declarator, ExpressionNode* Expr)
				: Type(Type), Declarator(Declarator), CastExpr(Expr) {}

			types::TypeNode* getType() const { return Type; }
			types::DeclarationIdentifierNode* getDeclarator() const { return Declarator; }
			ExpressionNode* getExpression() const { return CastExpr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { CastExpr = rewrite(CastExpr); }

            void Print(size_t Tabs = 0) const override {
				INDENT(Tabs);
				std::cout << "CastFactor(" << std::endl;

				Type->Print(Tabs + 1);
				Declarator->Print(Tabs + 1);
				CastExpr->Print(Tabs + 1);

				INDENT(Tabs);
				std::cout << ")" << std::endl;
			}
		};
	}
}
```

## `src/types/parser/ast/class.hh`

```cpp
#pragma once
#include <iostream>
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/structBody.hh"
#include "utils.hh"
#include "ast_node.hh"

namespace baka {
    namespace types{

        class ClassDefinitionNode : public ASTNode {
            IdentifierNode* ClassName;
            IdentifierNode* ParentClassName = nullptr;
            StructBodyNode* Body;

        public:
            ClassDefinitionNode(IdentifierNode* identifier, StructBodyNode* body) :
            ClassName(identifier), Body(body) {
            }
            ClassDefinitionNode(IdentifierNode* identifier, IdentifierNode* parent, StructBodyNode* body) :
            ClassName(identifier), ParentClassName(parent), Body(body) {
            }

            ~ClassDefinitionNode() override = default;

            IdentifierNode* getName() const {
                return ClassName;
            }

            IdentifierNode* getBaseName() const {
                return ParentClassName;
            }

            StructBodyNode* getBody() const {
                return Body;
            }


            void Print(size_t Tabs) const override {

                INDENT(Tabs);
                std::cout << "Class(" << std::endl;
                ClassName->Print(Tabs + 1);
                if (ParentClassName) {
                    INDENT(Tabs+1);
                    std::cout << "ParentClass : " << std::endl;
                    ParentClassName->Print(Tabs+2);
                }
                Body->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class ClassDeclarationNode : public ASTNode
        {
            IdentifierNode* ClassName;
        public:
            ClassDeclarationNode(IdentifierNode* identifier) : ClassName(identifier) {
            }

            ~ClassDeclarationNode() override = default;

            IdentifierNode* getName() const {
                return ClassName;
            }


            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "ClassDeclaration(" << std::endl;
                ClassName->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/constant.hh`

```cpp
#pragma once
#include <cstddef>
#include <iostream>
#include <string_view>
#include "ast_node.hh"
#include "memory/custom_strings.hh"
#include "types/parser/ast/utils.hh"

namespace baka {

    namespace types {

        class ConstantNode : public ASTNode {
            public:
                virtual ~ConstantNode() = default;
        };

        // int
        // string - this is alloc
        // char
        // float
        // double
        // long long
        // unsigned int
        // unsigned long long
        // bool
        // nullptr

        // this guy creates copy for storing
        template<class T>
        class ConstantTNode : public ConstantNode {
            T Value;

            public:
                ConstantTNode(T val) : Value(val) {
                }

                const T& getValue() const { return Value; }
                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(" << Value << ")" << std::endl;
                }
        };

        // this guy views into arena
        template<>
        class ConstantTNode<base::SLString> : public ConstantNode {
            std::string_view Value;

            public:
                ConstantTNode(const base::SLString& val) : Value(val) {
                }

                std::string_view getValue() const { return Value; }

                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(" << Value << ")" << std::endl;
                }
        };

        template<>
        class ConstantTNode<std::nullptr_t> : public ConstantNode {
            public:
                std::nullptr_t getValue() const { return nullptr; }

                void Print(size_t Tabs = 0) const override {
                    INDENT(Tabs);
                    std::cout << "Constant(nullptr)" << std::endl;
                }
        };


    }
}
```

## `src/types/parser/ast/constructor.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include <iostream>

#include "statement.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
    namespace types {
        class ConstructorNode : public ASTNode {
            FunctionParameterList* Args;
            StatementNode* Body;

        public:
            ConstructorNode(FunctionParameterList* args, StatementNode* body) :
                Args(args), Body(body) {
            }


            FunctionParameterList* getArgs() const { return Args; }
            StatementNode* getBody() const { return Body; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Constructor(\n";

                Args->Print(Tabs + 1);
                Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/declaration.hh`

```cpp
#pragma once


#include <iostream>
#include <utility>
#include <vector>

#include "types/parser/ast/utils.hh"
#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"

namespace baka {
namespace types {


    class DeclarationIdentifierNode : public ASTNode
    {
        size_t PointerCount = 0;
        std::variant<DeclarationIdentifierNode*, IdentifierNode*> VariableName;
        std::vector<ExpressionNode*> ArraySizes;

        ASTNode* FunctionParameters = nullptr;

    public:
        DeclarationIdentifierNode() = default;

        DeclarationIdentifierNode(size_t PointerCount, DeclarationIdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(PointerCount), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        DeclarationIdentifierNode(IdentifierNode* VariableName, std::vector<ExpressionNode*> ArraySizes) :
        PointerCount(0), VariableName(VariableName), ArraySizes(std::move(ArraySizes)) {}

        void Print(size_t Tabs) const override
        {
            INDENT(Tabs);
            std::cout << "DeclarationIdentifier(" << std::endl;

            INDENT(Tabs + 1);
            std::cout << "PointerCount: " << PointerCount << std::endl;

            if(std::holds_alternative<DeclarationIdentifierNode*>(VariableName))
            {
                if (auto Node = std::get<DeclarationIdentifierNode*>(VariableName); Node != nullptr)
                {
                    Node->Print(Tabs + 2);
                }
                else
                {
                    INDENT(Tabs + 2);
                    std::cout << "Variable: nullptr" << std::endl;
                }
            }
            else
            {
                if (auto Node = std::get<IdentifierNode*>(VariableName); Node != nullptr)
                {
                    Node->Print(Tabs + 2);
                }
                else
                {
                    INDENT(Tabs + 2);
                    std::cout << "Variable: nullptr" << std::endl;
                }
            }

            INDENT(Tabs + 1);
            std::cout << "ArraySizes: " << std::endl;
            for (const auto& ArraySize : ArraySizes)
            {
                if (ArraySize) {
                    ArraySize->Print(Tabs + 2);
                } else {
                    INDENT(Tabs + 2);
                    std::cout << "nullptr" << std::endl;
                }
            }

            if(FunctionParameters)
            {
                INDENT(Tabs + 1);
                std::cout << "FunctionParameters: " << std::endl;
                FunctionParameters->Print(Tabs + 2);
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

        const std::variant<DeclarationIdentifierNode*, IdentifierNode*>& getInnerDeclaration() const
        {
            return VariableName;
        }

        const std::vector<ExpressionNode*>& getArraySizes() const
        {
            return ArraySizes;
        }

        ASTNode* getFunctionParameters() const
        {
            return FunctionParameters;
        }


        void appendPointer()
        {
            PointerCount++;
        }

        void appendArraySize(ExpressionNode* ArraySize)
        {
            ArraySizes.push_back(ArraySize);
        }

        void setInnerDeclaration(std::variant<DeclarationIdentifierNode*, IdentifierNode*> InnerDeclaration)
        {
            VariableName = InnerDeclaration;
        }

        void setFunctionParameters(ASTNode* Params)
        {
            this->FunctionParameters = Params;
        }

        size_t getPointerCount() const
        {
            return PointerCount;
        }

        // returns undelying variable name
        // eg: int (*x)(int)  should return 'x'
        std::string_view getVariableName() const
        {
            if (std::holds_alternative<IdentifierNode*>(VariableName)) {
                auto* Identifier = std::get<IdentifierNode*>(VariableName);
                return Identifier ? Identifier->GetName() : std::string_view{};
            }
            auto* InnerDeclaration = std::get<DeclarationIdentifierNode*>(VariableName);
            return InnerDeclaration ? InnerDeclaration->getVariableName() : std::string_view{};
        }

        IdentifierNode* getIdentifier() const
        {
            if (std::holds_alternative<IdentifierNode*>(VariableName)) {
                return std::get<IdentifierNode*>(VariableName);
            }
            auto* InnerDeclaration = std::get<DeclarationIdentifierNode*>(VariableName);
            return InnerDeclaration ? InnerDeclaration->getIdentifier() : nullptr;
        }

        // Returns true if this node or any nested DeclarationIdentifierNode
        // has at least one pointer level - used to validate function pointer
        // need for: int ((*fp))(int x) : we need to check if the innermost thingy has a * cuz it has to be a ptr
        bool hasPointerAtAnyLevel() const
        {
            if (PointerCount > 0) return true;
            if (std::holds_alternative<DeclarationIdentifierNode*>(VariableName))
                return std::get<DeclarationIdentifierNode*>(VariableName)->hasPointerAtAnyLevel();
            return false;
        }
    };

    class SingleDeclarationNode : public ASTNode {

        DeclarationIdentifierNode* VariableName;
        ExpressionNode* Initialization; // can be nullptr
    public:
        SingleDeclarationNode(DeclarationIdentifierNode* VariableName, ExpressionNode* Initialization) :
        VariableName(VariableName), Initialization(Initialization) {}

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Initialization = rewrite(Initialization); }

            void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "SingleDeclarationNode(" << "\n";

            VariableName->Print(Tabs + 1);
            if (Initialization) {
                Initialization->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "Initialization : nullptr\n";
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

        DeclarationIdentifierNode* getDeclarator() const {
            return VariableName;
        }

        ExpressionNode* getInitializer() const {
            return Initialization;
        }


        bool hasInitalizer() const {
            return Initialization != nullptr;
        }
    };

    class DeclarationList : public ASTNode {
        bool IsStatic = false, IsConst = false;
        TypeSpecifierModifier Modifier = TypeSpecifierModifier::NONE;

        IdentifierNode* TypeName{};
        std::vector<SingleDeclarationNode*> Declarations;

        public:
        DeclarationList(bool IsStatic, bool IsConst, TypeSpecifierModifier modifier, IdentifierNode* TypeName, std::vector<SingleDeclarationNode*> Declarations) :
        IsStatic(IsStatic), IsConst(IsConst), Modifier(modifier), TypeName(TypeName), Declarations(std::move(Declarations)) {

        }

        bool getIsStatic() const {
            return IsStatic;
        }

        bool getIsConst() const {
            return IsConst;
        }

        TypeSpecifierModifier getModifier() const {
            return Modifier;
        }

        IdentifierNode* getTypeName() const {
            return TypeName;
        }

        const std::vector<SingleDeclarationNode*>& getDeclarations() const {
            return Declarations;
        }


        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "DeclarationList(" << "\n";
            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << "\n";
            INDENT(Tabs + 1);
            std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";
            if (TypeName) {
                TypeName->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "nullptr\n";
            }

            for (const auto& Declaration : Declarations) {
                Declaration->Print(Tabs + 1);
            }
            INDENT(Tabs);
            std::cout << ")\n";
        }
    };


    class FunctionParameter : public ASTNode {
        bool IsConst = false;
        TypeSpecifierModifier Modifier;

        SingleDeclarationNode* SDeclarationNode; // can be nullptr cuz bare type: int balls(int,int)
        IdentifierNode* TypeName;

        public:
        FunctionParameter(bool IsConst, TypeSpecifierModifier modifier, IdentifierNode* TypeName, SingleDeclarationNode* SingleDeclarationNode) :
            IsConst(IsConst), Modifier(modifier), SDeclarationNode(SingleDeclarationNode), TypeName(TypeName) {

        }

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "FunctionParameter(" << "\n";
            INDENT(Tabs + 1);
            std::cout << "IsConst: " << IsConst << "\n";
            INDENT(Tabs + 1);
            std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";
            if (TypeName) {
                TypeName->Print(Tabs + 1);
            } else {
                INDENT(Tabs + 1);
                std::cout << "nullptr\n";
            }
            if (SDeclarationNode) {
                SDeclarationNode->Print(Tabs + 1);
            }
            INDENT(Tabs);
            std::cout << ")\n";
        }


        bool getIsConst() const {
            return IsConst;
        }

        TypeSpecifierModifier getModifier() const {
            return Modifier;
        }

        IdentifierNode* getTypeName() const {
            return TypeName;
        }

        SingleDeclarationNode* getDeclaration() const {
            return SDeclarationNode;
        }


        bool hasInitalizer() const {
            if(SDeclarationNode)
                return SDeclarationNode->hasInitalizer();
            return false;
        }
    };

    // TODO: use pimpl idiom

    class FunctionParameterList : public ASTNode {
        std::vector<FunctionParameter*> Parameters;
        bool Variadic = false;
    public:
        FunctionParameterList(std::vector<FunctionParameter*> parameters, bool variadic = false)
            : Parameters(std::move(parameters)), Variadic(variadic) {}
        const std::vector<FunctionParameter*>& getParameters() const {
            return Parameters;
        }

        bool isVariadic() const {
            return Variadic;
        }

        void setVariadic(bool value = true) {
            Variadic = value;
        }

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Args(" << '\n';

            for (const auto param : Parameters) {
                param->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };



}
}
```

## `src/types/parser/ast/destructor.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include <iostream>

#include "statement.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
    namespace types {
        class DestructorNode : public ASTNode {
            StatementNode* Body;

        public:
            DestructorNode(StatementNode* body) :
                 Body(body) {
            }

            ~DestructorNode() = default;

            StatementNode* getBody() const { return Body; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Destructor(\n";

                Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/enums.hh`

```cpp
#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
#include <vector>
#include "expression.hh"
namespace baka {
namespace types {

    class EnumNode : public ASTNode {
        IdentifierNode* EnumName; // cann be nullptr - anonymous enums
        std::vector<ExpressionNode*> Initializers;
        std::vector<IdentifierNode*> EnumValues; // TODO use lsit mayube?
    public:

        EnumNode(IdentifierNode* name)
            : EnumName(name) {}

        IdentifierNode* getEnumName() const {
            return EnumName;
        }

        const std::vector<IdentifierNode*>& getEnumValues() const {
            return EnumValues;
        }


        const std::vector<ExpressionNode*>& getInitializers() const {
            return Initializers;
        }


        void addEnumValue(IdentifierNode* value, ExpressionNode* initializer = nullptr) {
            EnumValues.push_back(value);
            Initializers.push_back(initializer);
        }

        bool isAnonymous() const { return EnumName == nullptr; }

        void Print(size_t Tabs = 0) const {
            INDENT(Tabs);
            std::cout << "Enum(" << (EnumName ? EnumName->GetName() : "") << ")" << std::endl;
            for (size_t i = 0; i < EnumValues.size(); ++i) {
                EnumValues[i]->Print(Tabs + 1);
                if (Initializers[i]) Initializers[i]->Print(Tabs + 2);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }

    };


}
}
```

## `src/types/parser/ast/expression.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include "constant.hh"
#include <variant>
#include "identifier.hh"
#include "types/parser/ast/this.hh"
#include "utils.hh"
#include "operators.hh"
#include <vector>
#include <iostream>


namespace baka {

namespace types {

    class ExpressionNode : public ASTNode {
    public:
        virtual ~ExpressionNode() = default;
    };

    class PostfixExpressionNode : public ExpressionNode {
    public:
        virtual ~PostfixExpressionNode() = default;
    };

    class PrimaryExpressionNode : public ExpressionNode {
        std::variant<ConstantNode*, IdentifierNode*, ThisNode*> Expr;

        public:

        PrimaryExpressionNode(std::variant<ConstantNode*, IdentifierNode*, ThisNode*> Expr) : Expr(Expr) {}

        const std::variant<ConstantNode*, IdentifierNode*, ThisNode*>& getExpression() const {
            return Expr;
        }

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "PrimaryExpression(" << std::endl;

            if(std::holds_alternative<ConstantNode*>(Expr)) {
                std::get<ConstantNode*>(Expr)->Print(Tabs + 1);
            } else if(std::holds_alternative<IdentifierNode*>(Expr)) {
                std::get<IdentifierNode*>(Expr)->Print(Tabs + 1);
            } else if(std::holds_alternative<ThisNode*>(Expr)) {
                std::get<ThisNode*>(Expr)->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };


    class FactorNode : public ExpressionNode {
        ASTUnaryOp UnaryOp;
        ExpressionNode* Expr;
        public:
            FactorNode(ASTUnaryOp UnaryOp, ExpressionNode* Expr)
                : UnaryOp(UnaryOp), Expr(Expr) {}


            ASTUnaryOp getUnaryOp() const { return UnaryOp; }
            ExpressionNode* getOperand() const { return Expr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Factor(" << std::endl;

                INDENT(Tabs + 1);
                std::cout << "UnaryOp: " << ASTUnaryOpToString(UnaryOp) << ",\n";

                if(Expr)
                    Expr->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Expr: nullptr" << std::endl;
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    class BinaryExpressionNode : public ExpressionNode {
        ASTBinaryOp BinaryOp;
        ExpressionNode* LHSExpr;
        ExpressionNode* RHSExpr;
        public:
            BinaryExpressionNode(ASTBinaryOp BinaryOp, ExpressionNode* LHSExpr, ExpressionNode* RHSExpr)
                : BinaryOp(BinaryOp), LHSExpr(LHSExpr), RHSExpr(RHSExpr) {}

            ASTBinaryOp getBinaryOp() const { return BinaryOp; }
            ExpressionNode* getLeft() const { return LHSExpr; }
            ExpressionNode* getRight() const { return RHSExpr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { LHSExpr = rewrite(LHSExpr); RHSExpr = rewrite(RHSExpr); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "BinaryExpression(" << std::endl;

                INDENT(Tabs + 1);
                std::cout << "BinaryOp: " << ASTBinaryOpToString(BinaryOp) << ",\n";

                if(LHSExpr)
                    LHSExpr->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "LHSExpr: nullptr" << std::endl;
                }

                if(RHSExpr)
                    RHSExpr->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "RHSExpr: nullptr" << std::endl;
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    class CommaExpressionNode : public ExpressionNode {
        std::vector<ExpressionNode*> ExpressionList;
        public:
            CommaExpressionNode(std::vector<ExpressionNode*> ExpressionList)
                : ExpressionList(std::move(ExpressionList)) {}

            const std::vector<ExpressionNode*>& getExpressions() const { return ExpressionList; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { for (auto*& item : ExpressionList) item = rewrite(item); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "CommaExpression(" << std::endl;

                for(auto* expr : ExpressionList) {
                    if(expr)
                        expr->Print(Tabs + 1);
                    else {
                        INDENT(Tabs + 1);
                        std::cout << "nullptr" << std::endl;
                    }
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // POSTFIX EXPRESSION NODES: . , ->, [] , (),


    class IndexPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        ExpressionNode* Index;
        public:
            IndexPostfixExpr(ExpressionNode* LHSExpr, ExpressionNode* Index)
                : LHSExpr(LHSExpr), Index(Index) {}

            ExpressionNode* getBase() const { return LHSExpr; }
            ExpressionNode* getIndex() const { return Index; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { LHSExpr = rewrite(LHSExpr); Index = rewrite(Index); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "IndexPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                Index->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    // for ++ and -- postfix:
    class UnaryPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        ASTUnaryOp Op;
        public:
            UnaryPostfixExpr(ExpressionNode* LHSExpr, ASTUnaryOp Op)
                : LHSExpr(LHSExpr), Op(Op) {}

            ExpressionNode* getOperand() const { return LHSExpr; }
            ASTUnaryOp getOp() const { return Op; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { LHSExpr = rewrite(LHSExpr); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "UnaryPostfixExpr(" << '\n';

                LHSExpr->Print(Tabs + 1);

                INDENT(Tabs + 1);
                std::cout << ASTUnaryOpToString(Op) << '\n';

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // for . postfix expr
    class MemberPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        IdentifierNode* MemberName;
        public:
            MemberPostfixExpr(ExpressionNode* LHSExpr, IdentifierNode* MemberName)
                : LHSExpr(LHSExpr), MemberName(MemberName) {}

            ExpressionNode* getBase() const { return LHSExpr; }
            IdentifierNode* getMember() const { return MemberName; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { LHSExpr = rewrite(LHSExpr); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "MemberPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                MemberName->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

    // for -> postfix expr
    class ArrowPostfixExpr : public PostfixExpressionNode {
        ExpressionNode* LHSExpr;
        IdentifierNode* MemberName;
        public:
            ArrowPostfixExpr(ExpressionNode* LHSExpr, IdentifierNode* MemberName)
                : LHSExpr(LHSExpr), MemberName(MemberName) {}

            ExpressionNode* getBase() const { return LHSExpr; }
            IdentifierNode* getMember() const { return MemberName; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { LHSExpr = rewrite(LHSExpr); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "ArrowPostfixExpr(" << std::endl;

                LHSExpr->Print(Tabs + 1);
                MemberName->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


    class FunctionCallPostfixExpr :  public PostfixExpressionNode {
        ExpressionNode* FunctionIdentifier;
        ExpressionNode* ArgumentList;

        public:
            FunctionCallPostfixExpr(ExpressionNode* FunctionIdentifier, ExpressionNode* ArgumentList)
                : FunctionIdentifier(FunctionIdentifier), ArgumentList(ArgumentList) {}

            ExpressionNode* getCallee() const { return FunctionIdentifier; }
            ExpressionNode* getArguments() const { return ArgumentList; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { FunctionIdentifier = rewrite(FunctionIdentifier); ArgumentList = rewrite(ArgumentList); }

        void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "FunctionCallPostfixExpr(" << std::endl;

                FunctionIdentifier->Print(Tabs + 1);
                ArgumentList->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

}
}
```

## `src/types/parser/ast/function.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include <iostream>
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"


namespace baka {
namespace types {

    class StatementNode;

    template <typename T>
    void PrintStatementBody(T* Body, size_t Tabs) {
        Body->Print(Tabs);
    }

    class FunctionNode : public ASTNode {
        // todo use DeclarationIdentifierNode
        IdentifierNode* BoundReturnType;

        DeclarationIdentifierNode* FuncType;

        StatementNode* Body;

        bool IsStatic = false, IsConst = false;
        TypeSpecifierModifier Modifier = TypeSpecifierModifier::NONE;

    public:
        FunctionNode(IdentifierNode* BindedReturnType, DeclarationIdentifierNode* funcType, StatementNode* body, bool IsStatic, bool IsConst, TypeSpecifierModifier modifier) :
            BoundReturnType(BindedReturnType), FuncType(funcType), Body(body), IsStatic(IsStatic), IsConst(IsConst), Modifier(modifier) {
        }

        ~FunctionNode() override = default;

        IdentifierNode* getReturnTypeName() const { return BoundReturnType; }
        DeclarationIdentifierNode* getDeclarator() const { return FuncType; }
        StatementNode* getBody() const { return Body; }
        bool getIsStatic() const { return IsStatic; }
        bool getIsConst() const { return IsConst; }
        TypeSpecifierModifier getModifier() const { return Modifier; }

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "Function(\n";

            INDENT(Tabs + 1);
            std::cout << "IsStatic: " << IsStatic << ", IsConst: " << IsConst << '\n';

            INDENT(Tabs+1);
            std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";

            INDENT(Tabs+1);
            std::cout << "FuncType: " << '\n';
            FuncType->Print(Tabs + 1);

            INDENT(Tabs+1);
            std::cout << "ReturnType: " << '\n';
            BoundReturnType->Print(Tabs + 1);

            INDENT(Tabs + 1);
            std::cout << "FunctionBody:" << '\n';
            PrintStatementBody(Body, Tabs + 1);

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };
}
}
```

## `src/types/parser/ast/identifier.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <string_view>
#include <string>
#include <vector>

namespace baka {

namespace types {

class IdentifierNode : public ASTNode {
    std::string_view IdentifierName;
    std::string ResolvedName;
    std::vector<uint32_t> Bindings;
public:
    IdentifierNode(std::string_view Name) : IdentifierName(Name) {}

    void Print(size_t Tabs = 0) const override {
        INDENT(Tabs);
        std::cout << "Identifier(" << IdentifierName << ")" << std::endl;
    }

    std::string_view GetName() const {
        return IdentifierName;
    }
    // Keep the source spelling for diagnostics and parser golden output. The
    // owned resolved name is the unique renamed identity used after resolution.
    const std::string& getResolvedName() const { return ResolvedName; }
    const std::vector<uint32_t>& getBindings() const { return Bindings; }
    void resolve(std::vector<uint32_t> bindings, std::string uniqueName) {
        Bindings = std::move(bindings);
        ResolvedName = std::move(uniqueName);
        semanticInfo().Resolved = true;
        semanticInfo().Symbol = Bindings.size() == 1 ? Bindings.front() : 0;
    }
};


}
}
```

## `src/types/parser/ast/implicitCast.hh`

```cpp
#pragma once
#include "expression.hh"

namespace baka::types {
    // Semantic-only nodes: the parser never constructs these. Type IDs use the
    // same arena as ASTSemanticInfo::Type; keep that arena alive while lowering.
    enum class ImplicitConversionKind {
        LValueToRValue, ArrayToPointer, FunctionToPointer,
        IntegralPromotion, NumericConversion, BooleanConversion,
        NullToPointer, PointerConversion, QualificationConversion
    };

    inline const char* ImplicitConversionName(ImplicitConversionKind kind) {
        switch (kind) {
        case ImplicitConversionKind::LValueToRValue: return "LValueToRValue";
        case ImplicitConversionKind::ArrayToPointer: return "ArrayToPointer";
        case ImplicitConversionKind::FunctionToPointer: return "FunctionToPointer";
        case ImplicitConversionKind::IntegralPromotion: return "IntegralPromotion";
        case ImplicitConversionKind::NumericConversion: return "NumericConversion";
        case ImplicitConversionKind::BooleanConversion: return "BooleanConversion";
        case ImplicitConversionKind::NullToPointer: return "NullToPointer";
        case ImplicitConversionKind::PointerConversion: return "PointerConversion";
        case ImplicitConversionKind::QualificationConversion: return "QualificationConversion";
        }
        return "Unknown";
    }

    class ImplicitCastNode : public ExpressionNode {
        ExpressionNode* Operand;
        uint32_t SourceType, DestinationType;
        ImplicitConversionKind Kind;
    public:
        ImplicitCastNode(ExpressionNode* operand, uint32_t source, uint32_t destination,
                         ImplicitConversionKind kind)
            : Operand(operand), SourceType(source), DestinationType(destination), Kind(kind) {}
        ExpressionNode* getOperand() const { return Operand; }
        uint32_t getSourceType() const { return SourceType; }
        uint32_t getDestinationType() const { return DestinationType; }
        ImplicitConversionKind getConversionKind() const { return Kind; }
            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Operand = rewrite(Operand); }
        void Print(size_t tabs = 0) const override {
            INDENT(tabs);
            std::cout << "ImplicitCast(" << ImplicitConversionName(Kind) << ", "
                      << SourceType << " -> " << DestinationType << ")\n";
            if (Operand) Operand->Print(tabs + 1);
        }
    };

    // A compound assignment evaluates Target's address exactly once. Its Value
    // subtree uses this placeholder to read that saved address, never to repeat
    // evaluation of Target (which may contain calls or index increments).
    class CompoundTargetNode : public ExpressionNode {
    public:
        void Print(size_t tabs = 0) const override {
            INDENT(tabs); std::cout << "CompoundTarget(saved address)\n";
        }
    };
    class CompoundAssignmentNode : public ExpressionNode {
        ExpressionNode* Target;
        ExpressionNode* Value;
    public:
        CompoundAssignmentNode(ExpressionNode* target, ExpressionNode* value) : Target(target), Value(value) {}
        ExpressionNode* getTarget() const { return Target; }
        ExpressionNode* getValue() const { return Value; }
            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Target = rewrite(Target); Value = rewrite(Value); }
        void Print(size_t tabs = 0) const override {
            INDENT(tabs); std::cout << "CompoundAssignment(\n";
            Target->Print(tabs + 1); Value->Print(tabs + 1);
            INDENT(tabs); std::cout << ")\n";
        }
    };
}
```

## `src/types/parser/ast/initializer.hh`

```cpp
#pragma once
#include "types/parser/ast/expression.hh"
#include <vector>

namespace baka {
namespace types {

    class InitializerNode : public ExpressionNode {
        std::vector<baka::types::ExpressionNode*> _expressions; // bunch of assignment exprs
        bool Braced = false;

    public:
        InitializerNode() = default;
        InitializerNode(std::vector<baka::types::ExpressionNode*> expressions) : _expressions(std::move(expressions)) {}
        explicit InitializerNode(bool braced) : Braced(braced) {}
        const std::vector<baka::types::ExpressionNode*>& expressions() const { return _expressions; }
        void addExpression(baka::types::ExpressionNode* expression) { _expressions.push_back(expression); }


        bool isBraced() const { return Braced; }
        void setBraced(bool value = true) { Braced = value; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { for (auto*& item : _expressions) item = rewrite(item); }

            void Print(size_t Tabs = 0) const override
        {
            INDENT(Tabs);
            std::cout << "InitializerNode(" << '\n';
            INDENT(Tabs + 1);
            std::cout << "Expressions: " << _expressions.size() << std::endl;
            for (const auto& Expression : _expressions)
            {
                Expression->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }

    };
}
}
```

## `src/types/parser/ast/iterationStatement.hh`

```cpp
#pragma once
#include "expression.hh"
#include "statement.hh"
#include "types/parser/ast/typedef.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace types {
        class IterationStatementNode : public StatementNode {
        public:
            virtual ~IterationStatementNode() = default;
        };

        class ForBlockStatementNode : public IterationStatementNode {
            //TODO Change to declaration?
            // we should probably not support typedefs here
            std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> Decl;
            types::ExpressionNode* Cond;
            types::ExpressionNode* Upd;
            types::IdentifierNode* Label;

            StatementNode* Body;

        public:
            ForBlockStatementNode(std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> decl, types::ExpressionNode* cond, types::ExpressionNode* upd, types::IdentifierNode* label, StatementNode* body) :
                Decl(decl), Cond(cond), Upd(upd), Label(label), Body(body) {
            }

            using InitVariant = std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*>;
            const InitVariant& getInitializer() const { return Decl; }
            types::ExpressionNode* getCondition() const { return Cond; }
            types::ExpressionNode* getUpdate() const { return Upd; }
            types::IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { if (auto* expr = std::get_if<ExpressionNode*>(&Decl)) *expr = rewrite(*expr); Cond = rewrite(Cond); Upd = rewrite(Upd); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "For(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                std::visit([&](auto&& decl) {
                    if(decl)
                        decl->Print(Tabs + 1);
                    else {
                        INDENT(Tabs + 1);
                        std::cout << "Decl: nullptr" << std::endl;
                    }
                }, Decl);

                if (Cond)
                    Cond->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }

                if (Upd)
                    Upd->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Upd: nullptr" << std::endl;
                }

                if (Label)
                    Label->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Label: nullptr" << std::endl;
                }

                if (Body)
                    Body->Print(Tabs + 1);
                else {
                    INDENT(Tabs + 1);
                    std::cout << "Body: nullptr" << std::endl;
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class WhileBlockStatementNode : public IterationStatementNode {
            ExpressionNode* Cond;
            IdentifierNode* Label;
            StatementNode* Body;

        public:
            WhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            ExpressionNode* getCondition() const { return Cond; }
            IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Cond = rewrite(Cond); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "While(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Cond) {
                    Cond->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }

                if (Label) {
                    INDENT(Tabs);
                    std::cout << ";\n";
                    Label->Print(Tabs + 1);
                }
                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class DoWhileBlockStatementNode : public IterationStatementNode {
            ExpressionNode* Cond;
            IdentifierNode* Label;
            StatementNode* Body;

        public:
            DoWhileBlockStatementNode(ExpressionNode* cond, IdentifierNode* label, StatementNode* body) : Cond(cond), Label(label), Body(body) {
            }

            ExpressionNode* getCondition() const { return Cond; }
            IdentifierNode* getLabel() const { return Label; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Cond = rewrite(Cond); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Do While(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Body) Body->Print(Tabs + 1);

                if (Cond) {
                    Cond->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "Cond: nullptr" << std::endl;
                }
                if (Label) {
                    INDENT(Tabs);
                    std::cout << ";\n";
                    Label->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/jump.hh`

```cpp
#pragma once
#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class JumpStatementNode : public StatementNode {
        public:
            virtual ~JumpStatementNode() = default;
        };

        class ReturnStatementNode : public JumpStatementNode {
            ExpressionNode* Expr;

        public:
            ReturnStatementNode(ExpressionNode* expr) : Expr(expr) {
            }

            ExpressionNode* getExpression() const { return Expr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Return(" << "\n";

                if (Expr) Expr->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class GotoStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            GotoStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class BreakStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            BreakStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Break(\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ContinueStatementNode : public JumpStatementNode {
            IdentifierNode* Label;

        public:
            ContinueStatementNode(IdentifierNode* label) : Label(label) {
            }

            IdentifierNode* getLabel() const { return Label; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Continue(\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Label) Label->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/labelStatement.hh`

```cpp
#pragma once
#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class LabelStatementNode : public StatementNode {
        public:
            virtual ~LabelStatementNode() = default;
        };

        class GotoLabelStatementNode : public LabelStatementNode {
            IdentifierNode* Tag;
            StatementNode* Stmt;

        public:
            GotoLabelStatementNode(IdentifierNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            IdentifierNode* getTag() const { return Tag; }
            StatementNode* getStatement() const { return Stmt; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Goto Label(";
                if (Tag) Tag->Print();
                std::cout << ":\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class CaseLabelStatementNode : public LabelStatementNode {
            //TODO check this
            ExpressionNode* Tag;
            StatementNode* Stmt;

        public:
            CaseLabelStatementNode(ExpressionNode* tag, StatementNode* stmt) : Tag(tag), Stmt(stmt) {
            }

            ExpressionNode* getTag() const { return Tag; }
            StatementNode* getStatement() const { return Stmt; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Tag = rewrite(Tag); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Case Label(";
                if (Tag) Tag->Print();
                std::cout << ":\n";

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class DefaultLabelStatementNode : public LabelStatementNode {
            StatementNode* Stmt;

        public:
            DefaultLabelStatementNode(StatementNode* stmt) : Stmt(stmt) {
            }

            StatementNode* getStatement() const { return Stmt; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Default Label(:\n";

                if (Stmt) Stmt->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/labeled_st.hh`

```cpp
#pragma once

#include "types/parser/ast/statement.hh"
namespace baka {
namespace types {

    class DefaultLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        DefaultLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~DefaultLabelStatement() = default;
    };


    class CaseLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        CaseLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~CaseLabelStatement() = default;
    };

    class GOTOLabelStatement : public ASTNode {
        StatementNode* Statement;
    public:
        GOTOLabelStatement(StatementNode* Statement) : Statement(Statement) {}
        virtual ~GOTOLabelStatement() = default;
    };

}
}
```

## `src/types/parser/ast/operators.hh`

```cpp
#pragma once
#include "types/token/token.hh"
#include "precedence_climbing.hh"
#include <string_view>
#include <unordered_map>


namespace baka {
    namespace types {
        enum class ASTUnaryOp {
            OP_UNARY_MINUS,
            OP_UNARY_PLUS,
            OP_TILDE,
            OP_LOGICAL_NOT,
            OP_INC,
            OP_DEC,
            OP_DEREF,
            OP_ADDR_OF,
            OP_SIZEOF,
            OP_NOOP
        };


        enum ASTBinaryOp {
            OP_MUL,
            OP_DIV,
            OP_MOD,
            OP_ADD,
            OP_SUB,
            OP_LSHIFT,
            OP_RSHIFT,
            OP_BIT_AND,
            OP_BIT_XOR,
            OP_BIT_OR,
            OP_LOGICAL_AND,
            OP_LOGICAL_OR,
            OP_EQ,
            OP_NEQ,
            OP_LT,
            OP_GT,
            OP_LE,
            OP_GE,
            OP_ASSIGN,
            OP_ADD_ASSIGN,
            OP_SUB_ASSIGN,
            OP_MUL_ASSIGN,
            OP_DIV_ASSIGN,
            OP_MOD_ASSIGN,
            OP_AND_ASSIGN,
            OP_OR_ASSIGN,
            OP_XOR_ASSIGN,
            OP_LSHIFT_ASSIGN,
            OP_RSHIFT_ASSIGN,
            OP_COMMA,
            OP_DOT,
            OP_ARROW
        };


        inline std::string_view ASTUnaryOpToString(ASTUnaryOp op) {
            switch (op) {
                case ASTUnaryOp::OP_UNARY_MINUS: return "OP_UNARY_MINUS";
                case ASTUnaryOp::OP_UNARY_PLUS: return "OP_UNARY_PLUS";
                case ASTUnaryOp::OP_TILDE: return "OP_TILDE";
                case ASTUnaryOp::OP_LOGICAL_NOT: return "OP_LOGICAL_NOT";
                case ASTUnaryOp::OP_INC: return "OP_INC";
                case ASTUnaryOp::OP_DEC: return "OP_DEC";
                case ASTUnaryOp::OP_DEREF: return "OP_DEREF";
                case ASTUnaryOp::OP_ADDR_OF: return "OP_ADDR_OF";
                case ASTUnaryOp::OP_SIZEOF: return "OP_SIZEOF";
                case ASTUnaryOp::OP_NOOP: return "OP_NOOP";
            }
            return "";
        }

        inline std::string_view ASTBinaryOpToString(ASTBinaryOp op) {
            switch (op) {
                case OP_MUL: return "OP_MUL";
                case OP_DIV: return "OP_DIV";
                case OP_MOD: return "OP_MOD";
                case OP_ADD: return "OP_ADD";
                case OP_SUB: return "OP_SUB";
                case OP_LSHIFT: return "OP_LSHIFT";
                case OP_RSHIFT: return "OP_RSHIFT";
                case OP_BIT_AND: return "OP_BIT_AND";
                case OP_BIT_XOR: return "OP_BIT_XOR";
                case OP_BIT_OR: return "OP_BIT_OR";
                case OP_LOGICAL_AND: return "OP_LOGICAL_AND";
                case OP_LOGICAL_OR: return "OP_LOGICAL_OR";
                case OP_EQ: return "OP_EQ";
                case OP_NEQ: return "OP_NEQ";
                case OP_LT: return "OP_LT";
                case OP_GT: return "OP_GT";
                case OP_LE: return "OP_LE";
                case OP_GE: return "OP_GE";
                case OP_ASSIGN: return "OP_ASSIGN";
                case OP_ADD_ASSIGN: return "OP_ADD_ASSIGN";
                case OP_SUB_ASSIGN: return "OP_SUB_ASSIGN";
                case OP_MUL_ASSIGN: return "OP_MUL_ASSIGN";
                case OP_DIV_ASSIGN: return "OP_DIV_ASSIGN";
                case OP_MOD_ASSIGN: return "OP_MOD_ASSIGN";
                case OP_AND_ASSIGN: return "OP_AND_ASSIGN";
                case OP_OR_ASSIGN: return "OP_OR_ASSIGN";
                case OP_XOR_ASSIGN: return "OP_XOR_ASSIGN";
                case OP_LSHIFT_ASSIGN: return "OP_LSHIFT_ASSIGN";
                case OP_RSHIFT_ASSIGN: return "OP_RSHIFT_ASSIGN";
                case OP_COMMA: return "OP_COMMA";
                case OP_DOT: return "OP_DOT";
                case OP_ARROW: return "OP_ARROW";
                default: return "UNKNOWN";
            }
        }


        inline static std::unordered_map<TokenType, ASTUnaryOp> TokenTypeToASTUnaryOp = {
            {TokenType::OP_UNARY_MINUS, ASTUnaryOp::OP_UNARY_MINUS},
            {TokenType::OP_PLUS, ASTUnaryOp::OP_UNARY_PLUS},
            {TokenType::OP_TILDE, ASTUnaryOp::OP_TILDE},
            {TokenType::OP_LOGICAL_NOT, ASTUnaryOp::OP_LOGICAL_NOT},
            {TokenType::OP_INC, ASTUnaryOp::OP_INC},
            {TokenType::OP_DEC, ASTUnaryOp::OP_DEC},
            {TokenType::OP_MUL, ASTUnaryOp::OP_DEREF},
            {TokenType::OP_BIT_AND, ASTUnaryOp::OP_ADDR_OF},
            {TokenType::K_SIZEOF, ASTUnaryOp::OP_SIZEOF},
        };


        inline bool IsBinaryOperator(TokenType type) {
            switch (type) {
                case TokenType::OP_MUL:
                case TokenType::OP_DIV:
                case TokenType::OP_MOD:
                case TokenType::OP_PLUS:
                case TokenType::OP_UNARY_MINUS: // '-' is binary subtraction too
                case TokenType::OP_BIT_AND:
                case TokenType::OP_BIT_OR:
                case TokenType::OP_BIT_XOR:
                case TokenType::OP_LSHIFT:
                case TokenType::OP_RSHIFT:
                case TokenType::OP_EQ:
                case TokenType::OP_NEQ:
                case TokenType::OP_LT:
                case TokenType::OP_GT:
                case TokenType::OP_LE:
                case TokenType::OP_GE:
                case TokenType::OP_LOGICAL_AND:
                case TokenType::OP_LOGICAL_OR:
                case TokenType::OP_ASSIGN:
                case TokenType::OP_ADD_ASSIGN:
                case TokenType::OP_SUB_ASSIGN:
                case TokenType::OP_MUL_ASSIGN:
                case TokenType::OP_DIV_ASSIGN:
                case TokenType::OP_MOD_ASSIGN:
                case TokenType::OP_AND_ASSIGN:
                case TokenType::OP_OR_ASSIGN:
                case TokenType::OP_XOR_ASSIGN:
                case TokenType::OP_LSHIFT_ASSIGN:
                case TokenType::OP_RSHIFT_ASSIGN:
                case TokenType::OP_COMMA:
                case TokenType::OP_DOT:
                case TokenType::OP_ARROW:
                case TokenType::OP_QUESTION:
                    return true;
                default:
                    return false;
            }
        }

        inline bool IsRightToLeft(TokenType type) {
            switch (type) {
                case TokenType::OP_QUESTION:
                case TokenType::OP_ASSIGN:
                case TokenType::OP_MUL_ASSIGN:
                case TokenType::OP_DIV_ASSIGN:
                case TokenType::OP_MOD_ASSIGN:
                case TokenType::OP_ADD_ASSIGN:
                case TokenType::OP_SUB_ASSIGN:
                case TokenType::OP_LSHIFT_ASSIGN:
                case TokenType::OP_RSHIFT_ASSIGN:
                case TokenType::OP_OR_ASSIGN:
                case TokenType::OP_XOR_ASSIGN:
                case TokenType::OP_AND_ASSIGN:
                    return true;
                default:
                    return false;
            }
        }


        inline static std::unordered_map<TokenType, ASTBinaryOp> TokenTypeToASTBinaryOp = {
            {TokenType::OP_MUL, OP_MUL},
            {TokenType::OP_DIV, OP_DIV},
            {TokenType::OP_MOD, OP_MOD},
            {TokenType::OP_PLUS, OP_ADD},
            {TokenType::OP_UNARY_MINUS, OP_SUB},
            {TokenType::OP_LSHIFT, OP_LSHIFT},
            {TokenType::OP_RSHIFT, OP_RSHIFT},
            {TokenType::OP_BIT_AND, OP_BIT_AND},
            {TokenType::OP_BIT_XOR, OP_BIT_XOR},
            {TokenType::OP_BIT_OR, OP_BIT_OR},
            {TokenType::OP_LOGICAL_AND, OP_LOGICAL_AND},
            {TokenType::OP_LOGICAL_OR, OP_LOGICAL_OR},
            {TokenType::OP_EQ, OP_EQ},
            {TokenType::OP_NEQ, OP_NEQ},
            {TokenType::OP_LT, OP_LT},
            {TokenType::OP_GT, OP_GT},
            {TokenType::OP_LE, OP_LE},
            {TokenType::OP_GE, OP_GE},
            {TokenType::OP_ASSIGN, OP_ASSIGN},
            {TokenType::OP_ADD_ASSIGN, OP_ADD_ASSIGN},
            {TokenType::OP_SUB_ASSIGN, OP_SUB_ASSIGN},
            {TokenType::OP_MUL_ASSIGN, OP_MUL_ASSIGN},
            {TokenType::OP_DIV_ASSIGN, OP_DIV_ASSIGN},
            {TokenType::OP_MOD_ASSIGN, OP_MOD_ASSIGN},
            {TokenType::OP_AND_ASSIGN, OP_AND_ASSIGN},
            {TokenType::OP_OR_ASSIGN, OP_OR_ASSIGN},
            {TokenType::OP_XOR_ASSIGN, OP_XOR_ASSIGN},
            {TokenType::OP_LSHIFT_ASSIGN, OP_LSHIFT_ASSIGN},
            {TokenType::OP_RSHIFT_ASSIGN, OP_RSHIFT_ASSIGN},
            {TokenType::OP_COMMA, OP_COMMA},
            {TokenType::OP_DOT, OP_DOT},
            {TokenType::OP_ARROW, OP_ARROW}
        };

        inline int GetPrecedence(TokenType type) {
            if (TokenTypeToPrecedence.find(type) != TokenTypeToPrecedence.end()) {
                return TokenTypeToPrecedence[type];
            }
            return -1;
        }
    }
}
```

## `src/types/parser/ast/precedence_climbing.hh`

```cpp
#pragma once
#include "types/token/token.hh"
#include <unordered_map>

namespace baka {
namespace types {

    inline static std::unordered_map<TokenType, int> TokenTypeToPrecedence = {
        {TokenType::OP_DOT, 15},
        {TokenType::OP_ARROW, 15},
        {TokenType::OP_MUL, 12},
        {TokenType::OP_DIV, 12},
        {TokenType::OP_MOD, 12},
        {TokenType::OP_PLUS, 11},
        {TokenType::OP_UNARY_MINUS, 11}, // binary subtraction
        {TokenType::OP_LSHIFT, 10},
        {TokenType::OP_RSHIFT, 10},
        {TokenType::OP_LT, 9},
        {TokenType::OP_GT, 9},
        {TokenType::OP_LE, 9},
        {TokenType::OP_GE, 9},
        {TokenType::OP_EQ, 8},
        {TokenType::OP_NEQ, 8},
        {TokenType::OP_BIT_AND, 7},
        {TokenType::OP_BIT_XOR, 6},
        {TokenType::OP_BIT_OR, 5},
        {TokenType::OP_LOGICAL_AND, 4},
        {TokenType::OP_LOGICAL_OR, 3},
        {TokenType::OP_QUESTION, 1}, // ternary
        {TokenType::OP_ASSIGN, 1},
        {TokenType::OP_ADD_ASSIGN, 1},
        {TokenType::OP_SUB_ASSIGN, 1},
        {TokenType::OP_MUL_ASSIGN, 1},
        {TokenType::OP_DIV_ASSIGN, 1},
        {TokenType::OP_MOD_ASSIGN, 1},
        {TokenType::OP_AND_ASSIGN, 1},
        {TokenType::OP_OR_ASSIGN, 1},
        {TokenType::OP_XOR_ASSIGN, 1},
        {TokenType::OP_LSHIFT_ASSIGN, 1},
        {TokenType::OP_RSHIFT_ASSIGN, 1},
        {TokenType::OP_COMMA, 0},
    };


}
}
```

## `src/types/parser/ast/program.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <vector>

namespace baka {
    namespace types {
        class ProgramNode : public ASTNode {
            std::vector<ASTNode*> Body;
            // function decl
            // declaration list
            // union decl
            // class decl
            // enum decl
            // typedef

        public:
            ProgramNode(std::vector<ASTNode*> body) : Body(std::move(body)) {
            }

            ProgramNode() = default;

            void addNode(ASTNode* node) {
                Body.push_back(node);
            }

            const std::vector<ASTNode*>& getBody() const {
                return Body;
            }

            ~ProgramNode() = default;

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Program(" << '\n';
                for (const auto tlBlock : Body) {
                    tlBlock->Print(Tabs + 1);
                }

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/selectionStatement.hh`

```cpp
#pragma once
#include <iostream>
#include <utility>
#include <vector>

#include "expression.hh"
#include "statement.hh"
#include "utils.hh"

namespace baka {
    namespace types {
        class SelectionStatementNode : public StatementNode {
        public:
            virtual ~SelectionStatementNode() = default;
        };

        class IfSubBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            IfSubBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            ExpressionNode* getCondition() const { return Expr; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "If(" << "\n";

                if (Expr) {
                    Expr->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "nullptr" << std::endl;
                }

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ElseIfSubBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            ElseIfSubBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            ExpressionNode* getCondition() const { return Expr; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Else If(" << "\n";

                if (Expr) {
                    Expr->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "nullptr" << std::endl;
                }

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class ElseSubBlockStatementNode : public SelectionStatementNode {
            StatementNode* Body;

        public:
            ElseSubBlockStatementNode(StatementNode* body) : Body(body) {
            }

            StatementNode* getBody() const { return Body; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Else(" << "\n";

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class IfSuperBlockStatementNode : public SelectionStatementNode {
            IfSubBlockStatementNode* IfSubBlock;
            std::vector<ElseIfSubBlockStatementNode*> ElseIfSubBlocks;
            ElseSubBlockStatementNode* ElseSubBlock;

        public:
            IfSuperBlockStatementNode(IfSubBlockStatementNode* ifSubBlock, std::vector<ElseIfSubBlockStatementNode*> elseIfSubBlocks,
                                      ElseSubBlockStatementNode* elseSubBlock) : IfSubBlock(ifSubBlock), ElseIfSubBlocks(std::move(elseIfSubBlocks)), ElseSubBlock(elseSubBlock) {
            }

            IfSubBlockStatementNode* getIf() const { return IfSubBlock; }
            const std::vector<ElseIfSubBlockStatementNode*>& getElseIfs() const { return ElseIfSubBlocks; }
            ElseSubBlockStatementNode* getElse() const { return ElseSubBlock; }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "If Super Block(" << "\n";

                IfSubBlock->Print(Tabs + 1);

                for (auto elseIfSubBlock : ElseIfSubBlocks) {
                    elseIfSubBlock->Print(Tabs + 1);
                }

                if (ElseSubBlock) ElseSubBlock->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };

        class SwitchBlockStatementNode : public SelectionStatementNode {
            ExpressionNode* Expr;
            StatementNode* Body;

        public:
            SwitchBlockStatementNode(ExpressionNode* expr, StatementNode* body) : Expr(expr), Body(body) {
            }

            ExpressionNode* getCondition() const { return Expr; }
            StatementNode* getBody() const { return Body; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expr = rewrite(Expr); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "Switch(" << "\n";
                PrintControlTarget(semanticInfo().Target, Tabs + 1);

                if (Expr) {
                    Expr->Print(Tabs + 1);
                } else {
                    INDENT(Tabs + 1);
                    std::cout << "nullptr" << std::endl;
                }

                if (Body) Body->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/sizeof.hh`

```cpp
#pragma once


#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typenode.hh"
#include <variant>

#include "declaration.hh"

namespace baka {
namespace types {

    class SizeofNode : public ExpressionNode {
    public:

        struct SizeofTypeVariant {
            TypeNode* Type;
            DeclarationIdentifierNode* AbstractDecl;
        };

        std::variant<SizeofTypeVariant,
                    ExpressionNode*
        > Payload;

        SizeofNode(std::variant<SizeofTypeVariant, ExpressionNode*> Payload) : Payload(Payload) {}

        const std::variant<SizeofTypeVariant, ExpressionNode*>& getPayload() const { return Payload; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { if (auto* expr = std::get_if<ExpressionNode*>(&Payload)) *expr = rewrite(*expr); }

            void Print(size_t Tabs = 0) const override
        {
            INDENT(Tabs);
            std::cout << "SizeofNode(\n";

            if(std::holds_alternative<SizeofTypeVariant>(Payload)) {
                const auto& Variant = std::get<SizeofTypeVariant>(Payload);
                if(Variant.Type) {
                    Variant.Type->Print(Tabs + 1);
                }
                if(Variant.AbstractDecl) {
                    Variant.AbstractDecl->Print(Tabs + 1);
                }
            } else {
                const auto& Expr = std::get<ExpressionNode*>(Payload);
                Expr->Print(Tabs + 1);
            }

            INDENT(Tabs);
            std::cout << ")\n";
        }

    };

}
}
```

## `src/types/parser/ast/statement.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/typedef.hh"
#include <variant>
#include <vector>


namespace baka {
namespace types {

    class StructDefinitionNode;
    class StructDeclarationNode;
    class ClassDefinitionNode;
    class ClassDeclarationNode;
    class UnionNode;
    class EnumNode;

    enum class StatementNodeType {
        CompoundStatement,
        JumpStatement,
        LabeledStatement,
        IterationStatement,
        SelectionStatement,
        ExpressionStatement,
    };

    class StatementNode : public ASTNode {
    public:
        virtual ~StatementNode() = default;
    };

    class CompoundStatementNode : public StatementNode
    {
    public:
        using StatementVariant = std::variant<StatementNode*, DeclarationList*, TypedefNode*, FunctionNode*,
                                              StructDefinitionNode*, StructDeclarationNode*, ClassDefinitionNode*,
                                              ClassDeclarationNode*, UnionNode*, EnumNode*>;
        using StatementList = std::vector<StatementVariant>;

    private:
        StatementList Statements;

    public:
        CompoundStatementNode(StatementList Statements) : Statements(std::move(Statements)) {}

        const StatementList& getStatements() const { return Statements; }

        void addStatement(StatementVariant Statement) { Statements.push_back(Statement); }

        void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "CompoundStatement(" << std::endl;

            for (const auto& Statement : Statements) {
                std::visit([&](auto* node) {
                    if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
                }, Statement);
            }

            INDENT(Tabs);
            std::cout << ")" << std::endl;
        }
    };

    class EmptyStatementNode : public StatementNode {
    public:
        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "EmptyStatement()" << std::endl;
        }
    };

    class ExpressionStatementNode : public StatementNode {
        ExpressionNode* Expression; // can be nullptr
        public:
            ExpressionStatementNode(ExpressionNode* Expression) : Expression(Expression) {}
            ExpressionNode* getExpression() const { return Expression; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Expression = rewrite(Expression); }

            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "ExpressionStatement(" << std::endl;

                if (Expression)
                    Expression->Print(Tabs + 1);

                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };


}
}
```

## `src/types/parser/ast/struct.hh`

```cpp
#pragma once
#include <iostream>
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/structBody.hh"
#include "utils.hh"
#include "ast_node.hh"

namespace baka {
    namespace types{

        class StructDefinitionNode : public ASTNode {
            IdentifierNode* StructName;
            IdentifierNode* ParentStructName = nullptr;
            StructBodyNode* Body;

        public:
            StructDefinitionNode(IdentifierNode* identifier, StructBodyNode* body) :
            StructName(identifier), Body(body) {
            }
            StructDefinitionNode(IdentifierNode* identifier, IdentifierNode* parent, StructBodyNode* body) :
            StructName(identifier), ParentStructName(parent), Body(body) {
            }

            ~StructDefinitionNode() override = default;

            IdentifierNode* getName() const {
                return StructName;
            }

            IdentifierNode* getBaseName() const {
                return ParentStructName;
            }

            StructBodyNode* getBody() const {
                return Body;
            }


            void Print(size_t Tabs) const override {

                INDENT(Tabs);
                std::cout << "Struct(" << std::endl;
                StructName->Print(Tabs + 1);
                if (ParentStructName) {
                    INDENT(Tabs+1);
                    std::cout << "ParentStruct : " << std::endl;
                    ParentStructName->Print(Tabs+2);
                }
                Body->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };


        class StructDeclarationNode : public ASTNode
        {
            IdentifierNode* StructName;
        public:
            StructDeclarationNode(IdentifierNode* identifier) : StructName(identifier) {
            }

            ~StructDeclarationNode() override = default;

            IdentifierNode* getName() const {
                return StructName;
            }


            void Print(size_t Tabs) const override {
                INDENT(Tabs);
                std::cout << "StructDeclaration(" << std::endl;
                StructName->Print(Tabs + 1);
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
        };
    }
}
```

## `src/types/parser/ast/structBody.hh`

```cpp
#pragma once


#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/typedef.hh"
namespace baka {
namespace types {
    class StructBodyNode : public ASTNode {
        std::vector<DeclarationList*> Declarations;
        std::vector<FunctionNode*> Functions; // member functions
        std::vector<TypedefNode*> Typedefs;

        std::vector<ConstructorNode*> Constructors;
        std::vector<DestructorNode*> Destructors;
        std::vector<ASTNode*> Members;

        public:
            StructBodyNode() = default;

            void AddDeclaration(DeclarationList* decl) {
                Declarations.push_back(decl);
                Members.push_back(decl);
            }

            void AddFunction(FunctionNode* func) {
                Functions.push_back(func);
                Members.push_back(func);
            }

            void AddTypedef(TypedefNode* typedefNode) {
                Typedefs.push_back(typedefNode);
                Members.push_back(typedefNode);
            }

            void SetConstructor(ConstructorNode* func) {
                Constructors.push_back(func);
                Members.push_back(func);
            }

            void SetDestructor(DestructorNode* func) {
                Destructors.push_back(func);
                Members.push_back(func);
            }

            const std::vector<ASTNode*>& getMembers() const {
                return Members;
            }

            const std::vector<DeclarationList*>& getDeclarations() const {
                return Declarations;
            }

            const std::vector<FunctionNode*>& getFunctions() const {
                return Functions;
            }

            const std::vector<TypedefNode*>& getTypedefs() const {
                return Typedefs;
            }

            const std::vector<ConstructorNode*>& getConstructors() const {
                return Constructors;
            }

            const std::vector<DestructorNode*>& getDestructors() const {
                return Destructors;
            }


            bool empty() const { return Members.empty(); }

            void Print(size_t Tabs = 0) const override {
                INDENT(Tabs);
                std::cout << "StructBodyNode(\n";
                for (auto* Member : Members) {
                    Member->Print(Tabs + 1);
                }
                INDENT(Tabs);
                std::cout << ")" << std::endl;
            }
    };

}
}
```

## `src/types/parser/ast/ternary.hh`

```cpp
#pragma once
#include "expression.hh"
#include "utils.hh"

namespace baka {
namespace types {

    class TernaryOpNode : public ExpressionNode {
        ExpressionNode* ConditionExpr;
        ExpressionNode* ThenExpr;
        ExpressionNode* ElseExpr;

        public:
        TernaryOpNode(ExpressionNode* ConditionExpr, ExpressionNode* ThenExpr, ExpressionNode* ElseExpr) : ConditionExpr(ConditionExpr), ThenExpr(ThenExpr), ElseExpr(ElseExpr) {}

        ExpressionNode* getCondition() const { return ConditionExpr; }
        ExpressionNode* getThen() const { return ThenExpr; }
        ExpressionNode* getElse() const { return ElseExpr; }

            void rewriteExpressions(const ExpressionRewriter& rewrite) override { ConditionExpr = rewrite(ConditionExpr); ThenExpr = rewrite(ThenExpr); ElseExpr = rewrite(ElseExpr); }

            void Print(size_t Tabs) const override {
            INDENT(Tabs);
            std::cout << "TernaryOpNode(" << '\n';

            ConditionExpr->Print(Tabs + 1);
            ThenExpr->Print(Tabs + 1);
            ElseExpr->Print(Tabs + 1);

            INDENT(Tabs);
            std::cout << ')' << std::endl;
        }
    };


}
}
```

## `src/types/parser/ast/this.hh`

```cpp
#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
namespace baka {
namespace types {

    class ThisNode : public ASTNode {
    public:
        ThisNode() = default;
        ~ThisNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "This()\n" << std::endl;
        }
    };


}
}
```

## `src/types/parser/ast/typedef.hh`

```cpp
#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/utils.hh"
namespace baka {
namespace types {

    class TypedefNode : public ASTNode {
        bool isConst = false;
        // struct,enum, union or unsigned
        TypeSpecifierModifier Modifier;
        IdentifierNode* BoundReturnType;
        std::vector<DeclarationIdentifierNode*> Variables;

    public:
        TypedefNode(IdentifierNode* BoundReturnType, TypeSpecifierModifier Modifier, bool isConst) :
        isConst(isConst), Modifier(Modifier), BoundReturnType(BoundReturnType), Variables() {}

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Typedef(\n";

            INDENT(Tabs + 1);
            std::cout << "isConst = " << isConst << "\n";

            BoundReturnType->Print(Tabs + 1);
            for (const auto& Variable : Variables) {
                Variable->Print(Tabs + 1);
            }

            INDENT(Tabs + 1);
            std::cout << "Modifier: " << TypeSpecifierModifierToStr[Modifier] << "\n";
            INDENT(Tabs);
            std::cout << ")\n";
        }

        bool getIsConst() const {
            return isConst;
        }

        TypeSpecifierModifier getModifier() const {
            return Modifier;
        }

        IdentifierNode* getTypeName() const {
            return BoundReturnType;
        }

        const std::vector<DeclarationIdentifierNode*>& getVariables() const {
            return Variables;
        }


        void AddVariable(DeclarationIdentifierNode* Variable) {
            Variables.push_back(Variable);
        }
    };


}
}
```

## `src/types/parser/ast/typenode.hh`

```cpp
#pragma once
#include "ast_node.hh"
#include "types/parser/ast/utils.hh"
#include <iostream>
#include <string_view>

#include "identifier.hh"

namespace baka {

	namespace types {

		class TypeNode : public ASTNode
		{
			bool isConst = false;
			TypeSpecifierModifier Modifier = TypeSpecifierModifier::NONE;
			IdentifierNode* TypeName;
			// No identifier for name

		public:
			TypeNode(bool isConst, TypeSpecifierModifier modifier, IdentifierNode* TypeName) :
				isConst(isConst), Modifier(modifier), TypeName(TypeName)
			{
			}

			bool getIsConst() const
			{
				return isConst;
			}

			TypeSpecifierModifier getModifier() const
			{
				return Modifier;
			}

			IdentifierNode* getTypeName() const
			{
				return TypeName;
			}

			void Print(size_t Tabs = 0) const override
			{
				INDENT(Tabs);
				std::cout << "TypeNode(" << "\n";
				INDENT(Tabs + 1);
				std::cout << "isConst: " << isConst << "\n";
				INDENT(Tabs + 1);
				std::cout << "TypeSpecifierModifier: "<< TypeSpecifierModifierToStr[Modifier] << "\n";
				if (TypeName) {
					TypeName->Print(Tabs + 1);
				} else {
					INDENT(Tabs + 1);
					std::cout << "nullptr\n";
				}
				INDENT(Tabs);
				std::cout << ")\n";
			}

		};


	}
}
```

## `src/types/parser/ast/union.hh`

```cpp
#pragma once

#include "types/parser/ast/ast_node.hh"
#include "types/parser/ast/structBody.hh"
#include "types/parser/ast/utils.hh"
namespace baka {
namespace types {

    using UnionBodyNode = StructBodyNode; // union body is the same as struct body

    class UnionNode : public ASTNode {
        UnionBodyNode* body;
        IdentifierNode* Name;
    public:
        UnionNode(UnionBodyNode* body, IdentifierNode* name = nullptr) : body(body), Name(name) {}
        IdentifierNode* getName() const {
            return Name;
        }

        UnionBodyNode* getBody() const {
            return body;
        }

        ~UnionNode() = default;

        void Print(size_t Tabs = 0) const override {
            INDENT(Tabs);
            std::cout << "Union(" << std::endl;
            if (Name) Name->Print(Tabs + 1);
            if (body) body->Print(Tabs + 1);
            INDENT(Tabs);
            std::cout << ")\n";
        }
    };

}
}
```

## `src/types/parser/ast/utils.hh`

```cpp
#pragma once
#include <string_view>
#include <cstdint>
#include <iostream>
#include <unordered_map>

#define INDENT(Tabs) for (size_t i = 0; i < Tabs; i++) std::cout << "    "

namespace baka {

namespace types {

    // Before semantics the ID is zero, preserving parser-only AST output.
    // On a loop/switch/label it defines an identity; on a jump it names the
    // resolved destination. The backend maps this ID to its own basic blocks.
    inline void PrintControlTarget(uint32_t target, size_t tabs) {
        if (!target) return;
        INDENT(tabs);
        std::cout << "Target: " << target << '\n';
    }


    enum class TypeSpecifierModifier {
        NONE,
        UNSIGNED,
        STRUCT,
        CLASS,
        ENUM,
        UNION
    };

    inline std::unordered_map<TypeSpecifierModifier, std::string_view> TypeSpecifierModifierToStr = {
        {TypeSpecifierModifier::NONE, "None"},
        {TypeSpecifierModifier::UNSIGNED, "Unsigned"},
        {TypeSpecifierModifier::STRUCT, "Struct"},
        {TypeSpecifierModifier::CLASS, "Class"},
        {TypeSpecifierModifier::ENUM, "Enum"},
        {TypeSpecifierModifier::UNION, "Union"}
    };

}

namespace detail {

    inline bool isUnsignedTypeName(std::string_view TypeName) {
        if(TypeName == "long") return true;
        if(TypeName == "char") return true;
        if(TypeName == "int") return true;
        if(TypeName == "short") return true;
        return false;
    }
}
}
```

# Appendix B — Lexer/token definitions and ownership support, verbatim

## `src/types/token/token.hh`

```cpp
#pragma once
#include <string>
#include <string_view>
#include <variant>
#include "memory/custom_strings.hh"


namespace baka {
namespace types {
    enum class TokenType {
        // https://timsong-cpp.github.io/cppwp/lex.key
        K_BOOL,
        K_BREAK,
        K_CASE,
        K_CHAR,
        K_CLASS,
        K_CONST,
        K_CONTINUE,
        K_DELETE,
        K_DEFAULT,
        K_DO,
        K_DOUBLE,
        K_ELSE,
        K_ENUM,
        K_FALSE,
        K_FLOAT,
        K_FOR,
        K_GOTO,
        K_IF,
        K_INT,
        K_LONG,
        K_NAMESPACE,
        K_NEW,
        K_NULLPTR,
        K_RETURN,
        K_SHORT,
        K_SIGNED,
        K_SIZEOF,
        K_STATIC,
        K_STRUCT,
        K_SWITCH,
        K_THIS,
        K_TRUE,
        K_TYPEDEF,
        K_UNION,
        K_UNSIGNED,
        K_USING,
        K_VOID,
        K_WHILE,

        // operators: https://timsong-cpp.github.io/cppwp/lex.operators#nt:operator-or-punctuator
        // unary operators
        OP_TILDE,
        OP_UNARY_MINUS,
        OP_DEC,
        OP_INC,
        // binary operators
        OP_MUL,
        OP_DIV,
        OP_MOD,
        OP_BIT_AND,
        OP_BIT_OR,
        OP_BIT_XOR,
        OP_LSHIFT,
        OP_RSHIFT,
        OP_LOGICAL_AND,
        OP_LOGICAL_OR,
        OP_EQ,
        OP_NEQ,
        OP_LT,
        OP_GT,
        OP_LE,
        OP_GE,
        OP_PLUS,
        OP_LOGICAL_NOT,
        // probably will not support these
        // OP_DOT_STAR, .*
        // OP_ARROW_STAR, ->*

        // assignment operators
        OP_ASSIGN,
        OP_ADD_ASSIGN,
        OP_SUB_ASSIGN,
        OP_MUL_ASSIGN,
        OP_DIV_ASSIGN,
        OP_MOD_ASSIGN,
        OP_AND_ASSIGN,
        OP_OR_ASSIGN,
        OP_XOR_ASSIGN,
        OP_LSHIFT_ASSIGN,
        OP_RSHIFT_ASSIGN,
        OP_COLON,
        OP_DOUBLE_COLON,
        OP_COMMA,
        OP_QUESTION,
        OP_DOT,
        OP_ARROW,
        OP_ELLIPSIS,

        //@
        AT,

        // punctuators
        LPAREN_ROUND,
        RPAREN_ROUND,
        LPAREN_SQUARE,
        RPAREN_SQUARE,
        LPAREN_CURLY,
        RPAREN_CURLY,
        SEMICOLON,

        // identifiers
        IDENTIFIER,

        // literals: https://timsong-cpp.github.io/cppwp/lex.literal
        // If a token matches both user-defined-literal and another literal kind, it is treated as the latter.
        LITERAL_INTEGER,
        LITERAL_CHARACTER,
        LITERAL_FP,
        LITERAL_STRING,
        // LITERAL_PTR, basically nullptr which is keyword
        // LITERAL_NULL, this is macro not required
        // LITERAL_USER_DEFINED, not supporting this. its like in chrono 1s is lexed

        // header name
        HEADER_NAME, // possible needed by preprocessor

        // special:
        EOF_TOKEN,
        UNKNOWN
    };

    struct TokenSourceLocation {
        std::string_view FilePath;
        size_t LineNo;
        size_t Col;
    };

    struct Token {
        TokenType TokenType_v;
        std::variant<char, baka::base::SLString, std::string_view, std::string,  int, long long, unsigned int, unsigned long long, float, double> Value;
        // char - needed for char literal
        // string - string literal
        // string_view - any other lexeme
    };


    enum class NumericLiteralType {
        INT,
        LONG,
        UNSIGNED_INT,
        UNSIGNED_LONG,
        FLOAT,
        DOUBLE
    };
}
}
```

## `src/types/token/all.hh`

```cpp
#pragma once
#include "token.hh"
#include "pretty.hh"
```

## `src/types/token/pretty.hh`

```cpp
#pragma once
#include <cassert>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include "token.hh"


namespace baka {
namespace {
    inline std::unordered_map<types::TokenType, std::string_view> TokenToStr = {
        {types::TokenType::K_BOOL, "K_BOOL"},
        {types::TokenType::K_BREAK, "K_BREAK"},
        {types::TokenType::K_CASE, "K_CASE"},
        {types::TokenType::K_CHAR, "K_CHAR"},
        {types::TokenType::K_CLASS, "K_CLASS"},
        {types::TokenType::K_CONST, "K_CONST"},
        {types::TokenType::K_CONTINUE, "K_CONTINUE"},
        {types::TokenType::K_DEFAULT, "K_DEFAULT"},
        {types::TokenType::K_DELETE, "K_DELETE"},
        {types::TokenType::K_DO, "K_DO"},
        {types::TokenType::K_DOUBLE, "K_DOUBLE"},
        {types::TokenType::K_ELSE, "K_ELSE"},
        {types::TokenType::K_ENUM, "K_ENUM"},
        {types::TokenType::K_FALSE, "K_FALSE"},
        {types::TokenType::K_FLOAT, "K_FLOAT"},
        {types::TokenType::K_FOR, "K_FOR"},
        {types::TokenType::K_GOTO, "K_GOTO"},
        {types::TokenType::K_IF, "K_IF"},
        {types::TokenType::K_INT, "K_INT"},
        {types::TokenType::K_LONG, "K_LONG"},
        {types::TokenType::K_NAMESPACE, "K_NAMESPACE"},
        {types::TokenType::K_NEW, "K_NEW"},
        {types::TokenType::K_NULLPTR, "K_NULLPTR"},
        {types::TokenType::K_RETURN, "K_RETURN"},
        {types::TokenType::K_SHORT, "K_SHORT"},
        {types::TokenType::K_SIGNED, "K_SIGNED"},
        {types::TokenType::K_SIZEOF, "K_SIZEOF"},
        {types::TokenType::K_STATIC, "K_STATIC"},
        {types::TokenType::K_STRUCT, "K_STRUCT"},
        {types::TokenType::K_SWITCH, "K_SWITCH"},
        {types::TokenType::K_THIS, "K_THIS"},
        {types::TokenType::K_TRUE, "K_TRUE"},
        {types::TokenType::K_TYPEDEF, "K_TYPEDEF"},
        {types::TokenType::K_UNION, "K_UNION"},
        {types::TokenType::K_UNSIGNED, "K_UNSIGNED"},
        {types::TokenType::K_USING, "K_USING"},
        {types::TokenType::K_VOID, "K_VOID"},
        {types::TokenType::K_WHILE, "K_WHILE"},
        {types::TokenType::OP_TILDE, "OP_TILDE"},
        {types::TokenType::OP_UNARY_MINUS, "OP_UNARY_MINUS"},
        {types::TokenType::OP_DEC, "OP_DEC"},
        {types::TokenType::OP_INC, "OP_INC"},
        {types::TokenType::OP_MUL, "OP_MUL"},
        {types::TokenType::OP_DIV, "OP_DIV"},
        {types::TokenType::OP_MOD, "OP_MOD"},
        {types::TokenType::OP_BIT_AND, "OP_BIT_AND"},
        {types::TokenType::OP_BIT_OR, "OP_BIT_OR"},
        {types::TokenType::OP_BIT_XOR, "OP_BIT_XOR"},
        {types::TokenType::OP_LSHIFT, "OP_LSHIFT"},
        {types::TokenType::OP_RSHIFT, "OP_RSHIFT"},
        {types::TokenType::OP_LOGICAL_AND, "OP_LOGICAL_AND"},
        {types::TokenType::OP_LOGICAL_OR, "OP_LOGICAL_OR"},
        {types::TokenType::OP_EQ, "OP_EQ"},
        {types::TokenType::OP_NEQ, "OP_NEQ"},
        {types::TokenType::OP_LT, "OP_LT"},
        {types::TokenType::OP_GT, "OP_GT"},
        {types::TokenType::OP_LE, "OP_LE"},
        {types::TokenType::OP_GE, "OP_GE"},
        {types::TokenType::OP_PLUS, "OP_PLUS"},
        {types::TokenType::OP_LOGICAL_NOT, "OP_LOGICAL_NOT"},
        {types::TokenType::OP_ASSIGN, "OP_ASSIGN"},
        {types::TokenType::OP_ADD_ASSIGN, "OP_ADD_ASSIGN"},
        {types::TokenType::OP_SUB_ASSIGN, "OP_SUB_ASSIGN"},
        {types::TokenType::OP_MUL_ASSIGN, "OP_MUL_ASSIGN"},
        {types::TokenType::OP_DIV_ASSIGN, "OP_DIV_ASSIGN"},
        {types::TokenType::OP_MOD_ASSIGN, "OP_MOD_ASSIGN"},
        {types::TokenType::OP_AND_ASSIGN, "OP_AND_ASSIGN"},
        {types::TokenType::OP_OR_ASSIGN, "OP_OR_ASSIGN"},
        {types::TokenType::OP_XOR_ASSIGN, "OP_XOR_ASSIGN"},
        {types::TokenType::OP_LSHIFT_ASSIGN, "OP_LSHIFT_ASSIGN"},
        {types::TokenType::OP_RSHIFT_ASSIGN, "OP_RSHIFT_ASSIGN"},
        {types::TokenType::OP_COLON, "OP_COLON"},
        {types::TokenType::OP_DOUBLE_COLON, "OP_DOUBLE_COLON"},
        {types::TokenType::OP_COMMA, "OP_COMMA"},
        {types::TokenType::OP_QUESTION, "OP_QUESTION"},
        {types::TokenType::OP_DOT, "OP_DOT"},
        {types::TokenType::OP_ARROW, "OP_ARROW"},
        {types::TokenType::OP_ELLIPSIS, "OP_ELLIPSIS"},
        {types::TokenType::AT, "AT"},
        {types::TokenType::LPAREN_ROUND, "LPAREN_ROUND"},
        {types::TokenType::RPAREN_ROUND, "RPAREN_ROUND"},
        {types::TokenType::LPAREN_SQUARE, "LPAREN_SQUARE"},
        {types::TokenType::RPAREN_SQUARE, "RPAREN_SQUARE"},
        {types::TokenType::LPAREN_CURLY, "LPAREN_CURLY"},
        {types::TokenType::RPAREN_CURLY, "RPAREN_CURLY"},
        {types::TokenType::SEMICOLON, "SEMICOLON"},
        {types::TokenType::IDENTIFIER, "IDENTIFIER"},
        {types::TokenType::LITERAL_INTEGER, "LITERAL_INTEGER"},
        {types::TokenType::LITERAL_CHARACTER, "LITERAL_CHARACTER"},
        {types::TokenType::LITERAL_FP, "LITERAL_FP"},
        {types::TokenType::LITERAL_STRING, "LITERAL_STRING"},

        {types::TokenType::HEADER_NAME, "HEADER_NAME"}, // possible needed by preprocessor
        {types::TokenType::EOF_TOKEN, "EOF_TOKEN"},
        {types::TokenType::UNKNOWN, "UNKNOWN"},
    };


    inline constexpr int kTokenValueColumnWidth = 26;
    inline constexpr int kTokenTypeColumnWidth  = 22;
}

inline std::string_view TokenTypeToStr(types::TokenType tokenType) {
    return TokenToStr.at(tokenType);
}

inline std::ostream& PrintTokenTableHeader(std::ostream& os) {
    os << std::left
       << std::setw(kTokenValueColumnWidth) << "Lexeme"
       << std::setw(kTokenTypeColumnWidth)  << "Token" << '\n';
    os << std::string(kTokenValueColumnWidth + kTokenTypeColumnWidth, '-') << '\n';
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const types::Token& token) {
    std::ostringstream valueStream;
    switch (token.Value.index()) {
    case 0:
        valueStream << "'" << std::get<char>(token.Value) << "'";
        break;
    case 1:
        valueStream << '"' << std::get<baka::base::SLString>(token.Value) << '"';
        break;
    case 2:
        valueStream << std::get<std::string_view>(token.Value);
        break;
    case 3:
        valueStream << std::get<std::string>(token.Value);
        break;
    case 4:
        valueStream << std::get<int>(token.Value) << " (int)";
        break;
    case 5:
        valueStream << std::get<long long>(token.Value) << " (long)";
        break;
    case 6:
        valueStream << std::get<unsigned int>(token.Value) << " (uint)";
        break;
    case 7:
        valueStream << std::get<unsigned long long>(token.Value) << " (ulong)";
        break;
    case 8:
        valueStream << std::get<float>(token.Value) << " (float)";
        break;
    case 9:
        valueStream << std::get<double>(token.Value) << " (double)";
        break;
    default:
        assert(false && "what the helly?");
        break;
    }

    os << std::left
       << std::setw(kTokenValueColumnWidth) << valueStream.str()
       << std::setw(kTokenTypeColumnWidth)  << TokenTypeToStr(token.TokenType_v);

    return os;
}
}
```

## `src/lexer/lexer.hh`

```cpp
#pragma once
#include "types/token/all.hh"
#include <vector>

namespace baka {
    namespace lexer {
        struct Lexed {
            std::vector<types::Token> Tokens;
            std::vector<types::TokenSourceLocation> Locations;
        };

        Lexed Tokenize(std::string_view SourceCode);
    } // namespace lexer
} // namespace baka
```

## `src/lexer/keywords.hh`

```cpp
#pragma once

#include <string_view>
#include <unordered_map>
#include "types/token/all.hh"

namespace baka {
    namespace lexer {
        const inline std::unordered_map<std::string_view, types::TokenType> Kw_lexeme_to_type = {
            {"bool", types::TokenType::K_BOOL},
            {"break", types::TokenType::K_BREAK},
            {"case", types::TokenType::K_CASE},
            {"char", types::TokenType::K_CHAR},
            {"class", types::TokenType::K_CLASS},
            {"const", types::TokenType::K_CONST},
            {"continue", types::TokenType::K_CONTINUE},
            {"default", types::TokenType::K_DEFAULT},
            {"delete", types::TokenType::K_DELETE},
            {"do", types::TokenType::K_DO},
            {"double", types::TokenType::K_DOUBLE},
            {"else", types::TokenType::K_ELSE},
            {"enum", types::TokenType::K_ENUM},
            {"false", types::TokenType::K_FALSE},
            {"float", types::TokenType::K_FLOAT},
            {"for", types::TokenType::K_FOR},
            {"goto", types::TokenType::K_GOTO},
            {"if", types::TokenType::K_IF},
            {"int", types::TokenType::K_INT},
            {"long", types::TokenType::K_LONG},
            {"namespace", types::TokenType::K_NAMESPACE},
            {"new", types::TokenType::K_NEW},
            {"nullptr", types::TokenType::K_NULLPTR},
            {"return", types::TokenType::K_RETURN},
            {"short", types::TokenType::K_SHORT},
            {"signed", types::TokenType::K_SIGNED},
            {"sizeof", types::TokenType::K_SIZEOF},
            {"static", types::TokenType::K_STATIC},
            {"struct", types::TokenType::K_STRUCT},
            {"switch", types::TokenType::K_SWITCH},
            {"this", types::TokenType::K_THIS},
            {"true", types::TokenType::K_TRUE},
            {"typedef", types::TokenType::K_TYPEDEF},
            {"union", types::TokenType::K_UNION},
            {"unsigned", types::TokenType::K_UNSIGNED},
            {"using", types::TokenType::K_USING},
            {"void", types::TokenType::K_VOID},
            {"while", types::TokenType::K_WHILE},
        };
    }
}
```

## `src/lexer/operators.hh`

```cpp
#pragma once

#include <string_view>
#include <unordered_map>
#include "types/token/all.hh"


namespace baka::lexer {
    inline const std::unordered_map<std::string_view, types::TokenType> lexeme_to_operator = {
        {"~", types::TokenType::OP_TILDE},
        {"-", types::TokenType::OP_UNARY_MINUS},
        {"++", types::TokenType::OP_INC},
        {"--", types::TokenType::OP_DEC},
        {"*", types::TokenType::OP_MUL},
        {"/", types::TokenType::OP_DIV},
        {"%", types::TokenType::OP_MOD},
        {"&", types::TokenType::OP_BIT_AND},
        {"|", types::TokenType::OP_BIT_OR},
        {"^", types::TokenType::OP_BIT_XOR},
        {"<<", types::TokenType::OP_LSHIFT},
        {">>", types::TokenType::OP_RSHIFT},
        {"&&", types::TokenType::OP_LOGICAL_AND},
        {"||", types::TokenType::OP_LOGICAL_OR},
        {"==", types::TokenType::OP_EQ},
        {"!=", types::TokenType::OP_NEQ},
        {">", types::TokenType::OP_GT},
        {"<", types::TokenType::OP_LT},
        {">=", types::TokenType::OP_GE},
        {"<=", types::TokenType::OP_LE},
        {"+", types::TokenType::OP_PLUS},
        {"!", types::TokenType::OP_LOGICAL_NOT},
        {"=", types::TokenType::OP_ASSIGN},
        {"+=", types::TokenType::OP_ADD_ASSIGN},
        {"-=", types::TokenType::OP_SUB_ASSIGN},
        {"*=", types::TokenType::OP_MUL_ASSIGN},
        {"/=", types::TokenType::OP_DIV_ASSIGN},
        {"%=", types::TokenType::OP_MOD_ASSIGN},
        {"&=", types::TokenType::OP_AND_ASSIGN},
        {"|=", types::TokenType::OP_OR_ASSIGN},
        {"^=", types::TokenType::OP_XOR_ASSIGN},
        {"<<=", types::TokenType::OP_LSHIFT_ASSIGN},
        {">>=", types::TokenType::OP_RSHIFT_ASSIGN},
        {":", types::TokenType::OP_COLON},
        {"::", types::TokenType::OP_DOUBLE_COLON},
        {",", types::TokenType::OP_COMMA},
        {"?", types::TokenType::OP_QUESTION},
        {".", types::TokenType::OP_DOT},
        {"->", types::TokenType::OP_ARROW},
        {"...", types::TokenType::OP_ELLIPSIS},
        {"@", types::TokenType::AT}
    };
}
```

## `src/base/memory/custom_strings.hh`

```cpp
#pragma once
#include "memory/arena.hh"
#include "memory/arena_alloc_interface.hh"
#include "memory/arena_singleton_wrapper.hh"
#include <string>


namespace baka {
namespace base {

// string for storing string literals
using SLString = std::basic_string<char,
                    std::char_traits<char>,
                    ArenaAllocatorWrapper<
                        char,
                        SingletonArena<
                            Arena<1024>,
                            AllocArenaTag::SL_STRING
                        >
                    >
                >;


}
}
```

## `src/base/memory/arena_alloc_interface.hh`

```cpp
#pragma once

#include <cstddef>
#include <type_traits>

#include "concepts.hh"

namespace baka {
namespace base {

    // singleton area shoudl have getInstance() static function that returns an instance
    // the instance should have:
    //
    // template<class T, class ...Args>
    // requires (sizeof(T) <= SZ)
    // std::span<T> AllocArray(size_t N ,Args&& ...args)
    //
    // and
    //
    // template<class T, class ...Args>
    // requires (sizeof(T) <= SZ)
    // T* Alloc(Args&&... args)
    //


    // commented out things are deprecated
    // https://en.cppreference.com/cpp/memory/allocator
    template<class T, IsSingleton SingletonArenaT>
    requires std::is_trivially_constructible_v<T> // need this cuz this should technically return an unintialized buffer (but we are returning an intialized one)
    class ArenaAllocatorWrapper {
    public:
        using value_type = T;
        // using pointer = T*;
        // using const_pointer = const T*;
        // using reference = T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;

        constexpr ArenaAllocatorWrapper() noexcept = default;

        template<class U>
        constexpr ArenaAllocatorWrapper(const ArenaAllocatorWrapper<U, SingletonArenaT>&) noexcept {}

        T* allocate(std::size_t n) {
            if(n == 0) {
                return nullptr;
            }
            auto& arena = SingletonArenaT::getInstance();
            return arena.template AllocArray<T>(n).data();
        }

        void deallocate(T*, std::size_t) noexcept {
            // noop
        }

        template<class U>
        bool operator==(const ArenaAllocatorWrapper<U, SingletonArenaT>&) const noexcept {
            return true;
        }
    };
}
}
```

## `src/base/memory/custom_arenas.hh`

```cpp
#pragma once
#include "memory/arena.hh"
#include "memory/arena_singleton_wrapper.hh"


namespace baka {
namespace base {

using ASTNodeArena = SingletonArena<Arena<1024>, AllocArenaTag::AST_NODE>;
using UtilArena = SingletonArena<Arena<1024>, AllocArenaTag::UTIL_NODE >;

}
}
```

## `src/base/memory/arena_singleton_wrapper.hh`

```cpp
#pragma once


namespace baka {
namespace base {


// enumerate all kinds of different arenas we need
enum class AllocArenaTag {
    SL_STRING, // one for string literals
    AST_NODE,
    UTIL_NODE
};


template<class Arena, AllocArenaTag TAG_>
class SingletonArena : public Arena {
    private:
        SingletonArena() = default;

    public:
        static SingletonArena& getInstance() {
            static SingletonArena instance;
            return instance;
        }
};

}
}
```

Lexer implementation, included to make token values, scanning/error behavior and positioning inspectable without repository access:

## `src/lexer/lexer.cc`

```cpp
#include "lexer.hh"
#include <cctype>
#include <string>
#include <string_view>
#include <charconv>
#include <climits>
#include "base/base.hh"
#include "line_index.hh"
#include "memory/custom_strings.hh"
#include "types/driver/defs.hh"
#include "driver/gctx.hh"
#include "keywords.hh"
#include "operators.hh"
#include "types/token/token.hh"


#define LEX_ERROR(STR, IDX) driver::Gctx::GenerateLineError(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)
#define LEX_WARN(STR, IDX) driver::Gctx::GenerateLineWarning(LineNo, IDX, LineIndex->CurrentLine(), STR, driver::Stage::LEX)

#define EMIT_TOKEN(TT, VAL) \
    TokenLocations.emplace_back(driver::Gctx::GetSourceFilePath(), LineNo, ColNo); \
    Tokens.emplace_back((TT), (VAL))

#define EMIT_TOKEN_AT(TT, VAL, COL) \
    TokenLocations.emplace_back(driver::Gctx::GetSourceFilePath(), LineNo, (COL)); \
    Tokens.emplace_back((TT), (VAL))

namespace baka {
    namespace lexer {
        Lexed Tokenize(std::string_view SourceCode) {
            std::vector<types::Token> Tokens;

            // todo: dont need to store line ctx if doing single file
            std::vector<types::TokenSourceLocation> TokenLocations;

            size_t LineNo = 1;
            size_t ColNo = 1;

            size_t lh = 0;
            auto LineIndex = driver::Gctx::GetLineIndex();
            LineIndex->startLine(0);

            while (lh < SourceCode.size()) {
                char curr = SourceCode[lh];

                if (base::isValidKeywordNonDigit(curr)) {
                    // keyword or identifier
                    // deal with boolean (kword deals with this already)

                    size_t startIdx = lh;
                    size_t endIdx = startIdx;

                    size_t peek = lh + 1;
                    while (peek < SourceCode.size() &&
                        (base::isValidKeywordNonDigit(SourceCode[peek]) || std::isdigit(SourceCode[peek]))) {
                        endIdx = peek;
                        peek++;
                    }

                    std::string_view lexeme = SourceCode.substr(startIdx, endIdx - startIdx + 1);
                    if (Kw_lexeme_to_type.contains(lexeme)) {
                        EMIT_TOKEN(Kw_lexeme_to_type.at(lexeme), lexeme);
                    }
                    else {
                        EMIT_TOKEN(types::TokenType::IDENTIFIER, lexeme);
                    }
                    lh = endIdx + 1;
                    ColNo += endIdx - startIdx + 1;
                }
                else if (curr == '@') {
                    EMIT_TOKEN(types::TokenType::AT, std::string_view("@"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '(') {
                    EMIT_TOKEN(types::TokenType::LPAREN_ROUND, std::string_view("("));
                    lh++;
                    ColNo++;
                }
                else if (curr == ')') {
                    EMIT_TOKEN(types::TokenType::RPAREN_ROUND, std::string_view(")"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '[') {
                    EMIT_TOKEN(types::TokenType::LPAREN_SQUARE, std::string_view("["));
                    lh++;
                    ColNo++;
                }
                else if (curr == ']') {
                    EMIT_TOKEN(types::TokenType::RPAREN_SQUARE, std::string_view("]"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '{') {
                    EMIT_TOKEN(types::TokenType::LPAREN_CURLY, std::string_view("{"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '}') {
                    EMIT_TOKEN(types::TokenType::RPAREN_CURLY, std::string_view("}"));
                    lh++;
                    ColNo++;
                }
                else if (curr == ';') {
                    EMIT_TOKEN(types::TokenType::SEMICOLON, std::string_view(";"));
                    lh++;
                    ColNo++;
                }
                else if (curr == '\'') {
                    // character literal

                    size_t orgLh = lh; // need for col no calcs for error

                    size_t peek = lh + 1;
                    char ActualChar = '\0';
                    bool CharSeen = false;
                    size_t CharLength = 0;
                    bool RQuoteSeen = false;

                    while (peek < SourceCode.size()) {
                        if (SourceCode[peek] == '\\') {
                            // possibly escape seq
                            peek++;
                            if (peek >= SourceCode.size()) {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                break;
                            }

                            char q = SourceCode[peek];
                            // q must either be a letter for a simple escape
                            // or
                            // q must be a number 0-7 for octal escape seq


                            if (base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                                ActualChar = base::SimpleEscapeSequence(q);
                                CharSeen = true;
                                CharLength++;
                            }
                            else if (std::isdigit(q) && (0 <= (q - '0')) && (7 >= (q - '0'))) {
                                // octal escape sequence
                                // need to do this for \0

                                long long octal_seq = (q - '0');
                                peek++;

                                // octal escape seq is max 3 digits
                                size_t DigitsSeen = 1;

                                while (peek < SourceCode.size() &&
                                    SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                                    DigitsSeen < 3) {
                                    octal_seq *= 8;
                                    octal_seq += (SourceCode[peek] - '0');
                                    DigitsSeen++;
                                    peek++;
                                }
                                peek--; // move it back to last scanned char

                                ActualChar = static_cast<char>(octal_seq);
                                CharSeen = true;
                                CharLength++;
                            }
                            else {
                                // TODO: is line and column even updated????
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                // break; dont break here and search for closing quote
                            }
                        }
                        else if (base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '\'') {
                            // normal ascii char
                            ActualChar = SourceCode[peek];
                            CharSeen = true;
                            CharLength++;
                        }
                        else if (SourceCode[peek] == '\'') {
                            // end literal here
                            RQuoteSeen = true;
                            break;
                        }
                        else {
                            LEX_ERROR("Invalid character in character literal", ColNo + peek - orgLh);
                            // break; no break here to scan for end quote
                        }
                        peek++;
                    }

                    size_t Oldlh = lh;
                    size_t orgColNo = ColNo;
                    lh = ++peek;
                    ColNo += lh - Oldlh;

                    if (RQuoteSeen && CharSeen) {
                        EMIT_TOKEN_AT(types::TokenType::LITERAL_CHARACTER, ActualChar, orgColNo);

                        if (CharLength > 1) {
                            LEX_WARN(
                                "Multilength character literal will always chose the last character as its representation.",
                                ColNo-1);
                        }
                    }
                    else {
                        if (!CharSeen) {
                            LEX_ERROR("no character in character literal", ColNo-1);
                        }
                        if (!RQuoteSeen) {
                            LEX_ERROR("character literal not terminated", orgColNo);
                        }
                        EMIT_TOKEN_AT(types::TokenType::UNKNOWN, ActualChar, orgColNo);
                    }
                }
                else if (curr == '"') {
                    //string literal

                    size_t orgLh = lh; // need for col no calcs for error

                    size_t peek = lh + 1;

                    base::SLString str; // arena alloc

                    bool RQuoteSeen = false;

                    while (peek < SourceCode.size()) {
                        if (SourceCode[peek] == '\\') {
                            // escape sequence

                            peek++;
                            if (peek >= SourceCode.size()) {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                break;
                            }

                            char q = SourceCode[peek];
                            // q is either letter for escape seq
                            // or
                            // q is 0-7 for octal escape seq

                            if (base::isSimpleEscapeSequenceChar(q) && base::isValidStringChar(q)) {
                                // letter escape seq
                                char ToAdd = base::SimpleEscapeSequence(q);
                                str += (ToAdd);
                            }
                            else if (std::isdigit(q) && (0 <= (q - '0')) && (7 >= (q - '0'))) {
                                // octal escape sequence
                                // need to do this for \0

                                long long octal_seq = (q - '0');
                                peek++;

                                // octal escape seq has max 3 digits
                                size_t DigitsSeen = 1;

                                while (peek < SourceCode.size() &&
                                    SourceCode[peek] >= '0' && SourceCode[peek] <= '7' &&
                                    DigitsSeen < 3) {
                                    octal_seq *= 8;
                                    octal_seq += (SourceCode[peek] - '0');
                                    DigitsSeen++;
                                    peek++;
                                }
                                peek--; // move it back to last scanned char

                                str += static_cast<char>(octal_seq);
                            }
                            else {
                                LEX_ERROR("Invalid escape sequence", ColNo + peek - orgLh);
                                // break; dont break here and search for closing quote
                            }
                        }
                        else if (base::isValidStringChar(SourceCode[peek]) && SourceCode[peek] != '"') {
                            // normal ascii char
                            str += SourceCode[peek];
                        }
                        else if (SourceCode[peek] == '"') {
                            // end literal here
                            RQuoteSeen = true;
                            break;
                        }
                        else {
                            LEX_ERROR("Invalid character in string literal", ColNo + peek - orgLh);
                            // break; no break here to scan for end quote
                        }
                        peek++;
                    }

                    size_t Oldlh = lh;
                    size_t orgColNo = ColNo;
                    lh = ++peek;
                    ColNo += lh - Oldlh;

                    if (RQuoteSeen) {
                        EMIT_TOKEN_AT(types::TokenType::LITERAL_STRING, std::move(str), orgColNo);
                    }
                    else {
                        // must be invalid char
                        LEX_ERROR("string literal not terminated", orgColNo);
                        EMIT_TOKEN_AT(types::TokenType::UNKNOWN, std::move(str), orgColNo);
                    }
                }
                else if (std::isdigit(curr) ||
                    (curr == '.' &&
                        (lh + 1 < SourceCode.size()) && std::isdigit(SourceCode[lh + 1]))) {
                    types::NumericLiteralType numType = types::NumericLiteralType::INT;

                    //need this stuff for error reporting
                    size_t orgLh = lh;
                    size_t orgColNo = ColNo;

                    size_t peek = lh + 1;
                    size_t startIdx = lh;

                    bool dotSeen = false;
                    bool eSeen = false;
                    bool uSeen = false;
                    bool lSeen = false;
                    bool fSeen = false;

                    bool chatIsThisOctLarp = false;
                    size_t larpLoc = -1;
                    int nextDigitBase = 10;
                    int numBase = 10;


                    if (curr == '0' && (lh + 1 < SourceCode.size())) {
                        if (std::tolower(SourceCode[lh + 1]) == 'b') {
                            nextDigitBase = 2;
                            numBase = 2;
                            peek++;
                        }
                        else if (std::tolower(SourceCode[lh + 1]) == 'x') {
                            nextDigitBase = 16;
                            numBase = 16;
                            peek++;
                        }
                        else {
                            nextDigitBase = 8;
                            numBase = 8;
                        }
                    }

                    if (curr == '.') {
                        dotSeen = true;
                    }

                    while (peek < SourceCode.size()) {
                        if (base::isValidDigit(SourceCode[peek], nextDigitBase)) {
                            peek++;
                        }
                        else if (nextDigitBase == 8 && std::isdigit(SourceCode[peek])) {
                            chatIsThisOctLarp = true;
                            larpLoc = peek;
                            nextDigitBase = 10;
                        }
                        else if (SourceCode[peek] == '.' && (nextDigitBase == 8 || nextDigitBase == 10 || nextDigitBase
                                == 16) && !dotSeen
                            && !eSeen) {
                            if (nextDigitBase == 8) {
                                nextDigitBase = 10;
                            }
                            dotSeen = true;
                            peek++;
                        }
                        else if (((std::tolower(SourceCode[peek]) == 'p' && nextDigitBase == 16) ||
                                (std::tolower(SourceCode[peek]) == 'e' && nextDigitBase == 10)) && peek + 1 < SourceCode
                            .
                            size() &&
                            ((!eSeen && std::isdigit(SourceCode[peek + 1])) || (!eSeen && (SourceCode[peek + 1] == '-'
                                || SourceCode[peek + 1] == '+') && (peek + 2 <
                                SourceCode.size()) && std::isdigit(SourceCode[peek + 2])))) {
                            eSeen = true;
                            nextDigitBase = 10;
                            if (base::isValidDigit(SourceCode[peek + 1], nextDigitBase)) {
                                peek++;
                            }
                            else {
                                peek += 2;
                            }
                        }
                        else if (std::tolower(SourceCode[peek]) == 'u' && !dotSeen && !eSeen && !uSeen && !fSeen) {
                            uSeen = true;
                            peek++;
                            if (peek > SourceCode.size() || std::tolower(SourceCode[peek]) != 'l'){
                                break;
                            }
                        }
                        else if (std::tolower(SourceCode[peek]) == 'l' && !dotSeen && !eSeen && !lSeen && !fSeen) {
                            lSeen = true;
                            peek++;
                            break;
                        }
                        else if (std::tolower(SourceCode[peek]) == 'f' && (dotSeen || eSeen) && !lSeen && !fSeen) {
                            fSeen = true;
                            peek++;
                            break;
                        }
                        else {
                            break;
                        }
                    }


                    lh += peek - startIdx;
                    ColNo += peek - startIdx;
                    if (dotSeen || eSeen) {
                        if (fSeen) {
                            numType = types::NumericLiteralType::FLOAT;
                        }
                        else {
                            numType = types::NumericLiteralType::DOUBLE;
                        }
                    }
                    else {
                        if (chatIsThisOctLarp) {
                            LEX_ERROR(std::string("Invalid digit '") + SourceCode[larpLoc] + "' in octal number",
                                      orgColNo + (larpLoc - orgLh));
                        }
                        if (uSeen) {
                            if (lSeen) {
                                numType = types::NumericLiteralType::UNSIGNED_LONG;
                            }
                            else {
                                numType = types::NumericLiteralType::UNSIGNED_INT;
                            }
                        }
                        else {
                            if (lSeen) {
                                numType = types::NumericLiteralType::LONG;
                            }
                            else {
                                numType = types::NumericLiteralType::INT;
                            }
                        }
                    }

                    switch (numType) {
                    case types::NumericLiteralType::INT:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > INT_MAX) {
                                    LEX_ERROR("Integer literal overflows int", orgColNo);
                                }
                                else {
                                    EMIT_TOKEN_AT(types::TokenType::LITERAL_INTEGER, static_cast<int>(x), orgColNo);
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::LONG:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > LLONG_MAX) {
                                    LEX_ERROR("Integer literal overflows long", orgColNo);
                                }
                                else {
                                    EMIT_TOKEN_AT(types::TokenType::LITERAL_INTEGER, static_cast<long long>(x), orgColNo);
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::UNSIGNED_INT:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                if (x > UINT_MAX) {
                                    LEX_ERROR("Integer literal overflows unsigned int", orgColNo);
                                }
                                else {
                                    EMIT_TOKEN_AT(types::TokenType::LITERAL_INTEGER,
                                        static_cast<unsigned int>(x), orgColNo);
                                }
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::UNSIGNED_LONG:
                        {
                            if (numBase == 2 || numBase == 16) {
                                startIdx += 2;
                            }
                            unsigned long long x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             numBase);
                            if (ec == std::errc()) {
                                EMIT_TOKEN_AT(types::TokenType::LITERAL_INTEGER, x, orgColNo);
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::FLOAT:
                        {
                            std::chars_format fmtBase = std::chars_format::general;
                            if (numBase == 16) {
                                startIdx += 2;
                                fmtBase = std::chars_format::hex;
                            }
                            double x = 0;
                            auto [ptr, ec] =
                                std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x, fmtBase);
                            if (ec == std::errc()) {
                                EMIT_TOKEN_AT(types::TokenType::LITERAL_FP, static_cast<float>(x), orgColNo);
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    case types::NumericLiteralType::DOUBLE:
                        {
                            std::chars_format fmtBase = std::chars_format::general;
                            if (numBase == 16) {
                                startIdx += 2;
                                fmtBase = std::chars_format::hex;
                            }
                            double x = 0;
                            auto [ptr, ec] = std::from_chars(SourceCode.data() + startIdx, SourceCode.data() + lh, x,
                                                             fmtBase);
                            if (ec == std::errc()) {
                                EMIT_TOKEN_AT(types::TokenType::LITERAL_FP, x, orgColNo);
                            }
                            else {
                                LEX_ERROR("Not a valid numeric literal", orgColNo);
                            }
                        }
                        break;
                    }
                }
                else if (curr == ' ' || curr == '\t') {
                    lh++;
                    ColNo++;
                }
                else if (curr == '\n') {
                    LineNo++;

                    LineIndex->endLine(lh);
                    lh++;
                    LineIndex->startLine(lh);

                    ColNo = 1;
                }
                else if(SourceCode[lh] == '/' && (lh + 1 < SourceCode.size()) && SourceCode[lh+1] == '/') {
                    // skip comment
                    while (lh < SourceCode.size() && SourceCode[lh] != '\n') {
                        lh++;
                        ColNo++;
                    }
                }
                else if(SourceCode[lh] == '/' && (lh + 1 < SourceCode.size()) && SourceCode[lh + 1] == '*') {
                    // skip block comment
                    lh+=2;
                    ColNo+=2;

                    size_t orgLineNo = LineNo;
                    size_t orgColNo = ColNo-1;
                    auto orgLine = LineIndex->CurrentLine();

                    bool blockEndFound = false;
                    while (lh < SourceCode.size()) {
                        if(SourceCode[lh] == '*' && (lh + 1) < SourceCode.size() && SourceCode[lh + 1] == '/') {
                            blockEndFound = true;
                            ColNo += 2;
                            lh += 2;
                            break;
                        }

                        if(SourceCode[lh] == '\n') {
                            LineNo++;

                            LineIndex->endLine(lh);
                            lh++;
                            LineIndex->startLine(lh);

                            ColNo = 1;
                        } else {
                            lh++;
                            ColNo++;
                        }
                    }
                    if(!blockEndFound) {
                        driver::Gctx::GenerateLineError(orgLineNo, orgColNo, orgLine, "Block comment not terminated" , driver::Stage::LEX);
                    }
                }
                else if (base::isValidOperatorChar(curr)) {
                    // this is operator
                    if (curr == '.') {
                        // check for ellipsis
                        if (lh + 2 < SourceCode.size() && SourceCode[lh + 1] == '.' && SourceCode[lh + 2] == '.') {
                            EMIT_TOKEN(types::TokenType::OP_ELLIPSIS,
                                                std::string_view(SourceCode.substr(lh, 3)));
                            lh += 3;
                            ColNo += 3;
                        }
                        else {
                            EMIT_TOKEN(types::TokenType::OP_DOT, std::string_view(SourceCode.substr(lh, 1)));
                            lh++;
                            ColNo++;
                        }
                    }
                    else {
                        size_t startIdx = lh;
                        size_t endIdx = startIdx;

                        size_t peek = lh + 1;
                        while (peek < SourceCode.size() && base::isValidOperatorChar(SourceCode[peek]) &&
                            lexeme_to_operator.contains(SourceCode.substr(startIdx, peek - startIdx + 1))) {
                            endIdx = peek;
                            peek++;
                        }
                        std::string_view lexeme = SourceCode.substr(startIdx, endIdx - startIdx + 1);
                        EMIT_TOKEN(lexeme_to_operator.at(lexeme), lexeme);
                        lh = endIdx + 1;
                        ColNo += endIdx - startIdx + 1;
                    }
                }
                else {
                    LEX_ERROR("Unknown token", ColNo);

                    EMIT_TOKEN(types::TokenType::UNKNOWN, std::string_view(SourceCode.data() + lh, 1));
                    lh++;
                    ColNo++;
                }
            }
            LineIndex->endLine(lh);
            EMIT_TOKEN(types::TokenType::EOF_TOKEN, std::string_view());

            return Lexed{std::move(Tokens), std::move(TokenLocations)};
        }
    } // namespace lexer
} // namespace baka
```

# Appendix C — Parser interface and operational grammar source, verbatim

## `src/parser/parser.hh`

```cpp
#pragma once
#include "symbol_table/symbol_table.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/initializer.hh"
#include "types/parser/ast/statement.hh"
#include "types/parser/ast/structBody.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/union.hh"
#include "types/token/all.hh"
#include <initializer_list>
#include <span>
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/labelStatement.hh"
#include "types/parser/ast/jump.hh"
#include "types/parser/ast/selectionStatement.hh"
#include "types/parser/ast/iterationStatement.hh"
#include "types/parser/ast/typenode.hh"
#include "types/token/token.hh"
#include "utils.hh"

// functions in here:
// check - check of current token is of some type
// advance - return token and move fwd
// match - advance if checks is true
// peek - returns current token
// previous - returns previously matched token



namespace baka {
namespace parser {
    struct ParserSTE
    {
        private:
            uint8_t flag = 0;

        static constexpr uint8_t IS_STRUCT = 1 << 0;
        static constexpr uint8_t IS_STRUCT_DEFINED = 1 << 1;

        public:
            bool IsStruct() const noexcept { return flag & IS_STRUCT; }
            bool IsStructDefined() const noexcept { return flag & IS_STRUCT_DEFINED; }

            void SetIsStruct() noexcept { flag |= IS_STRUCT; }
            void SetIsStructDefined() {
                if(!IsStruct()) {
                    throw std::runtime_error("Cannot set struct defined flag on non-struct type");
                }
                flag |= IS_STRUCT_DEFINED;
            }

    };

    class Parser {
        size_t current = 0;
        std::span<types::Token> Tokens;
        base::ParserSymbolTable<ParserSTE> TypeLookup;
        std::span<types::TokenSourceLocation> TokenSourceLocations;
        size_t ReportedErrorCount = 0;

        public:
        Parser(std::span<types::Token> Tokens, std::span<types::TokenSourceLocation> TokenSourceLocations) : Tokens(Tokens), TokenSourceLocations(TokenSourceLocations) {};
        types::ASTNode* Parse();

        // states needed for parser
        bool InTypedef = false;

        // for typedef redefinition exemption
        class TypedefFlagGuard {
            Parser& Self;
            bool Saved;
        public:
            TypedefFlagGuard(Parser& parser, bool NewValue) :
                Self(parser), Saved(Self.InTypedef) {
                Self.InTypedef = NewValue;
            }
            ~TypedefFlagGuard() {
                Self.InTypedef = Saved;
            }
        };


        // type lookup helpers:
        bool LookupType(types::IdentifierNode* Identifier);
        ParserSTE* GetParserSTE(types::IdentifierNode* Identifier);
        bool LookupType(const std::string_view& Name);
        void EnterScope();
        void ExitScope();
        void AddType(types::IdentifierNode* Identifier);
        bool RegisterOrReplaceType(types::IdentifierNode* Identifier);


        // Composite nodes are usually allocated AFTER their children/body were
        // parsed. Callers must capture the introducing token before descending;
        // using current-1 here would point diagnostics at ')' or a closing brace.
        // Expression builders may instead pass their operator token explicitly.
        template <class T, class... Args>
        T* AllocNodeAt(size_t TokenIndex, Args&&... Arguments) {
            T* Node = ASTALLOC.Alloc<T>(std::forward<Args>(Arguments)...);
            if (!TokenSourceLocations.empty()) {
                auto Index = std::min(TokenIndex, TokenSourceLocations.size() - 1);
                Node->setSourceSpan(TokenSourceLocations[Index].LineNo, TokenSourceLocations[Index].Col);
            }
            return Node;
        }

        // helpers
        bool Check(types::TokenType type) const noexcept;
        bool Check2(types::TokenType type) const noexcept;
        bool CheckN(size_t Offset, types::TokenType type) const noexcept;
        const types::Token& Advance();
        bool Match(types::TokenType type);
        const types::Token& Peek() const noexcept;
        const types::Token& Previous() const;
        void ReportError(std::string Message, bool ReportPreviousToken = false);
        types::TokenType SkipTo(std::initializer_list<types::TokenType> TokenList);

        bool isTypeName(const types::Token& token);

        // recursive descent nodes:

        types::ProgramNode* ParseProgram();

        types::ConstructorNode* ParseConstructor(types::IdentifierNode* ParentName);
        types::DestructorNode* ParseDestructor(types::IdentifierNode* ParentName);
        std::optional<types::FunctionNode*> TryParseFunction();
        types::FunctionParameter* ParseFunctionParameter();
        types::FunctionParameterList* ParseFunctionParameterList();
        std::variant<types::StructDefinitionNode*, types::StructDeclarationNode*> ParseStruct();
        std::variant<types::ClassDefinitionNode*, types::ClassDeclarationNode*> ParseClass();
        types::StructBodyNode* ParseStructBody(types::IdentifierNode* ParentName);

        types::UnionNode* ParseUnion();
        types::UnionBodyNode* ParseUnionBody(types::IdentifierNode* ParentName);

        types::IdentifierNode* ParseTypeIdentifier();

        types::StatementNode* ParseStatement();
        bool AtDeclarationStart();
        bool AtAggregateDefinition();
        bool TryParseLocalAggregate(types::CompoundStatementNode* Node);
        types::IdentifierNode* ParseLoopSuffixLabel();
        types::CompoundStatementNode* CompoundStatement();

        types::JumpStatementNode* JumpStatement();
        types::ReturnStatementNode* ReturnStatement();
        types::GotoStatementNode* GotoStatement();
        types::BreakStatementNode* BreakStatement();
        types::ContinueStatementNode* ContinueStatement();
        types::IfSuperBlockStatementNode* IfSuperBlockStatement();
        types::IfSubBlockStatementNode* IfSubBlockStatement();
        types::ElseIfSubBlockStatementNode* ElseIfSubBlockStatement();
        types::ElseSubBlockStatementNode* ElseSubBlockStatement();
        types::SwitchBlockStatementNode* SwitchBlockStatement();
        types::SelectionStatementNode* SelectionStatement();
        types::ForBlockStatementNode* ForBlockStatement();
        types::WhileBlockStatementNode* WhileBlockStatement();
        types::DoWhileBlockStatementNode* DoWhileBlockStatement();
        types::IterationStatementNode* IterationStatement();
        types::GotoLabelStatementNode* GotoLabelStatement();
        types::CaseLabelStatementNode* CaseLabelStatement();
        types::DefaultLabelStatementNode* DefaultLabelStatement();
        types::LabelStatementNode* LabelStatement();

        types::TypedefNode* ParseTypedef();

        types::ExpressionStatementNode* ParseExpressionStatement();

        types::TypeSpecifierModifier DetermineTypeSpecifierModifier() const;
        types::StatementNodeType DetermineStatementType();
        types::EnumNode* ParseEnumDecl();

        types::ExpressionNode* Expression();
        // types::UnaryExpressionNode* UnaryExpression();
        types::ConstantNode* ParseConstantNode();


        // expressions:
        types::ExpressionNode* ParseCommaExpression();

        types::ExpressionNode* ParsePrimaryExpression();
        types::ExpressionNode* ParseSizeofExpression();


        types::ExpressionNode* ParseFactor();
        std::optional<types::TypeNode*> TryParseTypeName();

        // for ++, -- , [], ->, .
        types::ExpressionNode* ParsePostfixExpression();
        types::ExpressionNode* ParseAssignmentExpression(size_t MinPrecedence = 1);
        types::IdentifierNode* ParseIdentifier();

        types::DeclarationList* ParseDeclarationList();
        types::SingleDeclarationNode* ParseSingleDeclaration();
        types::DeclarationIdentifierNode* ParseDeclarationIdentifier();
        types::DeclarationIdentifierNode* ParseAbstractDeclarator();
        types::InitializerNode* ParseInitializer(); // { intializer , intializer .... } or just 'assingmentexpr'
    };

}
}
```

## `src/parser/utils.hh`

```cpp
#pragma once
#include "types/token/all.hh"
#include "types/token/token.hh"
#include "memory/custom_arenas.hh"

#define ASTALLOC baka::base::ASTNodeArena::getInstance()

namespace baka {
namespace parser {
namespace detail {

    inline bool isConstantToken(types::TokenType TokenType_v) {
        switch (TokenType_v) {
            case types::TokenType::LITERAL_CHARACTER:
            case types::TokenType::LITERAL_FP:
            case types::TokenType::LITERAL_STRING:
            case types::TokenType::LITERAL_INTEGER:
            case types::TokenType::K_TRUE:
            case types::TokenType::K_FALSE:
            case types::TokenType::K_NULLPTR:
                return true;
            default:
                return false;
        }
    }

    inline bool IsUnaryOperator(types::TokenType Type) {
        switch (Type) {
            case types::TokenType::OP_UNARY_MINUS:   // unary -
            case types::TokenType::OP_PLUS:           // unary +
            case types::TokenType::OP_TILDE:          // bitwise NOT ~
            case types::TokenType::OP_LOGICAL_NOT:    // logical NOT !
            case types::TokenType::OP_INC:            // prefix ++
            case types::TokenType::OP_DEC:            // prefix --
            case types::TokenType::OP_MUL:            // dereference *
            case types::TokenType::OP_BIT_AND:        // address-of &
            case types::TokenType::K_SIZEOF:          // sizeof
                return true;
            default:
                return false;
        }
    }

    inline bool IsSpecifier(types::TokenType Type)
    {
        switch (Type) {
            case types::TokenType::K_CONST:
            case types::TokenType::K_STATIC:
            case types::TokenType::K_STRUCT:
            case types::TokenType::K_CLASS:
            case types::TokenType::K_ENUM:
            case types::TokenType::K_UNSIGNED:
                return true;
            default:
                return false;
        }
    }

}
}
}
```

## `src/base/symbol_table/symbol_table.hh`

```cpp
#pragma once

#include "memory/custom_arenas.hh"
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>

namespace baka {
namespace base {

struct SymbolTableUpdate {
    std::string_view Symbol;
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class SymbolTable {

    // Node-based storage keeps entry addresses stable across rehashes.
    using Map = std::unordered_map<std::string_view, SymbolTableEntry>;

    std::vector<Map> Scopes;

public:
    SymbolTable() = default;

    void EnterNewScope() {
        Scopes.emplace_back();
    }

    void ExitScope() {
        if (!Scopes.empty()) Scopes.pop_back();
    }

    std::optional<SymbolTableEntry> Lookup(std::string_view IdentifierName) {
        for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
            auto found = it->find(IdentifierName);
            if (found != it->end()) {
                return found->second;
            }
        }

        return std::nullopt;
    }

    bool AddEntry(std::string_view Symbol, SymbolTableEntry Entry) {
        if (Scopes.empty()) EnterNewScope();
        if (Scopes.back().find(Symbol) != Scopes.back().end()) {
            return false;
        }
        Scopes.back().insert({Symbol, std::move(Entry)});
        return true;
    }

    bool RegisterOrReplaceType(std::string_view Symbol, SymbolTableEntry Entry) {
        if (Scopes.empty()) EnterNewScope();
        // true if symbol is new
        auto it = Scopes.back().find(Symbol);
        if (it != Scopes.back().end()) {
            it->second = std::move(Entry);
            return false;
        }
        Scopes.back().insert({Symbol, std::move(Entry)});
        return true;
    }

    std::optional<SymbolTableEntry> LookupCurrent(std::string_view IdentifierName) const {
        if (Scopes.empty()) {
            return std::nullopt;
        }
        auto it = Scopes.back().find(IdentifierName);
        if (it == Scopes.back().end()) {
            return std::nullopt;
        }
        return it->second;
    }

    // Stable until the containing scope is exited.
    SymbolTableEntry* GetEntry(std::string_view Symbol) {
        for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it) {
            auto found = it->find(Symbol);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
};


template<class SymbolTableEntry, class Alloc = base::UtilArena>
class ParserSymbolTable : public SymbolTable<SymbolTableEntry, Alloc> {

    public:
        ParserSymbolTable() {
            this->EnterNewScope();
            this->AddEntry("int", SymbolTableEntry()); // has unsigned
            this->AddEntry("float", SymbolTableEntry());
            this->AddEntry("double", SymbolTableEntry());
            this->AddEntry("char", SymbolTableEntry()); // has unsigned
            this->AddEntry("void", SymbolTableEntry());
            this->AddEntry("long", SymbolTableEntry());  // has unsigned
            this->AddEntry("short", SymbolTableEntry());  // has unsigned
            this->AddEntry("bool", SymbolTableEntry());
            // this->AddEntry("signed", SymbolTableEntry());
            // this->AddEntry("unsigned", SymbolTableEntry()); //TODO NOT IMPLEMENTED SOME GET ON THIS RNRNRN
        }

};

}
}
```

## `src/parser/class.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
    namespace parser {
        std::variant<types::ClassDefinitionNode *, types::ClassDeclarationNode *> Parser::ParseClass() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CLASS)) {
                assert(false && "not be possible");
            }


            if (!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after 'class' keyword");
                auto Sync = SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
                return AllocNodeAt<types::ClassDeclarationNode>(SourceStart,
                    AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?")));
            }

            types::IdentifierNode *ClassIdentifier = this->ParseIdentifier();

            if (Match(types::TokenType::SEMICOLON)) {
                // this is a declaration
                auto *Node = AllocNodeAt<types::ClassDeclarationNode>(SourceStart, ClassIdentifier);
                if (!TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
                    AddType(ClassIdentifier);
                    GetParserSTE(ClassIdentifier)->SetIsStruct();
                } else if(!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    ReportError("not a class");
                }
                return Node;
            }

            // surely a definition
            if (TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
                // TODO throw error
                if (GetParserSTE(ClassIdentifier)->IsStructDefined()) {
                    ReportError("redefinition of class");
                } else if (!GetParserSTE(ClassIdentifier)->IsStruct()) {
                    ReportError("not a class");
                }
            }

            types::IdentifierNode *ParentStructIdentifier = nullptr;
            if (this->Match(types::TokenType::OP_COLON)) {
                if (!this->Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after ':'");
                } else {
                    ParentStructIdentifier = this->ParseIdentifier();
                }
                if (ParentStructIdentifier && (!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->IsStructDefined())) {
                    ReportError("parent struct/class not found in symbol table");
                }
            }

            if (!TypeLookup.LookupCurrent(ClassIdentifier->GetName()).has_value()) {
                AddType(ClassIdentifier);
                GetParserSTE(ClassIdentifier)->SetIsStruct();
            }
            if (GetParserSTE(ClassIdentifier)->IsStruct())
                GetParserSTE(ClassIdentifier)->SetIsStructDefined();
            EnterScope();

            types::StructBodyNode *Body = nullptr;
            if (!this->Check(types::TokenType::LPAREN_CURLY)) {
                ReportError("expected '{' after struct/class declaration");
                Body = AllocNodeAt<types::StructBodyNode>(SourceStart);
            } else {
                Body = this->ParseStructBody(ClassIdentifier);
            }
            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after struct/class body", true);
                SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
                Match(types::TokenType::SEMICOLON);
            }

            types::ClassDefinitionNode *Node = nullptr;
            if (ParentStructIdentifier) {
                Node = AllocNodeAt<types::ClassDefinitionNode>(SourceStart, ClassIdentifier, ParentStructIdentifier, Body);
            } else {
                Node = AllocNodeAt<types::ClassDefinitionNode>(SourceStart, ClassIdentifier, Body);
            }

            ExitScope();

            return Node;
        }
    }
}
```

## `src/parser/compoundStatement.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/enums.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/union.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>


namespace baka
{
	namespace parser
	{
		bool Parser::TryParseLocalAggregate(types::CompoundStatementNode* Node)
		{
			if (!AtAggregateDefinition()) return false;

			switch (Peek().TokenType_v) {
			case types::TokenType::K_STRUCT: {
				auto Parsed = ParseStruct();
				if (std::holds_alternative<types::StructDefinitionNode*>(Parsed)) {
					Node->addStatement(std::get<types::StructDefinitionNode*>(Parsed));
				} else {
					Node->addStatement(std::get<types::StructDeclarationNode*>(Parsed));
				}
				return true;
			}
			case types::TokenType::K_CLASS: {
				auto Parsed = ParseClass();
				if (std::holds_alternative<types::ClassDefinitionNode*>(Parsed)) {
					Node->addStatement(std::get<types::ClassDefinitionNode*>(Parsed));
				} else {
					Node->addStatement(std::get<types::ClassDeclarationNode*>(Parsed));
				}
				return true;
			}
			case types::TokenType::K_UNION:
				Node->addStatement(ParseUnion());
				return true;
			case types::TokenType::K_ENUM:
				Node->addStatement(ParseEnumDecl());
				return true;
			default:
				return false;
			}
		}

		types::CompoundStatementNode* Parser::CompoundStatement()
		{
			const size_t SourceStart = current;
			if (!Match(types::TokenType::LPAREN_CURLY))
			{
				ReportError("expected '{'");
			}

			EnterScope();

			types::CompoundStatementNode::StatementList statements;

			while (Peek().TokenType_v != types::TokenType::RPAREN_CURLY && !Check(types::TokenType::EOF_TOKEN))
			{
				if (AtAggregateDefinition()) {
					auto* Scratch = AllocNodeAt<types::CompoundStatementNode>(SourceStart,
					    types::CompoundStatementNode::StatementList{});
					if (TryParseLocalAggregate(Scratch)) {
						for (auto& Statement : Scratch->getStatements()) statements.push_back(Statement);
						continue;
					}
				}

				if (Check(types::TokenType::K_TYPEDEF)) {
					auto* TypedefNode = ParseTypedef();
					statements.emplace_back(TypedefNode);
				} else if (AtDeclarationStart()) {
					if (auto PossibleFunction = TryParseFunction()) {
						statements.emplace_back(PossibleFunction.value());
					} else if (Check(types::TokenType::EOF_TOKEN) || Check(types::TokenType::RPAREN_CURLY)) {
						// TryParseFunction consumed the rest while bailing out of a broken probe.
						// Nothing left to parse as a declaration here.
					} else {
						auto* DeclarationListNode = ParseDeclarationList();
						statements.emplace_back(DeclarationListNode);
					}
				}
				else
				{
					auto* StatementNode = ParseStatement();
					statements.emplace_back(StatementNode);
				}
			}

			if (!Match(types::TokenType::RPAREN_CURLY))
			{
				ReportError("expected '}'");
			}

			ExitScope();

			return AllocNodeAt<types::CompoundStatementNode>(SourceStart, std::move(statements));
		}

	}
}
```

## `src/parser/constructor.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ConstructorNode* Parser::ParseConstructor(types::IdentifierNode* ParentName) {
            const size_t SourceStart = current;
            types::IdentifierNode* ConstructorIdentifier = this->ParseIdentifier();
            if (ConstructorIdentifier->GetName() != ParentName->GetName()) {
                ReportError("constructor name does not match the type name");
            }

            // is ctor already checks for LPARENROUND so we dont need to sync here
            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after constructor name");
            }

            types::FunctionParameterList* Args = this->ParseFunctionParameterList();
            if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected ')' after constructor parameter list");
                if (SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::RPAREN_ROUND) {
                    Advance();
                }
            }

            types::StatementNode* Body = this->CompoundStatement();
            auto* Node = AllocNodeAt<types::ConstructorNode>(SourceStart, Args, Body);

            return Node;
        }
    }
}
```

## `src/parser/declaration.cc`

```cpp
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <variant>


namespace baka
{
	namespace parser
	{
		// static int a,*b,*c[10]
		//
		//
		// declaration        := 'static'? 'const'? type-specifier declarator-list ';'
		// type-specifier      := 'int' | 'char' | ... | 'struct' IDENTIFIER
		// declarator-list     := declarator (',' declarator)*
		// declarator          := '*'* IDENTIFIER ('[' expr? ']')* ('=' initializer)?
		// initializer         := expr | '{' expr (',' expr)* '}'


		// TODO: stuff left to support
		// function ptr

		// bare struct - not gonna support


		struct X {};
		unsigned  x = 1;
		// char
		// int

		types::TypeSpecifierModifier Parser::DetermineTypeSpecifierModifier() const {
			switch (Peek().TokenType_v) {
			case types::TokenType::K_UNSIGNED:
				return types::TypeSpecifierModifier::UNSIGNED;
			case types::TokenType::K_STRUCT:
				return types::TypeSpecifierModifier::STRUCT;
			case types::TokenType::K_CLASS:
				return types::TypeSpecifierModifier::CLASS;
			case types::TokenType::K_ENUM:
				return types::TypeSpecifierModifier::ENUM;
			case types::TokenType::K_UNION:
				return types::TypeSpecifierModifier::UNION;
			default:
				return types::TypeSpecifierModifier::NONE;
			}
		}

		// wherever declaration list is typedef also must be there
		types::DeclarationList* Parser::ParseDeclarationList()
		{
			const size_t SourceStart = current;
		    types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;

			bool IsStatic = false;
			bool IsConst = false;

			while (Check(types::TokenType::K_STATIC) || Check(types::TokenType::K_CONST))
			{
				if (Match(types::TokenType::K_STATIC))
				{
					if (IsStatic) {
						ReportError("duplicate 'static' specifier");
					}
					IsStatic = true;
				}
				if (Match(types::TokenType::K_CONST))
				{
					if (IsConst) {
						ReportError("duplicate 'const' specifier");
					}
					IsConst = true;
				}
			}

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			types::IdentifierNode* TypeName = ParseTypeIdentifier();

			if (Modifier == types::TypeSpecifierModifier::STRUCT || Modifier == types::TypeSpecifierModifier::CLASS) {
				if (auto* Entry = this->GetParserSTE(TypeName); Entry && !Entry->IsStruct()) {
					ReportError("not a struct");
				}
			}

			std::vector<types::SingleDeclarationNode*> Declarations;
			auto* Node = ParseSingleDeclaration();
			if(Node == nullptr) {
				ReportError("expected single declaration");
				Node = AllocNodeAt<types::SingleDeclarationNode>(SourceStart,
				    AllocNodeAt<types::DeclarationIdentifierNode>(SourceStart), nullptr);
				Node->setHasError();
			}

			Declarations.push_back(Node);

			while (Match(types::TokenType::OP_COMMA))
			{
				auto* NextNode = ParseSingleDeclaration();
				Declarations.push_back(NextNode);
			}

			if (!Match(types::TokenType::SEMICOLON) && !Check(types::TokenType::EOF_TOKEN))
			{
				// e.g. "int a b = 2;" — a second identifier where ',' or ';' was expected
				ReportError("expected ',' or ';' in declaration", true);
				if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
					Advance();
				}
			}

			auto* DeclarationListNode = AllocNodeAt<types::DeclarationList>(SourceStart, IsStatic, IsConst, Modifier, TypeName, std::move(Declarations));
			return DeclarationListNode;
		}

		// int * ((***a[2])[4])[5];
		// int a = {1}, b;
		types::SingleDeclarationNode* Parser::ParseSingleDeclaration()
		{
			const size_t SourceStart = current;
			auto* DeclarationIdentifer = ParseDeclarationIdentifier();
			if(DeclarationIdentifer == nullptr) {
				ReportError("expected declaration identifier");
				DeclarationIdentifer = AllocNodeAt<types::DeclarationIdentifierNode>(SourceStart);
				DeclarationIdentifer->setHasError();
			}

			types::ExpressionNode* Initialization = nullptr;
			if (Match(types::TokenType::OP_ASSIGN))
			{
				Initialization = ParseInitializer();
				// Intializer is either : assignment_expr, { intializer, ... }
			}

			auto* Node = AllocNodeAt<types::SingleDeclarationNode>(SourceStart, DeclarationIdentifer, Initialization);
			return Node;
		}


		// TODO : write function declaration parser
		// AllowRedefine : if true allows redefining the same identifier (useful for typedef that lets u redefine the same name)
		// false by default
		types::DeclarationIdentifierNode* Parser::ParseDeclarationIdentifier()
		{
			const size_t SourceStart = current;
			auto* Variable = AllocNodeAt<types::DeclarationIdentifierNode>(SourceStart);
			while (Match(types::TokenType::OP_MUL))
			{
			    Variable->appendPointer();
			}

			// not supporting const pointer
			// int* const ptr - not supported

			if (Match(types::TokenType::LPAREN_ROUND))
			{
				auto* InnerDeclaration = ParseDeclarationIdentifier();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}


				Variable->setInnerDeclaration(InnerDeclaration);
			}
			else
			{
				if (!Check(types::TokenType::IDENTIFIER))
				{
					std::string_view ErrName = "?";
					if (std::holds_alternative<std::string_view>(Peek().Value)) {
						ErrName = std::get<std::string_view>(Peek().Value);
					}
					ReportError("expected identifier");
					if (!Check(types::TokenType::EOF_TOKEN)) {
						Advance();
					}
					auto* Errored = AllocNodeAt<types::IdentifierNode>(SourceStart, ErrName);
					Errored->setHasError();
					Variable->setInnerDeclaration(Errored);
				}
				else
				{
					auto* Identifier = ParseIdentifier();

					Variable->setInnerDeclaration(Identifier);
				}

			}

			// check for function
			// bind inner declaration to function ptr;
			// cases:
			// if inner decl = function ptr, bind to its return type
			// if inner decl = identifier, bind to it
			// if inner decl = function definition, bind to its return type - NOT POSSIBLE
			// if inner decl = pointer type,
			//

			// int (*(*foo[2])())();
			if(Match(types::TokenType::LPAREN_ROUND)) {
				auto* ParameterList = ParseFunctionParameterList();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}
				Variable->setFunctionParameters(ParameterList);
			}

			// int (*foo[20])()[];
			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				if (Match(types::TokenType::RPAREN_SQUARE)) {
                    Variable->appendArraySize(nullptr);
                    continue;
                }
                types::ExpressionNode* ArraySize = ParseAssignmentExpression();
				if (ArraySize == nullptr) {
				    // "expected expression" already reported deep; resync to the ']'
					SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
					Match(types::TokenType::RPAREN_SQUARE);
				} else {
					Variable->appendArraySize(ArraySize);
					if (!Match(types::TokenType::RPAREN_SQUARE))
					{
						ReportError("expected ']'");
						SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
						Match(types::TokenType::RPAREN_SQUARE);
					}
				}
			}


			return Variable;
		}

		types::DeclarationIdentifierNode* Parser::ParseAbstractDeclarator()
		{
			const size_t SourceStart = current;
			auto* Variable = AllocNodeAt<types::DeclarationIdentifierNode>(SourceStart);
			while (Match(types::TokenType::OP_MUL))
			{
			    Variable->appendPointer();
			}
			// not supporting const pointer
			// int* const ptr - not supported

			if (Check(types::TokenType::LPAREN_ROUND) && Check2(types::TokenType::OP_MUL))
			{
				Advance();
				auto* InnerDeclaration = ParseAbstractDeclarator();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}

				Variable->setInnerDeclaration(InnerDeclaration);
			}

			if(Match(types::TokenType::LPAREN_ROUND)) {
				auto* ParameterList = ParseFunctionParameterList();
				if(!Match(types::TokenType::RPAREN_ROUND))
				{
					ReportError("expected ')'");
					SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				}
				Variable->setFunctionParameters(ParameterList);
			}


			while (Match(types::TokenType::LPAREN_SQUARE))
			{
				if (Match(types::TokenType::RPAREN_SQUARE)) {
                    Variable->appendArraySize(nullptr);
                    continue;
                }
                types::ExpressionNode* ArraySize = ParseAssignmentExpression();
				if (ArraySize == nullptr) {
				    // "expected expression" already reported deep; resync to the ']'
					SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
				} else {
					Variable->appendArraySize(ArraySize);
					if (!Match(types::TokenType::RPAREN_SQUARE))
					{
						ReportError("expected ']'");
						SkipTo({types::TokenType::RPAREN_SQUARE, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
					}
				}
				Match(types::TokenType::RPAREN_SQUARE);
			}

			return Variable;
		}
	}
}
```

## `src/parser/destructor.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/statement.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::DestructorNode* Parser::ParseDestructor(types::IdentifierNode* ParentName) {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::OP_TILDE)) {
                assert(false);
                // throw error
            }

            types::IdentifierNode* DestructorIdentifier = nullptr;
            if(!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after '~'");
            } else {
                DestructorIdentifier = this->ParseIdentifier();
            }
            if(DestructorIdentifier && DestructorIdentifier->GetName() != ParentName->GetName()) {
                ReportError("destructor name does not match the type name");
            }

            if (!this->Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after destructor name");
                if (!Check(types::TokenType::LPAREN_CURLY) &&
                    SkipTo({types::TokenType::LPAREN_CURLY, types::TokenType::SEMICOLON}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            } else if (!this->Match(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected ')' after destructor parameter list");
                if (!Check(types::TokenType::LPAREN_CURLY) &&
                    SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::RPAREN_ROUND) {
                    Advance();
                }
            }

            types::StatementNode* Body = nullptr;
            if (Check(types::TokenType::LPAREN_CURLY)) {
                Body = this->CompoundStatement();
            } else {
                ReportError("expected '{' for destructor body");
                types::CompoundStatementNode::StatementList EmptyBody;
                Body = AllocNodeAt<types::CompoundStatementNode>(SourceStart, std::move(EmptyBody));
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }
            auto* Node = AllocNodeAt<types::DestructorNode>(SourceStart, Body);

            return Node;
        }
    }
}
```

## `src/parser/enums.cc`

```cpp
#include "types/parser/ast/enums.hh"
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
namespace parser {

    types::EnumNode* Parser::ParseEnumDecl() {
        const size_t SourceStart = current;
        if(!Match(types::TokenType::K_ENUM)) {
            // todo throw error
            assert(false);
        }

        types::IdentifierNode* EnumName = nullptr;
        if(!Check(types::TokenType::LPAREN_CURLY)) { // supporting anonymous enums
            if(!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier");
            } else {
                EnumName = ParseIdentifier();
                // enums cannot have fwd decl btw
                if(TypeLookup.LookupCurrent(EnumName->GetName()).has_value()) {
                    ReportError("redefinition of existing type");
                }
                else AddType(EnumName);
            }
        }

        types::EnumNode* Enum = AllocNodeAt<types::EnumNode>(SourceStart, EnumName);

        if(!Match(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after enum name");
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
            return Enum;
        }

        do {
            if(!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier in enumeration");
                SkipTo({types::TokenType::OP_COMMA, types::TokenType::RPAREN_CURLY});
            } else {
                types::IdentifierNode* EnumValue = ParseIdentifier();
                types::ExpressionNode* value = nullptr;
                if (Match(types::TokenType::OP_ASSIGN)) value = ParseAssignmentExpression();
                Enum->addEnumValue(EnumValue, value);
            }

            if(!Match(types::TokenType::OP_COMMA)) {
                if (!Match(types::TokenType::RPAREN_CURLY)) {
                    ReportError("expected '}' or ',' in enumeration");
                    SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                }
                break;
            }

        } while(!Match(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN));

        if(!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after enum definition", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        return Enum;
    }

}
}
```

## `src/parser/expressionStatement.cc`

```cpp
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/statement.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
namespace parser {

    types::ExpressionStatementNode* Parser::ParseExpressionStatement() {
        const size_t SourceStart = current;
        types::ExpressionNode* Expression = this->Expression();
        if (!Match(types::TokenType::SEMICOLON)) {
            if (Expression != nullptr) {
                ReportError("expected ';' after expression", true);
            }
            // expression error was already reported deeper; resync to the end of the doomed statement
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        types::ExpressionStatementNode* Node = AllocNodeAt<types::ExpressionStatementNode>(SourceStart, Expression);
        if (Expression == nullptr) {
            Node->setHasError();
        }
        return Node;
    }
}
}
```

## `src/parser/expressions.cc`

```cpp

#include "parser.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/operators.hh"
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/this.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>
#include <variant>


namespace baka {

namespace parser {

// parse_factor():
//     if next token is unop:
//         consume unop
//         return Unary(op, parse_factor())
//     else:
//         return parse_postfix_exp()
//
// parse_postfix_exp():
//     expr = parse_primary_exp()      // int, identifier, "(" expr ")"
//     while next token is '++', '--', '.', '->', '[':
//         ... wrap expr accordingly ...
//     return expr
//
// parse_primary_exp():
//     // just the int/identifier/paren cases, factored out of the old parse_factor

//
//   parse_expr         (binary ops: + - * / etc, via precedence climbing)
//     -> parse_factor      (unary ops: - ! ~)
//         -> parse_postfix_exp   (postfix: ++ -- . -> [ ( )
//             -> parse_primary_exp   (atoms: constants, identifiers, "(...)")
//         -> try_parse_typename (on finding "(")


    types::ExpressionNode* Parser::ParsePrimaryExpression() {
        const size_t SourceStart = current;
        auto& ConstantOrIdentifierOrThis = Peek();


        if(Match(types::TokenType::LPAREN_ROUND)) {
            types::ExpressionNode* Node = ParseCommaExpression();
            if(!Match(types::TokenType::RPAREN_ROUND)) {
                if (Node) ReportError("expected ')'");
            }
            return Node;
        }

        if (detail::isConstantToken(ConstantOrIdentifierOrThis.TokenType_v)) {
            auto& Constant = ConstantOrIdentifierOrThis;

            auto* ConstantNode =  ParseConstantNode();
            Advance();

            return AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, ConstantNode);

        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::IDENTIFIER) {
            auto& Identifier = ConstantOrIdentifierOrThis;
            Advance();

            assert(std::holds_alternative<std::string_view>(Identifier.Value));
            auto* IdentifierNode = AllocNodeAt<types::IdentifierNode>(SourceStart, std::get<std::string_view>(Identifier.Value));

            auto* PrimaryExpressionNode = AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, IdentifierNode);

            return PrimaryExpressionNode;
        } else if(ConstantOrIdentifierOrThis.TokenType_v == types::TokenType::K_THIS) {
            Advance();

            auto* ThisNode = AllocNodeAt<types::ThisNode>(SourceStart);
            auto* PrimaryExpressionNode = AllocNodeAt<types::PrimaryExpressionNode>(SourceStart, ThisNode);
            return PrimaryExpressionNode;
        } else {
            ReportError("expected expression");
            return nullptr;
        }

        return nullptr;
    }


    types::ExpressionNode* Parser::ParseFactor() {
        const size_t SourceStart = current;

        if (Check(types::TokenType::LPAREN_ROUND))
        {
            // try parsing contents as a type name for c style cast
            size_t prev = current;
            Advance();

            auto TypeNameOpt = TryParseTypeName();
            if (TypeNameOpt.has_value())
            {
                auto Declarator = ParseAbstractDeclarator();
                if (!Match(types::TokenType::RPAREN_ROUND))
                {
                    ReportError("expected ')'");
                }
                auto CastExpression = ParseFactor();
                return AllocNodeAt<types::CastFactorNode>(SourceStart, TypeNameOpt.value(), Declarator, CastExpression);
            }

            current = prev;
        }

        if (detail::IsUnaryOperator(Peek().TokenType_v)) {
            types::ASTUnaryOp UnaryOp = types::TokenTypeToASTUnaryOp[Peek().TokenType_v];

            if(UnaryOp == types::ASTUnaryOp::OP_SIZEOF) {
                return ParseSizeofExpression();
            } else {
                Advance();
                types::ExpressionNode* Expr = ParseFactor();
                if (Expr == nullptr) return nullptr;
                return AllocNodeAt<types::FactorNode>(SourceStart, UnaryOp, Expr);
            }
        }

        return ParsePostfixExpression();
    }

    // for ++, -- , [], ->, .
    types::ExpressionNode* Parser::ParsePostfixExpression() {
        const size_t SourceStart = current;
        types::ExpressionNode* Left = ParsePrimaryExpression(); // can be int, (expr), constants, identifier
        if (Left == nullptr) return nullptr;
        // primary expr because cannot be something like 'a-- --' or 'a--[]' - need be brakcetted

        auto isPostfixExprOp = [](types::TokenType TokenType_v) -> bool {
            switch(TokenType_v) {
                case types::TokenType::OP_INC:
                case types::TokenType::OP_DEC:
                case types::TokenType::OP_DOT:
                case types::TokenType::OP_ARROW:
                case types::TokenType::LPAREN_SQUARE:
                case types::TokenType::LPAREN_ROUND:
                    return true;
                default:
                    return false;
            }
        };

        while(isPostfixExprOp(Peek().TokenType_v)) {
            const size_t OperatorStart = current;
            types::TokenType Op = Advance().TokenType_v;

            if(Op == types::TokenType::LPAREN_SQUARE) {

                types::ExpressionNode* Index = ParseCommaExpression();
                if (Index == nullptr) return nullptr;
                Left = AllocNodeAt<types::IndexPostfixExpr>(OperatorStart, Left, Index);

                if(!Match(types::TokenType::RPAREN_SQUARE)) {
                    ReportError("expected ']'");
                }
            } else if(Op == types::TokenType::OP_INC || Op == types::TokenType::OP_DEC) {


                Left = AllocNodeAt<types::UnaryPostfixExpr>(OperatorStart, Left, types::TokenTypeToASTUnaryOp.at(Op));

            } else if(Op == types::TokenType::OP_DOT) {
                // (expr).Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after '.'");
                    return nullptr;
                }

                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                auto* IdentifierNode = AllocNodeAt<types::IdentifierNode>(current - 1, std::get<std::string_view>(Identifier.Value));

                Left = AllocNodeAt<types::MemberPostfixExpr>(OperatorStart, Left, IdentifierNode);

            } else if(Op == types::TokenType::OP_ARROW) {
                // (expr)->Identifier

                if(!Check(types::TokenType::IDENTIFIER)) {
                    ReportError("expected identifier after '->'");
                    return nullptr;
                }
                auto& Identifier = Advance();
                assert(std::holds_alternative<std::string_view>(Identifier.Value) && "identifier doesnt have string view");
                types::IdentifierNode* IdentifierNode = AllocNodeAt<types::IdentifierNode>(current - 1, std::get<std::string_view>(Identifier.Value));

                Left = AllocNodeAt<types::ArrowPostfixExpr>(OperatorStart, Left, IdentifierNode);
            } else {
                // Identifier(expr) expr will have commas
                // (2+2)(1,2) will allow this

                types::ExpressionNode* ArgsList = nullptr;
                if (!Check(types::TokenType::RPAREN_ROUND)) {
                    ArgsList = ParseCommaExpression();
                    if (ArgsList == nullptr) return nullptr;
                } else {
                    ArgsList = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::vector<types::ExpressionNode*>{});
                }

                if(!Match(types::TokenType::RPAREN_ROUND)) {
                    ReportError("expected ')'");
                }

                Left = AllocNodeAt<types::FunctionCallPostfixExpr>(OperatorStart, Left, ArgsList);
            }
        }

        return Left;
    }

    types::ExpressionNode* Parser::ParseAssignmentExpression(size_t MinPrecedence)
    {
        const size_t SourceStart = current;
        auto* LeftFactor = ParseFactor();
        if (LeftFactor == nullptr) return nullptr;

        auto Op = Peek().TokenType_v;
        while (IsBinaryOperator(Op) && GetPrecedence(Op) >= MinPrecedence && Op != types::TokenType::OP_COMMA) {
            const size_t OperatorStart = current;
            Advance();
            if (IsRightToLeft(Op))
            {
                if (Op == types::TokenType::OP_QUESTION) {
                    // The middle operand is a full expression; an unparenthesized comma belongs to it.
                    auto* ThenExpr = ParseAssignmentExpression();
                    if (ThenExpr && Check(types::TokenType::OP_COMMA)) {
                        std::vector<types::ExpressionNode*> Middle{ThenExpr};
                        while (Match(types::TokenType::OP_COMMA)) {
                            auto* Next = ParseAssignmentExpression();
                            if (!Next) return nullptr;
                            Middle.push_back(Next);
                        }
                        ThenExpr = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Middle));
                    }
                    if (ThenExpr == nullptr) return nullptr;
                    if (!Match(types::TokenType::OP_COLON)) {
                        ReportError("expected ':' in ternary expression");
                    }

                    auto* ElseExpr = ParseAssignmentExpression();
                    if (ElseExpr == nullptr) return nullptr;

                    LeftFactor = AllocNodeAt<types::TernaryOpNode>(OperatorStart, LeftFactor, ThenExpr, ElseExpr);
                } else {
                    auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op));
                    if (RightFactor == nullptr) return nullptr;
                    LeftFactor = AllocNodeAt<types::BinaryExpressionNode>(OperatorStart, types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
                }
            }
            else
            {
                auto* RightFactor = ParseAssignmentExpression(GetPrecedence(Op) + 1);
                if (RightFactor == nullptr) return nullptr;
                LeftFactor = AllocNodeAt<types::BinaryExpressionNode>(OperatorStart, types::TokenTypeToASTBinaryOp.at(Op), LeftFactor, RightFactor);
            }
            Op = Peek().TokenType_v;
        }

        return LeftFactor;
    }

    types::ExpressionNode* Parser::ParseCommaExpression()
    {
        const size_t SourceStart = current;
        std::vector<types::ExpressionNode*> Args;
        auto* LeftFactor = ParseAssignmentExpression();
        if (LeftFactor == nullptr) return nullptr;

        Args.push_back(LeftFactor);
        while (Match(types::TokenType::OP_COMMA)) {
            auto* RightFactor = ParseAssignmentExpression();
            if (RightFactor == nullptr) return nullptr;
            Args.push_back(RightFactor);
        }
        return AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Args));
    }

}
}
```

## `src/parser/function.cc`

```cpp
#include "types/parser/ast/function.hh"
#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/identifier.hh"
#include "types/parser/ast/statement.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <optional>

// A declaration of a parameter as "function returning type" shall be adjusted to "pointer to function returning type"
// void x(int x, int foo() = 0) {
// }

namespace baka
{
	namespace parser
	{
		types::FunctionParameter* Parser::ParseFunctionParameter() {
			const size_t SourceStart = current;

			TypedefFlagGuard ScopedInTypedef(*this, false);

			types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;

			bool IsConst = false;
			if (Match(types::TokenType::K_CONST)) {
				IsConst = true;
			}

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			if (!isTypeName(this->Peek())) {
				ReportError("expected type name");
				auto* ErroredParam = AllocNodeAt<types::FunctionParameter>(SourceStart, IsConst, Modifier, nullptr, nullptr);
				ErroredParam->setHasError();
				return ErroredParam;
			}
			types::IdentifierNode* TypeName = ParseTypeIdentifier();


			types::SingleDeclarationNode* Node = ParseSingleDeclaration();

			// TODO: fix these disgusting constructors

			auto* Parameter = AllocNodeAt<types::FunctionParameter>(SourceStart, IsConst, Modifier, TypeName, Node);
			return Parameter;
		}

		types::FunctionParameterList* Parser::ParseFunctionParameterList()
		{
			const size_t SourceStart = current;
			// parses <functionparam>, <functionparam> , ...

			std::vector<types::FunctionParameter*> Decls;
			bool Variadic = false;

			if (Check(types::TokenType::RPAREN_ROUND))
			{
				// empty parameter list
				return AllocNodeAt<types::FunctionParameterList>(SourceStart, Decls, Variadic);
			}

			bool DefaultRunning = false;
			do
			{
				if (this->Match(types::TokenType::OP_ELLIPSIS))
				{
					Variadic = true;
					break;
				}

				auto* FunctionParam = ParseFunctionParameter();

				if (FunctionParam->hasError())
				{
					// the errored param consumed nothing; resync to the next separator so the
					// enclosing declarator still sees a balanced parenthesized list
					SkipTo({types::TokenType::OP_COMMA, types::TokenType::RPAREN_ROUND});
				}

				if (FunctionParam->hasInitalizer())
				{
					DefaultRunning = true;
				}
				else if (DefaultRunning)
				{
					// default should be at the end
					ReportError("default argument must be at the end of the parameter list");
				}

				Decls.push_back(FunctionParam);
			}
			while (this->Match(types::TokenType::OP_COMMA));

			return AllocNodeAt<types::FunctionParameterList>(SourceStart, Decls, Variadic);
		}

		std::optional<types::FunctionNode*> Parser::TryParseFunction()
		{
			const size_t SourceStart = current;
			size_t prev = current;
			size_t ErrorsOnEntry = ReportedErrorCount;

			types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;

			bool IsStatic = false;
			bool IsConst = false;

			while (Check(types::TokenType::K_STATIC) || Check(types::TokenType::K_CONST))
			{
				if (Match(types::TokenType::K_STATIC))
				{
					if (IsStatic) {
						ReportError("duplicate 'static' specifier");
					}
					IsStatic = true;
				}
				if (Match(types::TokenType::K_CONST))
				{
					if (IsConst) {
						ReportError("duplicate 'const' specifier");
					}
					IsConst = true;
				}
			}

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			if (!isTypeName(this->Peek()))
			{
				// not a plausible function/declaration start (e.g. a stray 'if' at top level)
				std::string_view Name = "?";
				if (std::holds_alternative<std::string_view>(Peek().Value)) {
					Name = std::get<std::string_view>(Peek().Value);
				}
				if (Check(types::TokenType::IDENTIFIER)) {
					ReportError("unknown type name '" + std::string(Name) + "'");
				} else {
					ReportError("expected identifier or '('");
				}
				if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
					Advance();
				}
				return std::nullopt;
			}

            types::IdentifierNode* ReturnTypeBindedIdentifier = this->ParseTypeIdentifier();

			// int (*foo())();

			// TODO: declaration identifer should deal with function
			// single declaration can take a default init issok
			// in single declaration if its declared then put in symbol table and mark as not declared (not defined = true)

			types::DeclarationIdentifierNode* FunctionTypeDecl = this->ParseDeclarationIdentifier();
			if (!FunctionTypeDecl)
			{
				ReportError("expected declaration identifier after return type");
				return std::nullopt;
			}

			if (!Check(types::TokenType::LPAREN_CURLY))
			{
				if (ReportedErrorCount != ErrorsOnEntry)
				{
					// the probe already reported an error; rolling back would re-parse the
					// broken declaration and report it again. resync to the end instead.
					if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
						Advance();
					}
					return std::nullopt;
				}
				current = prev;
				return std::nullopt;
			}

			// function body
			types::StatementNode* Body = this->CompoundStatement();

			return AllocNodeAt<types::FunctionNode>(SourceStart, ReturnTypeBindedIdentifier, FunctionTypeDecl, Body, IsStatic,
			                                           IsConst, Modifier);
		}
	}
}
```

## `src/parser/identifier.cc`

```cpp
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
    namespace parser {
        types::IdentifierNode* Parser::ParseIdentifier() {
            const size_t SourceStart = current;
            if (!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier");
                if (!Check(types::TokenType::EOF_TOKEN)) {
                    Advance();
                }
                auto* Errored = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?"));
                Errored->setHasError();
                return Errored;
            }
            return AllocNodeAt<types::IdentifierNode>(SourceStart, std::get<std::string_view>(this->Advance().Value));
        }
    }
}
```

## `src/parser/initializer.cc`

```cpp
#include "types/parser/ast/initializer.hh"
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
namespace parser {


    types::InitializerNode* Parser::ParseInitializer() {
        const size_t SourceStart = current;
        auto* IntializerNodeRet = AllocNodeAt<types::InitializerNode>(SourceStart);

        // struct X {
        //     int a;
        //     int b;
        // };
        // X b = {1,2,}; // trailing comma

        if(Match(types::TokenType::LPAREN_CURLY)) {
            IntializerNodeRet->setBraced();
            if(Match(types::TokenType::RPAREN_CURLY)) {
                return IntializerNodeRet; // default intializer;
            }

            IntializerNodeRet->addExpression(ParseInitializer());
            while(Match(types::TokenType::OP_COMMA)) {
                if(Check(types::TokenType::RPAREN_CURLY)) break;

                IntializerNodeRet->addExpression(ParseInitializer());
            }

            if(!Match(types::TokenType::RPAREN_CURLY)) {
                ReportError("expected '}'");
                auto Sync = SkipTo({types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_CURLY) {
                    Advance();
                } else if (Sync == types::TokenType::SEMICOLON){
                    Advance();
                }
            }

        } else {
            auto* Expr = ParseAssignmentExpression();
            if (Expr) {
                IntializerNodeRet->addExpression(Expr);
            }
        }

        return IntializerNodeRet;
    }

}
}
```

## `src/parser/iterationStatement.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typedef.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <variant>

namespace baka {
    namespace parser {

        // A loop suffix is ';' IDENTIFIER, immediately before the closing ')'.
        // It is never a runtime expression: `while (x; label)` and `do { } while (x; label);`.
        types::IdentifierNode* Parser::ParseLoopSuffixLabel() {
            if (!Match(types::TokenType::SEMICOLON)) return nullptr;
            if (!Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected loop label identifier after ';'");
                return nullptr;
            }
            auto* Label = ParseIdentifier();
            if (Label && LookupType(Label)) {
                ReportError("identifier cannot be a type name");
            }
            return Label;
        }

        types::ForBlockStatementNode* Parser::ForBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_FOR)) {
                assert(false);
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'for'");
                Errored = true;
            }

            EnterScope();

            // init-clause ';' condition? ';' update? (';' label)? ')'
            std::variant<types::ExpressionNode*, types::DeclarationList*, types::TypedefNode*> Decl;

            if (Check(types::TokenType::SEMICOLON)) {
                Advance();
            } else if (detail::IsSpecifier(Peek().TokenType_v) || isTypeName(Peek())) {
                Decl = this->ParseDeclarationList();
            } else if (Check(types::TokenType::K_TYPEDEF)) {
                Decl = this->ParseTypedef();
            } else {
                auto* InitExpr = this->Expression();
                if (InitExpr == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                } else {
                    Decl = InitExpr;
                }
                if (!Match(types::TokenType::SEMICOLON)) {
                    ReportError("expected ';' in 'for' statement specifier");
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                    Match(types::TokenType::SEMICOLON);
                }
            }

            types::ExpressionNode* ComparisonExpr = nullptr;
            if (!Check(types::TokenType::SEMICOLON)) {
                ComparisonExpr = Expression();
                if (ComparisonExpr == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' in 'for' statement specifier");
                Errored = true;
                SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                Match(types::TokenType::SEMICOLON);
            }

            types::ExpressionNode* Update = nullptr;
            if (!Check(types::TokenType::RPAREN_ROUND) && !Check(types::TokenType::SEMICOLON)) {
                Update = Expression();
                if (Update == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_ROUND, types::TokenType::RPAREN_CURLY});
                }
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            auto* Node = AllocNodeAt<types::ForBlockStatementNode>(SourceStart, Decl, ComparisonExpr, Update, Label, Body);
            if (Errored) {
                Node->setHasError();
            }

            ExitScope();

            return Node;
        }

        types::WhileBlockStatementNode* Parser::WhileBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_WHILE)) {
                assert(false);
            }

            bool Errored = false;
            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'while'");
                Errored = true;
            }

            types::ExpressionNode* Cond = this->Expression();
            if (Cond == nullptr) {
                Errored = true;
                SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();
            auto* Node = AllocNodeAt<types::WhileBlockStatementNode>(SourceStart, Cond, Label, Body);
            if (Errored) {
                Node->setHasError();
            }
            return Node;
        }

        types::DoWhileBlockStatementNode* Parser::DoWhileBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_DO)) {
                assert(false);
            }

            types::StatementNode* Body = this->ParseStatement();

            bool Errored = false;
            if (!this->Match(types::TokenType::K_WHILE)) {
                ReportError("expected 'while' after 'do' body");
                Errored = true;
                if (SkipTo({types::TokenType::K_WHILE, types::TokenType::RPAREN_CURLY, types::TokenType::SEMICOLON}) != types::TokenType::K_WHILE) {
                    auto* Bail = AllocNodeAt<types::DoWhileBlockStatementNode>(SourceStart, nullptr, nullptr, Body);
                    Bail->setHasError();
                    return Bail;
                }
                this->Match(types::TokenType::K_WHILE);
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                ReportError("expected '(' after 'while'");
                Errored = true;
            }

            types::ExpressionNode* Cond = nullptr;
            if (Check(types::TokenType::RPAREN_ROUND)) {
                ReportError("expected expression");
                Errored = true;
            } else {
                Cond = this->Expression();
                if (Cond == nullptr) {
                    Errored = true;
                    SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                }
            }

            types::IdentifierNode* Label = nullptr;
            if (!Errored) {
                Label = ParseLoopSuffixLabel();
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (!Errored) {
                    ReportError("expected ')'");
                }
                Errored = true;
            }
            if (!Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after 'do ... while'", true);
                Errored = true;
            }

            auto* Node = AllocNodeAt<types::DoWhileBlockStatementNode>(SourceStart, Cond, Label, Body);
            if (Errored) {
                Node->setHasError();
            }
            return Node;
        }

        types::IterationStatementNode* Parser::IterationStatement() {
            if (this->Check(types::TokenType::K_FOR)) {
                return ForBlockStatement();
            }
            else if (this->Check(types::TokenType::K_WHILE)) {
                return WhileBlockStatement();
            }
            else if (this->Check(types::TokenType::K_DO)) {
                return DoWhileBlockStatement();
            }
            else {
                assert(false);
                return nullptr;
            }
        }
    }
}
```

## `src/parser/jumpStatement.cc`

```cpp
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::ReturnStatementNode* Parser::ReturnStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_RETURN)) {
                // throw error
            }

            types::ExpressionNode* Expression = nullptr;
            if (!Check(types::TokenType::SEMICOLON)) {
                Expression = this->Expression();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after return statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::ReturnStatementNode* Node = AllocNodeAt<types::ReturnStatementNode>(SourceStart, Expression);
            return Node;
        }

        types::GotoStatementNode* Parser::GotoStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_GOTO)) {
                // throw error
            }

            types::IdentifierNode* Label = this->ParseIdentifier();

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after goto statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::GotoStatementNode* Node = AllocNodeAt<types::GotoStatementNode>(SourceStart, Label);
            return Node;
        }

        types::BreakStatementNode* Parser::BreakStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_BREAK)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;

            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after break statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::BreakStatementNode* Node = AllocNodeAt<types::BreakStatementNode>(SourceStart, Label);
            return Node;
        }

        types::ContinueStatementNode* Parser::ContinueStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CONTINUE)) {
                // throw error
            }

            types::IdentifierNode* Label = nullptr;
            if (this->Check(types::TokenType::IDENTIFIER)) {
                Label = this->ParseIdentifier();
            }

            if (!this->Match(types::TokenType::SEMICOLON)) {
                ReportError("expected ';' after continue statement", true);
                if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            types::ContinueStatementNode* Node = AllocNodeAt<types::ContinueStatementNode>(SourceStart, Label);
            return Node;
        }

        types::JumpStatementNode* Parser::JumpStatement() {
            if (this->Check(types::TokenType::K_RETURN)) {
                return ReturnStatement();
            }
            else if (this->Check(types::TokenType::K_GOTO)) {
                return GotoStatement();
            }
            else if (this->Check(types::TokenType::K_BREAK)) {
                return BreakStatement();
            }
            else if (this->Check(types::TokenType::K_CONTINUE)) {
                return ContinueStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
```

## `src/parser/labelStatement.cc`

```cpp
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        // Labels own the statement they prefix. The parser produces the attached body;
        // semantic checking decides whether the label placement is legal.
        types::GotoLabelStatementNode* Parser::GotoLabelStatement() {
            const size_t SourceStart = current;
            types::IdentifierNode* Tag = this->ParseIdentifier();

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after label", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::GotoLabelStatementNode>(SourceStart, Tag, Stmt);
            return Node;
        }

        types::CaseLabelStatementNode* Parser::CaseLabelStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_CASE)) {
                // throw error
            }

            types::ExpressionNode* Tag = this->Expression();
            if (Tag == nullptr) {
                // the expression parser already diagnosed; resync to the ':'.
                SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after case value", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::CaseLabelStatementNode>(SourceStart, Tag, Stmt);
            return Node;
        }

        types::DefaultLabelStatementNode* Parser::DefaultLabelStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_DEFAULT)) {
                // throw error
            }

            if (!this->Match(types::TokenType::OP_COLON)) {
                ReportError("expected ':' after 'default'", true);
                if (SkipTo({types::TokenType::OP_COLON, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::OP_COLON) {
                    Advance();
                }
            }

            types::StatementNode* Stmt = nullptr;
            if (!Check(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN) &&
                !Check(types::TokenType::K_CASE) && !Check(types::TokenType::K_DEFAULT)) {
                Stmt = this->ParseStatement();
            }

            auto* Node = AllocNodeAt<types::DefaultLabelStatementNode>(SourceStart, Stmt);
            return Node;
        }

        types::LabelStatementNode* Parser::LabelStatement() {
            if (this->Check(types::TokenType::IDENTIFIER) && this->Check2(types::TokenType::OP_COLON)) {
                return this->GotoLabelStatement();
            }
            else if (this->Check(types::TokenType::K_CASE)) {
                return this->CaseLabelStatement();
            }
            else if (this->Check(types::TokenType::K_DEFAULT)) {
                return this->DefaultLabelStatement();
            }
            else {
                return nullptr;
            }
        }
    }
}
```

## `src/parser/parser.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/constant.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/program.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include "base/line_index.hh"
#include "driver/gctx.hh"
#include "types/driver/defs.hh"
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <optional>
#include <variant>

// TODO: deal with dtypddefs by mainting symbol table

namespace baka {
    namespace parser {
        bool Parser::Check(types::TokenType type) const noexcept {
            if (current >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

            return Tokens[current].TokenType_v == type;
        }

        bool Parser::Check2(types::TokenType type) const noexcept {
            return CheckN(1, type);
        }

        bool Parser::CheckN(size_t Offset, types::TokenType type) const noexcept {
            if (current + Offset >= Tokens.size()) return (type == types::TokenType::EOF_TOKEN);

            return Tokens[current + Offset].TokenType_v == type;
        }

        bool Parser::AtAggregateDefinition() {
            auto Keyword = Peek().TokenType_v;
            if (Keyword == types::TokenType::K_ENUM) {
                // Enums have no forward declaration in this grammar.
                return (Check2(types::TokenType::IDENTIFIER) && CheckN(2, types::TokenType::LPAREN_CURLY)) ||
                       Check2(types::TokenType::LPAREN_CURLY);
            }
            if (Keyword != types::TokenType::K_STRUCT && Keyword != types::TokenType::K_CLASS &&
                Keyword != types::TokenType::K_UNION) {
                return false;
            }
            // `struct S {` and `struct S;` are aggregate forms; `struct S value;` is a declaration.
            if (Check2(types::TokenType::IDENTIFIER)) {
                return CheckN(2, types::TokenType::LPAREN_CURLY) || CheckN(2, types::TokenType::SEMICOLON) ||
                       CheckN(2, types::TokenType::OP_COLON);
            }
            return Check2(types::TokenType::LPAREN_CURLY);
        }

        const types::Token& Parser::Advance() {
            return Tokens[current++];
        }

        bool Parser::Match(types::TokenType type) {
            if (Check(type)) {
                Advance();
                return true;
            }
            return false;
        }

        const types::Token& Parser::Peek() const noexcept {
            if (current >= Tokens.size()) return Tokens.back();

            return Tokens[current];
        }

        const types::Token& Parser::Previous() const {
            return Tokens[current - 1];
        }


        types::ASTNode* Parser::Parse() {
            return ParseProgram();
        };

        types::ProgramNode* Parser::ParseProgram() {
            const size_t SourceStart = current;
            auto* ProgramNode = AllocNodeAt<types::ProgramNode>(SourceStart);

            while (!Check(types::TokenType::EOF_TOKEN)) {
                // function decl
                // declaration list
                // union decl
                // class decl
                // enum decl
                // typdef

                if (Check(types::TokenType::K_STRUCT) && AtAggregateDefinition()) {
                    auto Struct = this->ParseStruct();
                    if (std::holds_alternative<types::StructDefinitionNode*>(Struct)) {
                        ProgramNode->addNode(std::get<types::StructDefinitionNode*>(Struct));
                    } else {
                        ProgramNode->addNode(std::get<types::StructDeclarationNode*>(Struct));
                    }
                } else if (Check(types::TokenType::K_CLASS) && AtAggregateDefinition()) {
                    auto Class = this->ParseClass();
                    if (std::holds_alternative<types::ClassDefinitionNode*>(Class)) {
                        ProgramNode->addNode(std::get<types::ClassDefinitionNode*>(Class));
                    } else {
                        ProgramNode->addNode(std::get<types::ClassDeclarationNode*>(Class));
                    }
                } else if (Check(types::TokenType::K_UNION) && AtAggregateDefinition()) {
                    auto Union = this->ParseUnion();
                    ProgramNode->addNode(Union);
                } else if (Check(types::TokenType::K_ENUM) && AtAggregateDefinition()) {
                    auto Enum = this->ParseEnumDecl();
                    ProgramNode->addNode(Enum);
                } else if (Check(types::TokenType::K_TYPEDEF)) {
                    auto Typedef = this->ParseTypedef();
                    ProgramNode->addNode(Typedef);
                } else {
                    auto PossibleFunction = TryParseFunction();

                    if(PossibleFunction) {
                        ProgramNode->addNode(PossibleFunction.value());
                    } else {
                        // try parse function could have munched entire file in err path
                        if(!Check(types::TokenType::EOF_TOKEN)) {
                            auto* Node = ParseDeclarationList();
                            ProgramNode->addNode(Node);
                        }
                    }
                }
            }

            return ProgramNode;
        }

        types::ExpressionNode* Parser::Expression() {
            // todo: does this have to be different
            return ParseCommaExpression();
        }

        types::ConstantNode* Parser::ParseConstantNode() {
            const size_t SourceStart = current;
            const types::Token& Token = this->Peek();

            // int
            // string
            // char
            // float
            // double
            // long long
            // unsigned int
            // unsigned long long

            #define CONSTANT_NODE(T) else if(std::holds_alternative<T>(Token.Value)) do { \
                auto* cn = AllocNodeAt<types::ConstantTNode<T>>(SourceStart, std::get<T>(Token.Value)); \
                return cn; } while(0)

            if(false) {}
            CONSTANT_NODE(int);
            CONSTANT_NODE(base::SLString);
            CONSTANT_NODE(char);
            CONSTANT_NODE(std::string);
            CONSTANT_NODE(float);
            CONSTANT_NODE(double);
            CONSTANT_NODE(long long);
            CONSTANT_NODE(unsigned int);
            CONSTANT_NODE(unsigned long long);
            else if (Check(types::TokenType::K_TRUE)) {
                auto* cn = AllocNodeAt<types::ConstantTNode<bool>>(SourceStart, true);
                return cn;
            } else if (Check(types::TokenType::K_FALSE)) {
                auto* cn = AllocNodeAt<types::ConstantTNode<bool>>(SourceStart, false);
                return cn;
            } else if (Check(types::TokenType::K_NULLPTR)) {
                auto* cn = AllocNodeAt<types::ConstantTNode<std::nullptr_t>>(SourceStart);
                return cn;
            }

            ReportError("expected constant");
            auto* cn = AllocNodeAt<types::ConstantTNode<int>>(SourceStart, 0);
            cn->setHasError();
            return cn;
        }

        bool Parser::LookupType(types::IdentifierNode* Identifier) {
            return TypeLookup.Lookup(Identifier->GetName()).has_value();
        }

        bool Parser::LookupType(const std::string_view& IdentifierName) {
            return TypeLookup.Lookup(IdentifierName).has_value();
        }

        ParserSTE* Parser::GetParserSTE(types::IdentifierNode* Identifier) {
            auto* Entry = TypeLookup.GetEntry(Identifier->GetName());
            if (!Entry) {
                ReportError("unknown type name '" + std::string(Identifier->GetName()) + "'");
            }
            return Entry;
        }

        void Parser::EnterScope() {
            TypeLookup.EnterNewScope();
        }

        void Parser::ExitScope() {
            TypeLookup.ExitScope();
        }

        void Parser::AddType(types::IdentifierNode* Identifier) {
            TypeLookup.AddEntry(Identifier->GetName(), ParserSTE{});
        }

        bool Parser::isTypeName(const types::Token& token) {
            if(!std::holds_alternative<std::string_view>(token.Value))
                return false;

            return LookupType(std::get<std::string_view>(token.Value));
        }

        bool Parser::RegisterOrReplaceType(types::IdentifierNode* Identifier) {
            return TypeLookup.RegisterOrReplaceType(Identifier->GetName(), ParserSTE{});
        }

        void Parser::ReportError(std::string Message, bool ReportPreviousToken) {
            ReportedErrorCount++;

            if (TokenSourceLocations.empty()) return;

            size_t idx = current;
            if(ReportPreviousToken) idx--;

            idx = std::clamp(idx, 0uz, TokenSourceLocations.size() - 1);


            const auto& Loc = TokenSourceLocations[idx];
            base::LineCtx LineCtx_v{Loc.LineNo - 1, driver::Gctx::GetLineIndex().get()};
            driver::Gctx::GenerateLineError(Loc.LineNo, Loc.Col, LineCtx_v, std::move(Message), driver::Stage::PARSE, ReportPreviousToken);
        }

        types::TokenType Parser::SkipTo(std::initializer_list<types::TokenType> TokenList) {

            while (!Check(types::TokenType::EOF_TOKEN)) {
                for (auto Sync : TokenList) {
                    if (Check(Sync)) return Sync;
                }

                if (Check(types::TokenType::LPAREN_CURLY)) {
                    int Depth = 0;
                    do {
                        if (Check(types::TokenType::LPAREN_CURLY)) {
                            Depth++;
                        } else if (Check(types::TokenType::RPAREN_CURLY)) {
                            if (--Depth <= 0) {
                                Advance();
                                break;
                            }
                        }
                        Advance();
                    } while (!Check(types::TokenType::EOF_TOKEN));
                    continue;
                }

                Advance();
            }
            return types::TokenType::EOF_TOKEN;
        }

    }
}
```

## `src/parser/selectionStatement.cc`

```cpp
#include "parser.hh"
#include "types/token/token.hh"
#include "utils.hh"

namespace baka {
    namespace parser {
        types::IfSubBlockStatementNode* Parser::IfSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_IF)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                // condition failed; error already reported deep. resync to the closing
                // paren or a semicolon so the offending tokens aren't re-parsed as a body
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            types::IfSubBlockStatementNode* Node = AllocNodeAt<types::IfSubBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::ElseIfSubBlockStatementNode* Parser::ElseIfSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_ELSE)) {
                // throw error
            }
            if (!this->Match(types::TokenType::K_IF)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ElseIfSubBlockStatementNode* Node = AllocNodeAt<types::ElseIfSubBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::ElseSubBlockStatementNode* Parser::ElseSubBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_ELSE)) {
                // throw error
            }

            types::StatementNode* Body = this->ParseStatement();

            types::ElseSubBlockStatementNode* Node = AllocNodeAt<types::ElseSubBlockStatementNode>(SourceStart,  Body);
            return Node;
        }

        types::IfSuperBlockStatementNode* Parser::IfSuperBlockStatement() {
            const size_t SourceStart = current;
            if (!Check(types::TokenType::K_IF)) {
                // throw error
            }
            types::IfSubBlockStatementNode* IfSubBlock = this->IfSubBlockStatement();

            std::vector<types::ElseIfSubBlockStatementNode*> ElseIfSubBlocks;
            while (Check(types::TokenType::K_ELSE) && Check2(types::TokenType::K_IF)) {
                ElseIfSubBlocks.push_back(this->ElseIfSubBlockStatement());
            }

            types::ElseSubBlockStatementNode* ElseSubBlock = nullptr;
            if (Check(types::TokenType::K_ELSE) && !Check2(types::TokenType::K_IF)) {
                ElseSubBlock = this->ElseSubBlockStatement();
            }

            types::IfSuperBlockStatementNode* Node = AllocNodeAt<types::IfSuperBlockStatementNode>(SourceStart, IfSubBlock, ElseIfSubBlocks, ElseSubBlock);
            return Node;
        }

        types::SwitchBlockStatementNode* Parser::SwitchBlockStatement() {
            const size_t SourceStart = current;
            if (!this->Match(types::TokenType::K_SWITCH)) {
                // throw error
            }

            if (!Match(types::TokenType::LPAREN_ROUND)) {
                // throw error
            }

            types::ExpressionNode* Expression = this->Expression();

            if (Expression == nullptr) {
                auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON});
                if (Sync == types::TokenType::RPAREN_ROUND || Sync == types::TokenType::SEMICOLON) {
                    Advance();
                }
            }

            if (!Match(types::TokenType::RPAREN_ROUND)) {
                if (Expression != nullptr) {
                    ReportError("expected ')'");
                }
            }

            // A switch body is any statement; braces are the common case, not a grammar rule.
            types::StatementNode* Body = this->ParseStatement();

            types::SwitchBlockStatementNode* Node = AllocNodeAt<types::SwitchBlockStatementNode>(SourceStart, Expression, Body);
            if (Expression == nullptr) {
                Node->setHasError();
            }
            return Node;
        }

        types::SelectionStatementNode* Parser::SelectionStatement() {
            if (this->Check(types::TokenType::K_IF)) {
                return IfSuperBlockStatement();
            }
            else if (this->Check(types::TokenType::K_SWITCH)) {
                return SwitchBlockStatement();
            }
            else {
                // throw error
                return nullptr;
            }
        }
    }
}
```

## `src/parser/sizeof.cc`

```cpp
#include "types/parser/ast/sizeof.hh"
#include "parser.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/typenode.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>
#include <optional>
#include <variant>



namespace baka {
namespace parser {

types::ExpressionNode* Parser::ParseSizeofExpression() {
    const size_t SourceStart = current;
    if(!Match(types::TokenType::K_SIZEOF)) {
        assert(false);
        // todo throw error
    }

    if(!Match(types::TokenType::LPAREN_ROUND)) {

        types::ExpressionNode* Expr = ParseFactor();
        if(!Expr) {
            // "expected expression" already reported deep
            return nullptr;
        }
        return AllocNodeAt<types::SizeofNode>(SourceStart, Expr);
    }
    // ate bracks

    std::optional<types::TypeNode*> Type = TryParseTypeName();

    std::variant<types::SizeofNode::SizeofTypeVariant, types::ExpressionNode*> Result;
    if(Type) {
        auto* AbstractDecl = ParseAbstractDeclarator();
        Result = types::SizeofNode::SizeofTypeVariant{Type.value(), AbstractDecl};
    } else {
        types::ExpressionNode* Expr = ParseAssignmentExpression();
        if(!Expr) {
            // "expected expression" already reported deep
            return nullptr;
        }
        if (Check(types::TokenType::OP_COMMA)) {
            std::vector<types::ExpressionNode*> Expressions{Expr};
            while (Match(types::TokenType::OP_COMMA)) {
                auto* Next = ParseAssignmentExpression();
                if (!Next) {
                    return nullptr;
                }
                Expressions.push_back(Next);
            }
            Expr = AllocNodeAt<types::CommaExpressionNode>(SourceStart, std::move(Expressions));
        }
        Result = Expr;
    }

    if(!Match(types::TokenType::RPAREN_ROUND)) {
        ReportError("expected ')'");
        auto Sync = SkipTo({types::TokenType::RPAREN_ROUND, types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
        if (Sync != types::TokenType::EOF_TOKEN) {
            Advance();
        }
    }

    return AllocNodeAt<types::SizeofNode>(SourceStart, Result);
}

}
}
```

## `src/parser/statement.cc`

```cpp
#include "types/parser/ast/statement.hh"
#include "parser.hh"
#include "types/token/token.hh"

namespace baka {
    namespace parser {
        types::StatementNodeType Parser::DetermineStatementType() {
            if (Check(types::TokenType::K_IF) || Check(types::TokenType::K_SWITCH)) {
                return types::StatementNodeType::SelectionStatement;
            }
            else if (Check(types::TokenType::K_FOR) || Check(types::TokenType::K_WHILE) || Check(types::TokenType::K_DO)) {
                return types::StatementNodeType::IterationStatement;
            }
            else if ((Check(types::TokenType::IDENTIFIER) && Check2(types::TokenType::OP_COLON)) || Check(types::TokenType::K_CASE) || Check(types::TokenType::K_DEFAULT)) {
                return types::StatementNodeType::LabeledStatement;
            }
            else if (Check(types::TokenType::K_GOTO) || Check(types::TokenType::K_RETURN) || Check(types::TokenType::K_BREAK) || Check(types::TokenType::K_CONTINUE)) {
                return types::StatementNodeType::JumpStatement;
            } else if(Check(types::TokenType::LPAREN_CURLY)) {
                return types::StatementNodeType::CompoundStatement;
            } else {
                return types::StatementNodeType::ExpressionStatement;
            }
        }

        bool Parser::AtDeclarationStart() {
            return detail::IsSpecifier(Peek().TokenType_v) || Check(types::TokenType::K_TYPEDEF) ||
                   isTypeName(Peek());
        }

        types::StatementNode* Parser::ParseStatement() {
            const size_t SourceStart = current;
            // A bare ';' is a valid empty statement, not a missing expression.
            if (Check(types::TokenType::SEMICOLON)) {
                Advance();
                return AllocNodeAt<types::EmptyStatementNode>(SourceStart);
            }

            switch(DetermineStatementType()) {
                case types::StatementNodeType::SelectionStatement:
                    return this->SelectionStatement();
                case types::StatementNodeType::IterationStatement:
                    return this->IterationStatement();
                case types::StatementNodeType::LabeledStatement:
                    return this->LabelStatement();
                case types::StatementNodeType::JumpStatement:
                    return this->JumpStatement();
                case types::StatementNodeType::CompoundStatement:
                    return this->CompoundStatement();
                case types::StatementNodeType::ExpressionStatement:
                    return this->ParseExpressionStatement();
                default:
                    return nullptr;
            }
        }
    }
}
```

## `src/parser/struct.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <cassert>


namespace baka {
namespace parser {

    std::variant<types::StructDefinitionNode*, types::StructDeclarationNode*> Parser::ParseStruct() {
        const size_t SourceStart = current;
        if (!this->Match(types::TokenType::K_STRUCT)) {
            // todo throw error
            assert(false && "Expected 'struct' keyword");
        }

        if (!this->Check(types::TokenType::IDENTIFIER)) {
            ReportError("expected identifier after 'struct' keyword");
            auto Sync = SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            if (Sync == types::TokenType::SEMICOLON) {
                Advance();
            }
            return AllocNodeAt<types::StructDeclarationNode>(SourceStart,
                AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?")));
        }
        types::IdentifierNode* StructIdentifier = this->ParseIdentifier();
        if (Match(types::TokenType::SEMICOLON)) {
            // this is a declaration
            auto* Node = AllocNodeAt<types::StructDeclarationNode>(SourceStart, StructIdentifier);
            if (!TypeLookup.LookupCurrent(StructIdentifier->GetName()).has_value()) {
                AddType(StructIdentifier);
                GetParserSTE(StructIdentifier)->SetIsStruct();

            } else {
                if(!GetParserSTE(StructIdentifier)->IsStruct()) {
                    ReportError("not a struct");
                }

            }
            return Node;
        }

        // surely a definition
        if(TypeLookup.LookupCurrent(StructIdentifier->GetName()).has_value()) {
            // TODO throw error
            if(GetParserSTE(StructIdentifier)->IsStructDefined()) {
                ReportError("redefinition of struct");
            } else if(!GetParserSTE(StructIdentifier)->IsStruct()) {
                ReportError("not a struct");
            }
        }

        types::IdentifierNode* ParentStructIdentifier = nullptr;
        if (this->Match(types::TokenType::OP_COLON))
        {
            if (!this->Check(types::TokenType::IDENTIFIER)) {
                ReportError("expected identifier after ':'");
            } else {
                ParentStructIdentifier = this->ParseIdentifier();
            }
            if(ParentStructIdentifier && (!LookupType(ParentStructIdentifier) || !GetParserSTE(ParentStructIdentifier)->IsStructDefined())) {
                ReportError("parent struct/class not found in symbol table");
            }
        }

        if (!TypeLookup.LookupCurrent(StructIdentifier->GetName()).has_value()) {
            AddType(StructIdentifier);
            GetParserSTE(StructIdentifier)->SetIsStruct();
        }
        if (GetParserSTE(StructIdentifier)->IsStruct())
            GetParserSTE(StructIdentifier)->SetIsStructDefined();
        EnterScope();

        types::StructBodyNode* Body = nullptr;
        if (!this->Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after struct/class declaration");
            Body = AllocNodeAt<types::StructBodyNode>(SourceStart);
        } else {
            Body = this->ParseStructBody(StructIdentifier);
        }
        if (!Match(types::TokenType::SEMICOLON))
        {
            ReportError("expected ';' after struct/class body", true);
            SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY});
            Match(types::TokenType::SEMICOLON);
        }

        types::StructDefinitionNode* Node = nullptr;
        if (ParentStructIdentifier) {
            Node = AllocNodeAt<types::StructDefinitionNode>(SourceStart, StructIdentifier, ParentStructIdentifier, Body);
        } else {
            Node = AllocNodeAt<types::StructDefinitionNode>(SourceStart, StructIdentifier, Body);
        }

        ExitScope();

        return Node;
    }

}
}
```

## `src/parser/structBody.cc`

```cpp

#include "types/parser/ast/structBody.hh"
#include "parser.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/function.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"
#include <string_view>

namespace baka {
namespace parser {

    types::StructBodyNode* Parser::ParseStructBody(types::IdentifierNode* ParentName) {
        const size_t SourceStart = current;
        types::StructBodyNode* Node = AllocNodeAt<types::StructBodyNode>(SourceStart);

        if(!Match(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{'");
        }

        while(!Match(types::TokenType::RPAREN_CURLY) && !Check(types::TokenType::EOF_TOKEN)) {

            auto isCtor = [&]() -> bool {
                if(Peek().TokenType_v == types::TokenType::IDENTIFIER && Check2(types::TokenType::LPAREN_ROUND)) {
                    const std::string_view& Name = std::get<std::string_view>(Peek().Value);
                    if(ParentName && Name == ParentName->GetName()) {
                        return true;
                    }
                }

                return false;
            };

            if (isCtor()) {
                types::ConstructorNode* Ctor = ParseConstructor(ParentName);
                Node->SetConstructor(Ctor);

            } else if(Check(types::TokenType::OP_TILDE)) {
                auto* destructorParent = ParentName;
                if (!destructorParent) {
                    ReportError("anonymous aggregates cannot declare destructors");
                    destructorParent = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("?"));
                }
                types::DestructorNode* Dtor = ParseDestructor(destructorParent);
                Node->SetDestructor(Dtor);

            } else if(Check(types::TokenType::K_TYPEDEF)) {
                types::TypedefNode* Typedef = ParseTypedef();
                Node->AddTypedef(Typedef);
            } else {
                std::optional<types::FunctionNode*> Func = TryParseFunction();
                if(Func) {
                    Node->AddFunction(Func.value());
                } else {
                    types::DeclarationList* Decl = ParseDeclarationList();
                    Node->AddDeclaration(Decl);
                }
            }
        }

        return Node;
    }

}
}
```

## `src/parser/type.cc`

```cpp
#include "types/parser/ast/identifier.hh"
#include "parser.hh"
#include "utils.hh"
#include <string_view>
#include <variant>

namespace baka {
namespace parser {

    types::IdentifierNode* Parser::ParseTypeIdentifier() {
        const size_t SourceStart = current;
        // Central validation also covers B-owned parameter/function parsers. Keep
        // invalid modifiers out of legacy AST constructors that still assert.
        if (current > 0 && Tokens[current-1].TokenType_v == types::TokenType::K_UNSIGNED &&
            std::holds_alternative<std::string_view>(Peek().Value) &&
            !baka::detail::isUnsignedTypeName(std::get<std::string_view>(Peek().Value))) {
            ReportError("unsigned requires char, short, int, or long");
            if (!Check(types::TokenType::EOF_TOKEN)) Advance();
            auto* invalid = AllocNodeAt<types::IdentifierNode>(SourceStart, std::string_view("int"));
            invalid->setHasError();
            return invalid;
        }
        if(Check(types::TokenType::IDENTIFIER)) {
            types::IdentifierNode* Node =  ParseIdentifier();
            if(!LookupType(Node)) {
                ReportError("unknown type name '" + std::string(Node->GetName()) + "'");
                Node->setHasError();
            }

            return Node;
        } else {
            if(std::holds_alternative<std::string_view>(Peek().Value)) {
                std::string_view Name = std::get<std::string_view>(Peek().Value);
                if(LookupType(Name)) {
                    Advance();
                    return AllocNodeAt<types::IdentifierNode>(SourceStart, Name);
                }
            }

            std::string_view ErrName = "?";
            if (std::holds_alternative<std::string_view>(Peek().Value)) {
                ErrName = std::get<std::string_view>(Peek().Value);
            }
            if (Check(types::TokenType::IDENTIFIER)) {
                ReportError("unknown type name '" + std::string(ErrName) + "'");
            } else {
                ReportError("expected identifier or '('");
            }
            if (!Check(types::TokenType::EOF_TOKEN)) {
                Advance();
            }
            auto* Errored = AllocNodeAt<types::IdentifierNode>(SourceStart, ErrName);
            Errored->setHasError();
            return Errored;
        }
    }

}
}
```

## `src/parser/typedef.cc`

```cpp
#include "parser.hh"
#include "types/parser/ast/declaration.hh"
#include "types/token/token.hh"
#include "utils.hh"


namespace baka {
namespace parser {

    // not supporting : const typdef int x;
    // typedef opt(const) typename decl_ident;
    // wherever declaration list is typedef also must be there
    // this eats semicolon btw
    types::TypedefNode* Parser::ParseTypedef() {
        const size_t SourceStart = current;
        if(!Match(types::TokenType::K_TYPEDEF)) {
            // todo throw error
            assert(false);
        }

        bool isConst = Match(types::TokenType::K_CONST);

        types::TypeSpecifierModifier Modifier = DetermineTypeSpecifierModifier();
        if (Modifier != types::TypeSpecifierModifier::NONE) {
            this->Advance();
        }

        types::IdentifierNode* BoundTypeName = ParseTypeIdentifier();

        types::TypedefNode* Node = AllocNodeAt<types::TypedefNode>(SourceStart, BoundTypeName, Modifier, isConst);

        // redefinition exemption applies to the typedef own variable name. not function params in the typdef
        // eg: typedef int (*foo)(int a, int b) exemption should not apply to a, b
        TypedefFlagGuard InTypedefGuard(*this, true);
        do {
            types::DeclarationIdentifierNode* Variable = ParseDeclarationIdentifier();
            // doesnt matter if typedef is already done on that variable name
            if (auto* identifier = Variable->getIdentifier(); identifier &&
                !TypeLookup.LookupCurrent(identifier->GetName()).has_value())
                RegisterOrReplaceType(identifier);
            Node->AddVariable(Variable);
        } while(Match(types::TokenType::OP_COMMA));

        if (!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after typedef declaration", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }


        return Node;
    }
}
}
```

## `src/parser/typename.cc`

```cpp
#include "parser.hh"
#include "utils.hh"

namespace baka
{
	namespace parser
	{
		std::optional<types::TypeNode*> Parser::TryParseTypeName()
		{
			const size_t SourceStart = current;
			size_t prev = current;

			if (Check(types::TokenType::K_STATIC))
				return std::nullopt;

			types::TypeSpecifierModifier Modifier = types::TypeSpecifierModifier::NONE;
			bool IsConst = false;
			if (Match(types::TokenType::K_CONST)) IsConst = true;

			Modifier = DetermineTypeSpecifierModifier();
			if (Modifier != types::TypeSpecifierModifier::NONE) {
				this->Advance();
			}

			if (!isTypeName(Peek())) {
				current = prev;
				return std::nullopt;
			}

			types::IdentifierNode* TypeName = ParseTypeIdentifier();
			auto* Node = AllocNodeAt<types::TypeNode>(SourceStart, IsConst, Modifier, TypeName);
			return Node;
		}

	}
}
```

## `src/parser/union.cc`

```cpp
#include "types/parser/ast/union.hh"
#include "parser.hh"
#include "types/parser/ast/identifier.hh"
#include "types/token/token.hh"
#include "utils.hh"


namespace baka {
namespace parser {

    types::UnionBodyNode* Parser::ParseUnionBody(types::IdentifierNode* ParentName) {
        // todo: does this have to be diff?
        return this->ParseStructBody(ParentName);
    }

    types::UnionNode* Parser::ParseUnion() {
        const size_t SourceStart = current;
        if(!Match(types::TokenType::K_UNION)) {
            // todo throw error;
            assert(false);
        }

        types::IdentifierNode* UnionName = nullptr;
        if(Check(types::TokenType::IDENTIFIER)) {
            UnionName = ParseIdentifier();
            if (!TypeLookup.LookupCurrent(UnionName->GetName()).has_value()) AddType(UnionName);
        } else if(!Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected identifier or '{' after 'union' keyword");
        }

        if (UnionName && Match(types::TokenType::SEMICOLON))
            return AllocNodeAt<types::UnionNode>(SourceStart, nullptr, UnionName);

        EnterScope();

        if(!Check(types::TokenType::LPAREN_CURLY)) {
            ReportError("expected '{' after union name");
        }
        types::UnionBodyNode* Body = ParseUnionBody(UnionName);
        if(!Match(types::TokenType::SEMICOLON)) {
            ReportError("expected ';' after union body", true);
            if (SkipTo({types::TokenType::SEMICOLON, types::TokenType::RPAREN_CURLY}) == types::TokenType::SEMICOLON) {
                Advance();
            }
        }

        types::UnionNode* Node = AllocNodeAt<types::UnionNode>(SourceStart, Body, UnionName);

        ExitScope();

        return Node;
    }

}
}
```

# Appendix D — Semantic type/symbol/pass interfaces and key implementations, verbatim

## `src/semantic/types.hh`

```cpp
#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace baka {
    namespace semantic {
        // IDs are arena indices. Zero is always invalid/error; never a fabricated int.
        struct TypeId {
            uint32_t value = 0;
            bool operator==(const TypeId&) const = default;
        };

        struct SymbolId {
            uint32_t value = 0;
            bool operator==(const SymbolId&) const = default;
        };

        struct ScopeId {
            uint32_t value = 0;
            bool operator==(const ScopeId&) const = default;
        };

        enum class TypeKind { TypeError, Builtin, Pointer, Array, Function, Const, Enum, Struct, Class, Union };

        enum class Builtin { Void, Bool, Char, UChar, Short, UShort, Int, UInt, Long, ULong, Float, Double };

        enum class Completion { Forward, Defining, Complete };

        enum class BoundKind { Unspecified, Constant };

        struct Bound {
            BoundKind kind = BoundKind::Unspecified;
            uint64_t count = 0;
            bool operator==(const Bound&) const = default;
        };

        struct Member {
            std::string name;
            TypeId type;
            SymbolId symbol{};
            bool isStatic = false;
            bool isType = false;
            bool isConstructor = false;
            bool isDestructor = false;
            bool isConstMethod = false;
            const void* source = nullptr;
            size_t minimumArity = static_cast<size_t>(-1);
        };

        struct Type {
            TypeKind kind = TypeKind::TypeError;
            Builtin builtin = Builtin::Void;
            TypeId element; // pointee, element, return, or qualified type
            Bound bound;
            std::vector<TypeId> parameters;
            bool variadic = false;
            std::string name;
            Completion completion = Completion::Forward;
            TypeId base;
            std::vector<Member> members; // source order, includes static/type/special members
        };

        // 64-bit mixing constant (Kynea / golden ratio derived).
        static constexpr uint64_t HashSeed = 0x9e3779b97f4a7c15ULL;

        inline void MixHash(size_t& Hash, size_t Value) {
            Hash ^= Value + static_cast<size_t>(HashSeed) + (Hash << 6) + (Hash >> 2);
        }

        // Fills the seed with the vector length and every element ID, order-sensitive.
        inline size_t HashVector(size_t Seed, const std::vector<TypeId>& Values) {
            auto Hash = Seed;
            MixHash(Hash, Values.size());
            for (auto Current : Values) {
                MixHash(Hash, Current.value);
            }
            return Hash;
        }

        struct Layout {
            uint64_t size = 0, alignment = 1;
            std::vector<std::optional<uint64_t>> offsets; // indexed like Type::members
            std::optional<uint64_t> baseOffset;
        };

        enum class QueryState { Ready, Incomplete, Error };

        struct LayoutResult {
            QueryState state = QueryState::Error;
            Layout layout;
            std::string message;

            explicit operator bool() const {
                return state == QueryState::Ready;
            }
        };

        // Fixed LP64 target contract. This service performs no expression evaluation.
        class Types {
            std::deque<Type> TypeArena{Type{}};
            std::vector<TypeId> BuiltinTypes;

            // Hash-to-integer interning of canonical type IDs, including parameter vectors.
            size_t CanonicalHash(const Type& TypeInfo) const;
            // Structural equivalence of values two hash entries might share bucket space for.
            bool CanonicalEqual(const Type& LeftType, const Type& RightType) const;

            struct InternHash {
                const Types* TypeSystem;
                size_t operator()(uint32_t index) const;
            };

            struct InternEqual {
                const Types* TypeSystem;
                bool operator()(uint32_t Left, uint32_t Right) const;
            };

            using InternSet = std::unordered_set<uint32_t, InternHash, InternEqual>;
            InternSet InternTable;

            TypeId AddType(Type type);
            TypeId InternType(Type type);
            static bool AddChecked(uint64_t Left, uint64_t Right, uint64_t& out);
            static bool AlignChecked(uint64_t size, uint64_t alignment, uint64_t& out);
            LayoutResult ComputeLayout(TypeId id, std::vector<TypeId>& active) const;

        public:
            // InternTable captures this; identities are only valid inside one instance.
            Types(const Types&) = delete;
            Types& operator=(const Types&) = delete;

            Types();

            const Type& get(TypeId id) const;
            size_t typeCount() const { return TypeArena.size() - 1; }

            TypeId builtin(Builtin Right) const;

            struct ArithmeticInfo {
                unsigned bits, rank;
                bool isSigned, floating;
            };

            std::optional<ArithmeticInfo> arithmetic(TypeId id) const;

            TypeId sizeType() const;
            TypeId unConst(TypeId id) const;
            bool isConst(TypeId id) const;
            bool isAggregate(TypeId id) const;
            bool isIntegral(TypeId id) const;
            bool isVoid(TypeId id) const;

            TypeId constant(TypeId id, bool constantValue = true);
            TypeId pointer(TypeId pointee);
            TypeId array(TypeId element, Bound bound = {});
            TypeId parameterObject(TypeId id);
            TypeId adjustParameter(TypeId id);
            TypeId function(TypeId result, std::vector<TypeId> parameters = {}, bool variadic = false);

            // Nominal declarations are never structurally interned; aliases keep this ID.
            TypeId nominal(TypeKind kind, std::string name = {});

            bool beginDefinition(TypeId id);
            bool setBase(TypeId id, TypeId base);
            bool addMember(TypeId id, Member member);
            bool bindMemberSymbol(TypeId owner, size_t index, SymbolId symbol);
            LayoutResult finishDefinition(TypeId id);
            LayoutResult layout(TypeId id) const;

            // Checks array formation even below pointer layers, independently of sizeof(pointer).
            bool validDerived(TypeId id) const;

            struct MemberLookup {
                TypeId owner;
                std::vector<size_t> indices;
            };

            MemberLookup lookupMember(TypeId receiver, const std::string& name) const;
            bool derivesFrom(TypeId derived, TypeId base) const;
            bool recursivelyConst(TypeId id) const;

            struct Subobject {
                TypeId type;
                std::optional<size_t> memberIndex;
            };

            std::vector<Subobject> subobjects(TypeId id, bool destruction = false) const;
            // Construction and cleanup share these ordered subobject lists; C records jumps.
            std::vector<size_t> instanceMembers(TypeId id, bool destruction = false) const;
        };
    }
}
```

## `src/semantic/symbols.hh`

```cpp
#pragma once

#include "types.hh"
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>

namespace baka {
    namespace semantic {
        enum class ScopeKind { Global, Block, Function, Prototype, Aggregate, For };
        enum class SymbolKind { Object, Function, Alias, Type, Enumerator, Parameter, BuiltinFunction };
        enum class Linkage { None, Internal, External };

        struct Diagnostic {
            std::string code, message;
            const void* source = nullptr;
            const void* previous = nullptr;
        };

        struct Symbol {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            ScopeId scope;
            Linkage linkage = Linkage::None;
            bool defined = false;
            bool isStatic = false;
            const void* source = nullptr;
            std::optional<int64_t> constant;
            // Stored for B/C. Declaration order is symbol ID order.
            std::vector<const void*> declarations;
            std::vector<const void*> defaultArguments{};
        };

        struct Scope {
            ScopeId parent;
            ScopeKind kind = ScopeKind::Block;
            std::unordered_map<std::string, std::vector<SymbolId>> names;
        };

        struct Declaration {
            std::string name;
            SymbolKind kind = SymbolKind::Object;
            TypeId type;
            bool definition = false;
            bool isStatic = false;
            Linkage linkage = Linkage::None;
            const void* source = nullptr;
        };

        class Symbols {
            Types& TypeSystem;
            std::deque<Symbol> SymbolEntries{Symbol{}};
            std::deque<Scope> ScopeArena{Scope{}};
            ScopeId CurrentScope;
            std::vector<Diagnostic> Diagnostics;
            uint32_t GlobalVisibility = std::numeric_limits<uint32_t>::max();

            static bool IsFunctionKind(SymbolKind Kind);
            bool AreCompatible(TypeId ExistingType, TypeId CandidateType) const;

        public:
            explicit Symbols(Types& TypeSystem);

            ScopeId currentScope() const;
            const Scope& scope(ScopeId id) const;
            const Symbol& get(SymbolId id) const;
            const std::vector<Diagnostic>& diagnostics() const;

            void diagnose(std::string code, std::string message, const void* source = nullptr,
                          const void* previous = nullptr);

            ScopeId enter(ScopeKind kind);
            bool leave();
            void selectScope(ScopeId id) { CurrentScope = id; }
            uint32_t symbolCount() const { return static_cast<uint32_t>(SymbolEntries.size() - 1); }
            void setGlobalVisibility(uint32_t limit) { GlobalVisibility = limit; }
            bool defaults(SymbolId id, const std::vector<const void*>& arguments);

            std::vector<SymbolId> lookupCurrent(const std::string& name) const;
            std::vector<SymbolId> lookup(const std::string& name) const;

            SymbolId bind(const std::string& name, const void* source = nullptr);
            SymbolId declare(Declaration NewDeclaration);
            bool completeObject(SymbolId id, TypeId completed);
            bool setConstant(SymbolId id, int64_t value);

            // C supplies an ancestor scope as the destination. Results are in cleanup order.
            std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const;

            TypeId lookupType(const std::string& name, const void* source = nullptr);

            void installTypes();
            void installLibrary();
        };
    }
}
```

## `src/semantic/declarations.hh`

```cpp
#pragma once

#include "symbols.hh"

namespace baka {
    namespace semantic {
        // B supplies the result of required constant evaluation. Unavailable is not a constant.
        enum class ConstantState { Value, Nonconstant, Unavailable, Error };

        struct IntegerResult {
            ConstantState state = ConstantState::Unavailable;
            int64_t value = 0;
        };

        struct DeclarationContext {
            bool parameter = false;
            bool member = false;
            bool prototype = false;
            bool storageRequired = true;
        };

        struct SpecialMembers {
            std::vector<size_t> constructors;
            std::optional<size_t> destructor;
            bool implicitDefaultConstructor = false;
            bool implicitCopyConstructor = false;
            bool implicitDestructor = false;
            bool nontrivial = false;
        };

        class Declarations {
            Types& TypeSystem;
            Symbols& SymbolTable;

        public:
            Declarations(Types& TypeSystem, Symbols& SymbolTable);

            TypeId specifier(std::string name, bool constant = false, bool isUnsigned = false,
                             std::optional<TypeKind> expected = {}, const void* source = nullptr);

            TypeId boundArray(TypeId element, IntegerResult value, const void* expression = nullptr,
                              bool omitted = false);

            bool validate(TypeId id, DeclarationContext context = {}, const void* source = nullptr);

            SymbolId object(Declaration NewDeclaration, DeclarationContext context = {});

            SymbolId functionDefinition(Declaration NewDeclaration);

            SymbolId alias(std::string name, TypeId type, const void* source = nullptr);

            TypeId aggregate(std::string name, TypeKind kind, const void* source = nullptr);

            bool beginAggregate(TypeId id, const void* source = nullptr);

            bool member(TypeId owner, Member member);

            bool base(TypeId owner, TypeId base, const void* source = nullptr);

            bool finishAggregate(TypeId type, const void* source = nullptr);

            SymbolId enumerator(std::string name, TypeId type, IntegerResult value, const void* source = nullptr);

            IntegerResult nextEnumerator(std::optional<int64_t> previous) const;

            SpecialMembers specialMembers(TypeId id) const;

            struct Access {
                TypeId type;
                TypeId owner;
                std::vector<size_t> candidates;
                bool lvalue = false;
            };

            Access memberAccess(TypeId receiver, const std::string& name, bool arrow = false,
                                bool staticContext = false, const void* source = nullptr);

            TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod, const void* source = nullptr);
        };
    }
}
```

## `src/semantic/ast_declarations.hh`

```cpp
#pragma once

#include "declarations.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/typenode.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/enums.hh"

namespace baka {
    namespace semantic {
        struct EvaluationContext {
            enum class Purpose { ArrayBound, Enumerator } purpose;
            ScopeId scope;
            TypeId expected;
        };

        // B implements this interface. Evaluation runs in the live declaration environment.
        class DeclarationEvaluator {
        public:
            virtual ~DeclarationEvaluator() = default;
            virtual IntegerResult integer(const types::ExpressionNode*, EvaluationContext) = 0;
        };

        class ASTDeclarations {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            DeclarationEvaluator* Evaluator;
            std::function<void(types::ExpressionNode*)> PrepareExpression;

            IntegerResult EvaluateInteger(const types::ExpressionNode* expression,
                                          EvaluationContext::Purpose purpose);

        public:
            ASTDeclarations(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                            DeclarationEvaluator* Evaluator = nullptr);

            void setEvaluator(DeclarationEvaluator* evaluator);
            void setExpressionResolver(std::function<void(types::ExpressionNode*)> resolver) {
                PrepareExpression = std::move(resolver);
            }

            TypeId specifier(const types::IdentifierNode* name, types::TypeSpecifierModifier modifier, bool constant,
                             const void* source = nullptr);

            TypeId typeName(const types::TypeNode& node);

            // Build outward suffixes before recursing inward: int (*p)[3] is pointer-to-array.
            TypeId declarator(TypeId base, const types::DeclarationIdentifierNode* node);

            // C enters one Function scope, calls this, then visits the outer body in that SAME scope.
            // Nested blocks use Block scopes, so only genuine inner-block shadowing is legal.
            std::vector<SymbolId> parameters(const types::FunctionParameterList& params);

            // B's initializer callback runs AFTER insertion; each declarator's bound runs BEFORE.
            // Thus int x=x; binds the new x, while int x[sizeof(x)]; sees the prior x.
            using InitializerCheck = std::function<void(SymbolId, const types::ExpressionNode*)>;

            std::vector<SymbolId> declarationList(const types::DeclarationList& node,
                                                  InitializerCheck initialize = {});

            // Infer storage shape from initializer syntax before later declarations
            // ask sizeof(array). Element conversion checks remain in annotation.
            bool inferArrayBound(SymbolId symbol, const types::ExpressionNode* initializer);

            std::vector<SymbolId> aliases(const types::TypedefNode& node);

            TypeId enumeration(const types::EnumNode& node);
        };
    }
}
```

## `src/semantic/aggregates.hh`

```cpp
#pragma once

#include "ast_declarations.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"

namespace baka {
    namespace semantic {
        // B resolves function/special-member signatures from its owned AST. Bodies must be
        // checked by C/B after this service finishes collecting metadata and layout.
        using MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>;

        class ASTAggregates {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            ASTDeclarations& ASTResolver;

        public:
            ASTAggregates(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                          ASTDeclarations& ASTResolver);

            TypeId forward(const types::IdentifierNode* name, TypeKind kind, const void* source = nullptr);

            TypeId definition(const types::IdentifierNode* name, const types::IdentifierNode* parent,
                              const types::StructBodyNode& body, TypeKind kind, MemberSignature signature = {},
                              const void* source = nullptr);

            TypeId definition(const types::StructDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(const types::ClassDefinitionNode& node, MemberSignature signature = {});

            TypeId definition(const types::UnionNode& node, MemberSignature signature = {});
        };
    }
}
```

## `src/semantic/expressions.hh`

```cpp
#pragma once

#include "ast_declarations.hh"
#include "aggregates.hh"
#include "types/parser/ast/expression.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/sizeof.hh"
#include "types/parser/ast/constructor.hh"
#include "types/parser/ast/destructor.hh"
#include "types/parser/ast/initializer.hh"
#include <unordered_map>
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/implicitCast.hh"

namespace baka {
    namespace semantic {
        enum class ValueCategory { LValue, RValue };

        struct ExprResult {
            TypeId type;
            ValueCategory category = ValueCategory::RValue;
            bool error = false;
            bool constantExpression = false;
            IntegerResult constant;
            bool nullPointerConstant = false;
            std::optional<long double> floatingConstant;
            // An instance method designator is bound to an object, not an
            // ordinary function value that may decay to a plain function pointer.
            bool boundInstanceFunction = false;

            explicit operator bool() const {
                return !error && type.value;
            }
        };

        // Expression typing service. Declaration-time dependencies can be evaluated
        // without annotation. beginAnnotation enables the mutating type pass, which
        // consumes frozen bindings and records intrinsic types on the AST.
        class ExpressionChecker : public DeclarationEvaluator {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            ASTDeclarations& ASTResolver;

            bool Annotating = false;
            // Destination conversions are recorded only after compatibility was
            // checked. Materialization runs once typing has finished, so overload
            // selection and constant evaluation see the original syntax tree.
            std::unordered_map<const types::ExpressionNode*, TypeId> ConversionTargets;
            void planConversion(const types::ExpressionNode* expression, TypeId target);
            types::ExpressionNode* wrapConversion(types::ExpressionNode*, TypeId, types::ImplicitConversionKind);
            types::ExpressionNode* convertValue(types::ExpressionNode*, TypeId target = {});
            types::ExpressionNode* materializeImpl(types::ExpressionNode*, bool value, TypeId target = {});
            std::unordered_map<const types::ExpressionNode*, ExprResult> RawResults, ValueResults;
            ExprResult checkImpl(const types::ExpressionNode* expression, bool decay);
            std::vector<SymbolId> lookupIdentifier(const types::IdentifierNode* identifier) const;
            size_t requiredArguments(SymbolId symbol, TypeId type) const;
            struct FunctionCandidate { TypeId type; SymbolId symbol; size_t required; TypeId owner{}; uint32_t memberIndex = 0; };
            std::unordered_map<uint32_t, int64_t> ConstantObjects;

            TypeId CurrentClass;
            bool InStaticContext = false;
            bool InConstMethod = false;

            ExprResult check(const types::ExpressionNode* expression, bool decay = true);
            ExprResult checkPrimary(const types::PrimaryExpressionNode* node);
            ExprResult checkConstant(const types::ConstantNode* node);
            ExprResult checkUnary(const types::FactorNode* node);
            ExprResult checkBinary(const types::BinaryExpressionNode* node);
            ExprResult checkAssignment(const types::BinaryExpressionNode* node);
            ExprResult checkIndex(const types::IndexPostfixExpr* node);
            ExprResult checkMember(const types::ExpressionNode* base, const types::IdentifierNode* member,
                                   bool arrow, const void* source);
            ExprResult checkCall(const types::FunctionCallPostfixExpr* node);
            ExprResult checkTernary(const types::TernaryOpNode* node);
            ExprResult checkCast(const types::CastFactorNode* node);
            ExprResult checkSizeof(const types::SizeofNode* node);
            ExprResult checkInitializerExpression(const types::InitializerNode* node, TypeId target);

            ExprResult makeError();
            ExprResult makeValue(TypeId type, ValueCategory category = ValueCategory::RValue);
            ExprResult makeConstant(TypeId type, int64_t value);
            ExprResult makeBool(bool value);

            bool isInteger(TypeId type) const;
            bool isArithmetic(TypeId type) const;
            bool isPointer(TypeId type) const;
            bool isScalar(TypeId type) const;
            bool isNullPointerConstant(const ExprResult& value) const;
            bool isCompleteObject(TypeId type) const;
            TypeId decay(TypeId type) const;
            bool isBoundInstanceFunction(const types::ExpressionNode* expression) const;
            TypeId promote(TypeId type) const;
            TypeId usualArithmetic(TypeId left, TypeId right, const void* source);
            bool isAssignable(TypeId from, TypeId to, const ExprResult& value) const;
            bool isModifiableLValue(const ExprResult& value) const;
            bool checkScalarCondition(const ExprResult& value, const void* source);

            std::vector<SymbolId> resolveCallee(const types::ExpressionNode* callee, const void* source);
            std::vector<FunctionCandidate> functionCandidates(const types::ExpressionNode* callee, const void* source);
            int conversionRank(TypeId from, TypeId to, const ExprResult& value) const;
            bool checkArguments(const TypeId& functionType, const std::vector<ExprResult>& arguments,
                                const void* source, size_t required);
            std::vector<ExprResult> argumentList(const types::ExpressionNode* list);

            bool initializeScalar(TypeId target, const types::InitializerNode& initializer, const void* source);
            bool initializeAggregate(TypeId target, const types::InitializerNode& initializer, bool topLevel);
            bool initializeArray(TypeId target, const types::InitializerNode& initializer);
            bool initializeList(TypeId target, const std::vector<types::ExpressionNode*>& values,
                                size_t& cursor, bool braced, const void* source);
            bool flattenMembers(TypeId type, std::vector<TypeId>& out) const;

        public:
            ExpressionChecker(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                              ASTDeclarations& ASTResolver);

            void setFunctionContext(TypeId currentClass, bool staticContext, bool constMethod);

            void beginAnnotation() { Annotating = true; RawResults.clear(); ValueResults.clear(); ConversionTargets.clear(); }
            void endAnnotation() { Annotating = false; }
            // Called only by the mutating annotation pass, after all typing work.
            types::ExpressionNode* materializeConversions(types::ExpressionNode* expression, bool value = true,
                                                         TypeId target = {}) {
                return materializeImpl(expression, value, target);
            }
            TypeId promotedType(TypeId type) const { return promote(type); }
            const auto& annotatedExpressions() const { return RawResults; }

            IntegerResult integer(const types::ExpressionNode* expression, EvaluationContext context) override;

            ExprResult checkValue(const types::ExpressionNode* expression);

            bool convertible(TypeId from, TypeId to, const ExprResult& value) const {
                return isAssignable(from, to, value);
            }

            bool checkCondition(const types::ExpressionNode* expression, const void* source);

            // Called from ASTDeclarations::declarationList after insertion. Completes inferred
            // array bounds and validates scalar/aggregate/string initialization.
            bool initialize(SymbolId symbol, const types::ExpressionNode* initializer);

            // B's MemberSignature for ASTAggregates: classifies functions as methods/ctors/dtors.
            std::optional<Member> memberSignature(const types::ASTNode* node, TypeId owner);
        };
    }
}
```

## `src/semantic/semantic_analyzer.hh`

```cpp
#pragma once

#include "expressions.hh"
#include "types/parser/ast/program.hh"
#include "types/parser/ast/selectionStatement.hh"
#include "types/parser/ast/iterationStatement.hh"
#include "types/parser/ast/labelStatement.hh"
#include "types/parser/ast/jump.hh"
#include "types/parser/ast/statement.hh"
#include <limits>
#include <unordered_set>

namespace baka::semantic {
    // The public phase boundaries are intentional: tests and future lowering can
    // inspect the mutated tree between passes without rerunning the parser.
    enum class SemanticPhase { Parsed, Registered, Resolved, Annotated, Checked };

    class SemanticAnalyzer {
        Types TypeSystem;
        Symbols SymbolTable;
        Declarations DeclarationChecker;
        ASTDeclarations ASTResolver;
        ExpressionChecker Checker;
        ASTAggregates Aggregates;
        SemanticPhase Phase = SemanticPhase::Parsed;
        std::vector<Diagnostic> Diagnostics;

        struct Context {
            ScopeId scope;
            TypeId owner, result;
            uint32_t globalLimit = std::numeric_limits<uint32_t>::max();
            bool isStatic = false, isConst = false;
        };
        Context Current;
        enum class ExpressionUse { Value, Condition, Initializer, Default, Return, Switch, Case };
        struct ExpressionWork {
            types::ExpressionNode* expression = nullptr;
            const types::ASTNode* source = nullptr;
            Context context;
            ExpressionUse use = ExpressionUse::Value;
            SymbolId symbol;
            TypeId expected;
            bool staticStorage = false;
            ExprResult value;
        };
        struct BodyWork {
            types::ASTNode* declaration = nullptr;
            types::StatementNode* body = nullptr;
            types::FunctionParameterList* parameters = nullptr;
            Context context;
            TypeId signature;
            SymbolId symbol;
        };
        struct JumpWork {
            types::StatementNode* statement = nullptr;
            const types::StatementNode* target = nullptr;
            std::vector<SymbolId> initialized;
        };
        struct LabelWork {
            types::GotoLabelStatementNode* label = nullptr;
            std::vector<SymbolId> initialized;
        };
        struct FunctionFlow {
            std::vector<JumpWork> jumps;
            std::vector<LabelWork> labels;
        };
        struct SwitchWork {
            const types::SwitchBlockStatementNode* statement = nullptr;
            size_t condition = 0;
            std::vector<size_t> cases;
            std::vector<const types::DefaultLabelStatementNode*> defaults;
        };
        // Registration collects these once. Later phases consume the queues, not
        // repeated DFS traversals from ProgramNode. Resolution visits each body
        // in lexical order and appends contextual expression/control work.
        std::vector<types::ASTNode*> TopLevel;
        std::vector<BodyWork> Bodies;
        std::vector<ExpressionWork> Expressions;
        std::unordered_set<const types::ExpressionNode*> DeclarationExpressions;
        std::vector<FunctionFlow> Flows;
        std::vector<SwitchWork> Switches;
        std::vector<const types::ASTNode*> InvalidNodes;
        std::vector<std::pair<const types::ASTNode*, std::string>> ResolutionErrors;
        std::vector<const types::StatementNode*> ActiveTargets;
        std::vector<std::string> ActiveLoopNames;
        std::vector<size_t> ActiveSwitches;
        std::vector<SymbolId> Initialized;
        size_t CurrentFlow = 0;
        uint32_t NextTarget = 1;

        void select(const Context& context);
        void refreshDiagnostics();
        void error(std::string code, std::string message, const types::ASTNode* source);
        void registerNode(types::ASTNode* node);
        void registerFunction(types::FunctionNode* function, TypeId owner = {});
        void registerMembers(types::StructBodyNode* body, TypeId owner);
        void registerDefaults(types::FunctionParameterList* parameters, TypeId signature,
                              SymbolId symbol, const types::ASTNode* source);
        void registerDeclarations(types::DeclarationList* list);
        void registerLocal(types::DeclarationList* list);
        void resolveBody(BodyWork& body);
        void resolveStatement(types::StatementNode* statement, bool outerBody = false);
        void resolveExpression(types::ExpressionNode* expression);
        void resolveInitializer(types::ExpressionNode* expression);
        void resolveTypeOperand(types::TypeNode* type, types::DeclarationIdentifierNode* declarator);
        size_t queue(types::ExpressionNode* expression, const types::ASTNode* source,
                     ExpressionUse use = ExpressionUse::Value, SymbolId symbol = {},
                     TypeId expected = {}, bool staticStorage = false);
        std::vector<Diagnostic> checkReadOnly() const;

    public:
        SemanticAnalyzer();
        bool registerDeclarations(types::ProgramNode& program);
        bool resolveAndRename();
        bool annotateTypes();
        bool check();
        bool analyze(types::ProgramNode& program);
        SemanticPhase phase() const { return Phase; }
        const std::vector<Diagnostic>& diagnostics() const { return Diagnostics; }
        const Types& types() const { return TypeSystem; }
        const Symbols& symbols() const { return SymbolTable; }
        size_t topLevelWorkCount() const { return TopLevel.size(); }
        size_t bodyWorkCount() const { return Bodies.size(); }
        size_t expressionWorkCount() const { return Expressions.size(); }
    };
}
```

## `src/semantic/semantic_analyzer.cc`

```cpp
#include "semantic_analyzer.hh"

namespace baka::semantic {
    SemanticAnalyzer::SemanticAnalyzer()
        : SymbolTable(TypeSystem), DeclarationChecker(TypeSystem, SymbolTable),
          ASTResolver(TypeSystem, SymbolTable, DeclarationChecker),
          Checker(TypeSystem, SymbolTable, DeclarationChecker, ASTResolver),
          Aggregates(TypeSystem, SymbolTable, DeclarationChecker, ASTResolver) {
        ASTResolver.setEvaluator(&Checker);
        // Bounds and enumerators are declaration-time dependencies. Resolve their
        // operands in the live lexical environment before evaluating them, rather
        // than deferring lookup until scopes have acquired later declarations.
        ASTResolver.setExpressionResolver([this](types::ExpressionNode* expression) {
            resolveExpression(expression);
            if (expression && DeclarationExpressions.insert(expression).second)
                queue(expression, expression);
        });
        SymbolTable.installTypes();
        SymbolTable.installLibrary();
        Current.scope = SymbolTable.currentScope();
    }

    void SemanticAnalyzer::select(const Context& context) {
        Current = context;
        SymbolTable.selectScope(context.scope);
        SymbolTable.setGlobalVisibility(context.globalLimit);
        Checker.setFunctionContext(context.owner, context.isStatic, context.isConst);
    }
    void SemanticAnalyzer::refreshDiagnostics() { Diagnostics = SymbolTable.diagnostics(); }
    void SemanticAnalyzer::error(std::string code, std::string message, const types::ASTNode* source) {
        SymbolTable.diagnose(std::move(code), std::move(message), source);
    }
    bool SemanticAnalyzer::analyze(types::ProgramNode& program) {
        // Invalid top-level signatures/types can invalidate later work entirely.
        // Once registration succeeds, local resolution/type errors are recoverable:
        // error-valued expressions suppress cascades, while independent work still
        // receives diagnostics (e.g. a bad call after a duplicate local).
        if (!registerDeclarations(program)) return false;
        resolveAndRename();
        annotateTypes();
        check();
        return Diagnostics.empty();
    }
}
```

## `src/semantic/registration.cc`

```cpp
#include "semantic_analyzer.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"

namespace baka::semantic {
    namespace {
        types::FunctionParameterList* Parameters(types::DeclarationIdentifierNode* declarator) {
            if (!declarator) return nullptr;
            if (auto* parameters = dynamic_cast<types::FunctionParameterList*>(declarator->getFunctionParameters()))
                return parameters;
            if (auto* inner = std::get_if<types::DeclarationIdentifierNode*>(&declarator->getInnerDeclaration()))
                return Parameters(*inner);
            return nullptr;
        }
    }

    bool SemanticAnalyzer::registerDeclarations(types::ProgramNode& program) {
        if (Phase != SemanticPhase::Parsed) return false;
        TopLevel.assign(program.getBody().begin(), program.getBody().end());
        // This is the only ProgramNode traversal. Definitions enqueue bodies;
        // they never inspect those bodies while top-level names are collected.
        for (auto* node : TopLevel) registerNode(node);
        Phase = SemanticPhase::Registered;
        refreshDiagnostics();
        return Diagnostics.empty();
    }

    void SemanticAnalyzer::registerDefaults(types::FunctionParameterList* parameters, TypeId signature,
                                           SymbolId symbol, const types::ASTNode* source) {
        if (!parameters || !signature.value || TypeSystem.get(signature).kind != TypeKind::Function) return;
        const auto& types = TypeSystem.get(signature).parameters;
        std::vector<const void*> defaults(types.size());
        for (size_t i = 0; i < parameters->getParameters().size() && i < types.size(); ++i) {
            auto* parameter = parameters->getParameters()[i];
            if (!parameter || !parameter->getDeclaration()) continue;
            auto* initializer = parameter->getDeclaration()->getInitializer();
            defaults[i] = initializer;
            if (initializer) {
                auto index = queue(initializer, parameter, ExpressionUse::Default, {}, types[i]);
                // Defaults are declared outside the eventual function invocation;
                // there is no implicit instance object in that environment.
                if (Current.owner.value) Expressions[index].context.isStatic = true;
            }
        }
        if (symbol.value) SymbolTable.defaults(symbol, defaults);
        (void)source;
    }

    void SemanticAnalyzer::registerDeclarations(types::DeclarationList* list) {
        if (!list) return;
        const auto ids = ASTResolver.declarationList(*list, [this](SymbolId symbol, const types::ExpressionNode* initializer) {
            // Resolve a const scalar dependency while its declaration-point scope
            // is live, so subsequent array bounds and enumerators can use it.
            if (initializer && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                resolveInitializer(const_cast<types::ExpressionNode*>(initializer));
                auto value = Checker.integer(initializer, {EvaluationContext::Purpose::ArrayBound,
                    SymbolTable.currentScope(), TypeSystem.builtin(Builtin::Int)});
                if (value.state == ConstantState::Value) SymbolTable.setConstant(symbol, value.value);
            }
        });
        const bool global = SymbolTable.scope(SymbolTable.currentScope()).kind == ScopeKind::Global;
        for (size_t i = 0; i < ids.size(); ++i) {
            auto id = ids[i];
            if (!id.value) continue;
            auto* item = list->getDeclarations()[i];
            const auto& symbol = SymbolTable.get(id);
            Current.globalLimit = id.value;
            if (symbol.kind == SymbolKind::Function) {
                registerDefaults(Parameters(item->getDeclarator()), symbol.type, id, item);
                if (item->getInitializer()) error("initializer", "function declaration cannot have an initializer", item);
            } else {
                queue(item->getInitializer(), item, ExpressionUse::Initializer, id, symbol.type,
                      global || list->getIsStatic());
            }
        }
    }

    void SemanticAnalyzer::registerFunction(types::FunctionNode* node, TypeId owner) {
        auto type = owner.value ? TypeId{node->semanticInfo().Type} :
            ASTResolver.declarator(ASTResolver.specifier(node->getReturnTypeName(), node->getModifier(),
                                                        node->getIsConst(), node), node->getDeclarator());
        if (!type.value || TypeSystem.get(type).kind != TypeKind::Function) {
            error("function", "function body requires a function declarator", node);
            return;
        }
        SymbolId symbol{node->semanticInfo().Symbol};
        if (!owner.value) {
            symbol = DeclarationChecker.functionDefinition({std::string(node->getDeclarator()->getVariableName()),
                SymbolKind::Function, type, true, node->getIsStatic(),
                node->getIsStatic() ? Linkage::Internal : Linkage::External, node});
            node->semanticInfo().Type = type.value;
            node->semanticInfo().Symbol = symbol.value;
        }
        auto* name = node->getDeclarator()->getIdentifier();
        if (name && symbol.value) {
            name->resolve({symbol.value}, std::string(name->GetName()) + "." + std::to_string(symbol.value));
            name->semanticInfo().Type = type.value;
        }
        Current.globalLimit = SymbolTable.symbolCount();
        auto context = Current;
        context.owner = owner; context.result = TypeSystem.get(type).element;
        context.isStatic = node->getIsStatic(); context.isConst = false; // leading const qualifies return type
        auto* parameters = Parameters(node->getDeclarator());
        registerDefaults(parameters, type, symbol, node);
        Bodies.push_back({node, node->getBody(), parameters, context, type, symbol});
    }

    void SemanticAnalyzer::registerMembers(types::StructBodyNode* body, TypeId owner) {
        auto saved = Current;
        Current.scope = {body->semanticInfo().Scope};
        Current.owner = owner;
        Current.globalLimit = SymbolTable.symbolCount();
        select(Current);
        for (auto* member : body->getMembers()) {
            if (auto* function = dynamic_cast<types::FunctionNode*>(member)) registerFunction(function, owner);
            else if (auto* constructor = dynamic_cast<types::ConstructorNode*>(member)) {
                auto context = Current; context.result = TypeSystem.builtin(Builtin::Void);
                auto signature = TypeId{constructor->semanticInfo().Type};
                registerDefaults(constructor->getArgs(), signature, {}, constructor);
                Bodies.push_back({constructor, constructor->getBody(), constructor->getArgs(), context, signature, {}});
            } else if (auto* destructor = dynamic_cast<types::DestructorNode*>(member)) {
                auto context = Current; context.result = TypeSystem.builtin(Builtin::Void);
                Bodies.push_back({destructor, destructor->getBody(), nullptr, context,
                                  TypeId{destructor->semanticInfo().Type}, {}});
            } else if (auto* list = dynamic_cast<types::DeclarationList*>(member)) {
                for (auto* item : list->getDeclarations()) {
                    SymbolId symbol{item->semanticInfo().Symbol};
                    if (!symbol.value) continue;
                    if (SymbolTable.get(symbol).kind == SymbolKind::Function)
                        registerDefaults(Parameters(item->getDeclarator()), SymbolTable.get(symbol).type, symbol, item);
                    else if (item->getInitializer())
                        queue(item->getInitializer(), item, ExpressionUse::Initializer, symbol,
                              SymbolTable.get(symbol).type, list->getIsStatic());
                }
            }
        }
        select(saved);
    }

    void SemanticAnalyzer::registerNode(types::ASTNode* node) {
        if (!node) return;
        Current.scope = SymbolTable.currentScope();
        Current.globalLimit = std::numeric_limits<uint32_t>::max();
        SymbolTable.setGlobalVisibility(Current.globalLimit);
        if (auto* list = dynamic_cast<types::DeclarationList*>(node)) registerDeclarations(list);
        else if (auto* alias = dynamic_cast<types::TypedefNode*>(node)) ASTResolver.aliases(*alias);
        else if (auto* enumeration = dynamic_cast<types::EnumNode*>(node)) ASTResolver.enumeration(*enumeration);
        else if (auto* forward = dynamic_cast<types::StructDeclarationNode*>(node))
            node->semanticInfo().Type = Aggregates.forward(forward->getName(), TypeKind::Struct, node).value;
        else if (auto* forward = dynamic_cast<types::ClassDeclarationNode*>(node))
            node->semanticInfo().Type = Aggregates.forward(forward->getName(), TypeKind::Class, node).value;
        else if (auto* definition = dynamic_cast<types::StructDefinitionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value) registerMembers(definition->getBody(), owner);
        } else if (auto* definition = dynamic_cast<types::ClassDefinitionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value) registerMembers(definition->getBody(), owner);
        } else if (auto* definition = dynamic_cast<types::UnionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value && definition->getBody()) registerMembers(definition->getBody(), owner);
        } else if (auto* function = dynamic_cast<types::FunctionNode*>(node)) registerFunction(function);
        else error("node", "unsupported top-level semantic node", node);
    }
}
```

## `src/semantic/resolution.cc`

```cpp
#include "semantic_analyzer.hh"
#include <algorithm>
#include <type_traits>

namespace baka::semantic {
    size_t SemanticAnalyzer::queue(types::ExpressionNode* expression, const types::ASTNode* source,
                                   ExpressionUse use, SymbolId symbol, TypeId expected, bool staticStorage) {
        auto context = Current;
        context.scope = SymbolTable.currentScope();
        Expressions.push_back({expression, source, context, use, symbol, expected, staticStorage, {}});
        return Expressions.size() - 1;
    }

    void SemanticAnalyzer::resolveTypeOperand(types::TypeNode* type, types::DeclarationIdentifierNode* declarator) {
        if (!type) return;
        auto resolved = ASTResolver.typeName(*type);
        if (declarator) resolved = ASTResolver.declarator(resolved, declarator);
        type->semanticInfo().Type = resolved.value;
        type->semanticInfo().Resolved = true;
    }

    void SemanticAnalyzer::resolveInitializer(types::ExpressionNode* expression) {
        if (auto* initializer = dynamic_cast<types::InitializerNode*>(expression)) {
            initializer->semanticInfo().Resolved = true;
            initializer->semanticInfo().Scope = SymbolTable.currentScope().value;
            for (auto* child : initializer->expressions()) resolveInitializer(child);
        } else resolveExpression(expression);
    }

    void SemanticAnalyzer::resolveExpression(types::ExpressionNode* expression) {
        if (!expression || expression->semanticInfo().Resolved) return;
        auto& metadata = expression->semanticInfo();
        metadata.Resolved = true;
        metadata.Scope = SymbolTable.currentScope().value;
        if (auto* primary = dynamic_cast<types::PrimaryExpressionNode*>(expression)) {
            auto payload = primary->getExpression();
            if (auto* name = std::get_if<types::IdentifierNode*>(&payload); name && *name) {
                auto* identifier = *name;
                auto found = SymbolTable.lookup(std::string(identifier->GetName()));
                // Local names shadow members; members shadow the surrounding global
                // scope in an instance/static method. Aggregate scopes persist.
                if (Current.owner.value && (found.empty() ||
                    SymbolTable.scope(SymbolTable.get(found.front()).scope).kind == ScopeKind::Global)) {
                    auto member = TypeSystem.lookupMember(Current.owner, std::string(identifier->GetName()));
                    if (!member.indices.empty()) {
                        found.clear();
                        for (auto index : member.indices) {
                            const auto& entry = TypeSystem.get(member.owner).members[index];
                            if (entry.symbol.value) found.push_back(entry.symbol);
                        }
                    }
                }
                if (found.empty()) {
                    error("undeclared", "undeclared name '" + std::string(identifier->GetName()) + "'", identifier);
                    metadata.Error = true;
                    identifier->resolve({}, {});
                } else {
                    std::vector<uint32_t> ids;
                    for (auto id : found) ids.push_back(id.value);
                    identifier->resolve(std::move(ids), std::string(identifier->GetName()) + "." +
                                        std::to_string(found.front().value));
                    identifier->semanticInfo().Scope = SymbolTable.currentScope().value;
                    metadata.Symbol = found.size() == 1 ? found.front().value : 0;
                }
            } else if (auto* self = std::get_if<types::ThisNode*>(&payload); self && *self) {
                auto found = SymbolTable.lookup("this");
                if (!found.empty()) {
                    metadata.Symbol = found.front().value;
                    (*self)->semanticInfo().Symbol = found.front().value;
                    (*self)->semanticInfo().Resolved = true;
                }
            }
        } else if (auto* factor = dynamic_cast<types::FactorNode*>(expression)) resolveExpression(factor->getOperand());
        else if (auto* binary = dynamic_cast<types::BinaryExpressionNode*>(expression)) {
            resolveExpression(binary->getLeft()); resolveExpression(binary->getRight());
        } else if (auto* postfix = dynamic_cast<types::UnaryPostfixExpr*>(expression)) resolveExpression(postfix->getOperand());
        else if (auto* index = dynamic_cast<types::IndexPostfixExpr*>(expression)) {
            resolveExpression(index->getBase()); resolveExpression(index->getIndex());
        } else if (auto* member = dynamic_cast<types::MemberPostfixExpr*>(expression)) resolveExpression(member->getBase());
        else if (auto* member = dynamic_cast<types::ArrowPostfixExpr*>(expression)) resolveExpression(member->getBase());
        else if (auto* call = dynamic_cast<types::FunctionCallPostfixExpr*>(expression)) {
            resolveExpression(call->getCallee()); resolveExpression(call->getArguments());
        } else if (auto* comma = dynamic_cast<types::CommaExpressionNode*>(expression)) {
            for (auto* child : comma->getExpressions()) resolveExpression(child);
        } else if (auto* ternary = dynamic_cast<types::TernaryOpNode*>(expression)) {
            resolveExpression(ternary->getCondition()); resolveExpression(ternary->getThen()); resolveExpression(ternary->getElse());
        } else if (auto* cast = dynamic_cast<types::CastFactorNode*>(expression)) {
            resolveTypeOperand(cast->getType(), cast->getDeclarator()); resolveExpression(cast->getExpression());
        } else if (auto* size = dynamic_cast<types::SizeofNode*>(expression)) {
            if (auto* type = std::get_if<types::SizeofNode::SizeofTypeVariant>(&size->Payload))
                resolveTypeOperand(type->Type, type->AbstractDecl);
            else resolveExpression(std::get<types::ExpressionNode*>(size->Payload));
        } else if (dynamic_cast<types::InitializerNode*>(expression)) resolveInitializer(expression);
    }

    void SemanticAnalyzer::registerLocal(types::DeclarationList* list) {
        const bool isStatic = list->getIsStatic();
        // The callback executes after EACH insertion, before the next declarator.
        // Resolving all initializers after registering the whole list would make
        // int a=b,b=1 incorrectly bind the later local b.
        ASTResolver.declarationList(*list, [this, isStatic](SymbolId symbol, const types::ExpressionNode* raw) {
            auto* expression = const_cast<types::ExpressionNode*>(raw);
            resolveInitializer(expression);
            auto source = static_cast<const types::ASTNode*>(SymbolTable.get(symbol).source);
            queue(expression, source, ExpressionUse::Initializer, symbol, SymbolTable.get(symbol).type, isStatic);
            if (expression) Initialized.push_back(symbol);
            // Declaration-time const dependencies are the same explicit exception
            // as enum/bound evaluation. Ordinary expression annotations wait for pass 3.
            if (expression && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                auto value = Checker.integer(expression, {EvaluationContext::Purpose::ArrayBound,
                    SymbolTable.currentScope(), TypeSystem.builtin(Builtin::Int)});
                if (value.state == ConstantState::Value) SymbolTable.setConstant(symbol, value.value);
            }
        });
    }

    void SemanticAnalyzer::resolveBody(BodyWork& body) {
        select(body.context);
        Current.scope = SymbolTable.enter(ScopeKind::Function);
        body.declaration->semanticInfo().Scope = Current.scope.value;
        body.declaration->semanticInfo().Resolved = true;
        if (Current.owner.value && !Current.isStatic) {
            auto type = TypeSystem.pointer(TypeSystem.constant(Current.owner, Current.isConst));
            SymbolTable.declare({"this", SymbolKind::Parameter, type, true, false, Linkage::None, body.declaration});
        }
        if (body.parameters) ASTResolver.parameters(*body.parameters);
        CurrentFlow = Flows.size();
        Flows.emplace_back();
        Initialized.clear(); ActiveTargets.clear(); ActiveLoopNames.clear(); ActiveSwitches.clear();
        resolveStatement(body.body, true);
        // Gotos are function-wide, unlike loop suffixes. Resolve them after this
        // body's single traversal so forward labels need no extra AST walk.
        for (auto& jump : Flows.back().jumps) {
            if (auto* go = dynamic_cast<types::GotoStatementNode*>(jump.statement)) {
                for (const auto& label : Flows.back().labels) {
                    if (go->getLabel()->GetName() == label.label->getTag()->GetName()) {
                        jump.target = label.label;
                        go->semanticInfo().Target = label.label->semanticInfo().Target;
                        go->getLabel()->semanticInfo().Target = go->semanticInfo().Target;
                        go->getLabel()->semanticInfo().Resolved = true;
                        break;
                    }
                }
            }
        }
        SymbolTable.leave();
    }

    bool SemanticAnalyzer::resolveAndRename() {
        if (Phase != SemanticPhase::Registered || !Diagnostics.empty()) return false;
        const auto registeredExpressions = Expressions.size();
        for (size_t i = 0; i < registeredExpressions; ++i) {
            auto context = Expressions[i].context;
            auto* expression = Expressions[i].expression;
            select(context); resolveInitializer(expression);
            auto symbol = Expressions[i].symbol;
            if (symbol.value && expression && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                auto value = Checker.integer(expression, {EvaluationContext::Purpose::ArrayBound,
                    context.scope, TypeSystem.builtin(Builtin::Int)});
                if (value.state == ConstantState::Value) SymbolTable.setConstant(symbol, value.value);
            }
        }
        for (auto& body : Bodies) resolveBody(body);
        Phase = SemanticPhase::Resolved;
        refreshDiagnostics();
        return Diagnostics.empty();
    }

    void SemanticAnalyzer::resolveStatement(types::StatementNode* node, bool outerBody) {
        if (!node) return;
        node->semanticInfo().Scope = SymbolTable.currentScope().value;
        node->semanticInfo().Resolved = true;
        if (auto* block = dynamic_cast<types::CompoundStatementNode*>(node)) {
            if (!outerBody) SymbolTable.enter(ScopeKind::Block);
            auto initializedCount = Initialized.size();
            for (const auto& item : block->getStatements()) std::visit([&](auto* child) {
                using T = std::remove_pointer_t<decltype(child)>;
                if constexpr (std::is_same_v<T, types::StatementNode>) resolveStatement(child);
                else if constexpr (std::is_same_v<T, types::DeclarationList>) registerLocal(child);
                else if constexpr (std::is_same_v<T, types::TypedefNode>) ASTResolver.aliases(*child);
                else InvalidNodes.push_back(child); // nested definitions deliberately unsupported
            }, item);
            Initialized.resize(initializedCount);
            if (!outerBody) SymbolTable.leave();
        } else if (auto* statement = dynamic_cast<types::ExpressionStatementNode*>(node)) {
            resolveExpression(statement->getExpression()); queue(statement->getExpression(), node);
        } else if (auto* chain = dynamic_cast<types::IfSuperBlockStatementNode*>(node)) {
            resolveStatement(chain->getIf());
            for (auto* branch : chain->getElseIfs()) resolveStatement(branch);
            resolveStatement(chain->getElse());
        } else if (auto* branch = dynamic_cast<types::IfSubBlockStatementNode*>(node)) {
            resolveExpression(branch->getCondition()); queue(branch->getCondition(), node, ExpressionUse::Condition);
            resolveStatement(branch->getBody());
        } else if (auto* branch = dynamic_cast<types::ElseIfSubBlockStatementNode*>(node)) {
            resolveExpression(branch->getCondition()); queue(branch->getCondition(), node, ExpressionUse::Condition);
            resolveStatement(branch->getBody());
        } else if (auto* branch = dynamic_cast<types::ElseSubBlockStatementNode*>(node)) resolveStatement(branch->getBody());
        else if (auto* selection = dynamic_cast<types::SwitchBlockStatementNode*>(node)) {
            node->semanticInfo().Target = NextTarget++;
            resolveExpression(selection->getCondition());
            auto condition = queue(selection->getCondition(), node, ExpressionUse::Switch);
            auto index = Switches.size(); Switches.push_back({selection, condition, {}, {}});
            ActiveSwitches.push_back(index); ActiveTargets.push_back(node);
            resolveStatement(selection->getBody());
            ActiveTargets.pop_back(); ActiveSwitches.pop_back();
        } else if (auto* loop = dynamic_cast<types::IterationStatementNode*>(node)) {
            node->semanticInfo().Target = NextTarget++;
            types::IdentifierNode* label = nullptr;
            types::StatementNode* body = nullptr;
            types::ExpressionNode* condition = nullptr;
            const bool isFor = dynamic_cast<types::ForBlockStatementNode*>(loop);
            auto initializedCount = Initialized.size();
            if (auto* forLoop = dynamic_cast<types::ForBlockStatementNode*>(loop)) {
                SymbolTable.enter(ScopeKind::For);
                std::visit([&](auto* initializer) {
                    using T = std::remove_pointer_t<decltype(initializer)>;
                    if (!initializer) return;
                    if constexpr (std::is_same_v<T, types::ExpressionNode>) {
                        resolveExpression(initializer); queue(initializer, node);
                    } else if constexpr (std::is_same_v<T, types::DeclarationList>) registerLocal(initializer);
                    else ASTResolver.aliases(*initializer);
                }, forLoop->getInitializer());
                condition = forLoop->getCondition(); body = forLoop->getBody(); label = forLoop->getLabel();
                resolveExpression(forLoop->getUpdate()); queue(forLoop->getUpdate(), node);
            } else if (auto* whileLoop = dynamic_cast<types::WhileBlockStatementNode*>(loop)) {
                condition = whileLoop->getCondition(); body = whileLoop->getBody(); label = whileLoop->getLabel();
            } else if (auto* doLoop = dynamic_cast<types::DoWhileBlockStatementNode*>(loop)) {
                condition = doLoop->getCondition(); body = doLoop->getBody(); label = doLoop->getLabel();
            }
            resolveExpression(condition);
            if (condition) queue(condition, node, ExpressionUse::Condition);
            std::string name = label ? std::string(label->GetName()) : std::string();
            if (!name.empty() && std::find(ActiveLoopNames.begin(), ActiveLoopNames.end(), name) != ActiveLoopNames.end())
                ResolutionErrors.push_back({label, "duplicate active loop label '" + name + "'"});
            // Prebind the label even for do-while: its source suffix follows the body.
            if (label) { label->semanticInfo().Target = node->semanticInfo().Target; label->semanticInfo().Resolved = true; }
            ActiveTargets.push_back(node); ActiveLoopNames.push_back(name);
            resolveStatement(body);
            ActiveTargets.pop_back(); ActiveLoopNames.pop_back();
            Initialized.resize(initializedCount);
            if (isFor) SymbolTable.leave();
        } else if (auto* result = dynamic_cast<types::ReturnStatementNode*>(node)) {
            resolveExpression(result->getExpression()); queue(result->getExpression(), node, ExpressionUse::Return, {}, Current.result);
        } else if (auto* label = dynamic_cast<types::GotoLabelStatementNode*>(node)) {
            label->semanticInfo().Target = NextTarget++;
            label->getTag()->semanticInfo().Target = label->semanticInfo().Target;
            label->getTag()->semanticInfo().Resolved = true;
            Flows[CurrentFlow].labels.push_back({label, Initialized});
            resolveStatement(label->getStatement());
        } else if (auto* label = dynamic_cast<types::CaseLabelStatementNode*>(node)) {
            resolveExpression(label->getTag()); auto index = queue(label->getTag(), node, ExpressionUse::Case);
            if (ActiveSwitches.empty()) ResolutionErrors.push_back({node, "case label is only valid inside a switch"});
            else Switches[ActiveSwitches.back()].cases.push_back(index);
            resolveStatement(label->getStatement());
        } else if (auto* label = dynamic_cast<types::DefaultLabelStatementNode*>(node)) {
            if (ActiveSwitches.empty()) ResolutionErrors.push_back({node, "default label is only valid inside a switch"});
            else Switches[ActiveSwitches.back()].defaults.push_back(label);
            resolveStatement(label->getStatement());
        } else if (dynamic_cast<types::GotoStatementNode*>(node)) {
            Flows[CurrentFlow].jumps.push_back({node, nullptr, Initialized});
        } else if (dynamic_cast<types::BreakStatementNode*>(node) || dynamic_cast<types::ContinueStatementNode*>(node)) {
            const bool isContinue = dynamic_cast<types::ContinueStatementNode*>(node);
            auto* label = isContinue ? static_cast<types::ContinueStatementNode*>(node)->getLabel()
                                     : static_cast<types::BreakStatementNode*>(node)->getLabel();
            const types::StatementNode* target = nullptr;
            size_t loopIndex = ActiveLoopNames.size();
            for (auto it = ActiveTargets.rbegin(); it != ActiveTargets.rend(); ++it) {
                bool loop = dynamic_cast<const types::IterationStatementNode*>(*it);
                if (loop) --loopIndex;
                if (label) {
                    if (loop && ActiveLoopNames[loopIndex] == label->GetName()) { target = *it; break; }
                } else if (loop || !isContinue) { target = *it; break; }
            }
            if (target) {
                node->semanticInfo().Target = target->semanticInfo().Target;
                if (label) {
                    label->semanticInfo().Target = node->semanticInfo().Target;
                    label->semanticInfo().Resolved = true;
                }
            }
            Flows[CurrentFlow].jumps.push_back({node, target, Initialized});
        } else if (!dynamic_cast<types::EmptyStatementNode*>(node)) InvalidNodes.push_back(node);
    }
}
```

## `src/semantic/annotation.cc`

```cpp
#include "semantic_analyzer.hh"

namespace baka::semantic {
    bool SemanticAnalyzer::annotateTypes() {
        if (Phase != SemanticPhase::Resolved) return false;
        Checker.beginAnnotation();
        for (auto& work : Expressions) {
            select(work.context);
            if (work.use == ExpressionUse::Initializer) {
                bool initialized = Checker.initialize(work.symbol, work.expression);
                if (work.expression) work.expression->semanticInfo().Error = !initialized;
                if (work.symbol.value) {
                    work.expected = SymbolTable.get(work.symbol).type; // inferred array bounds
                    auto* declaration = const_cast<types::ASTNode*>(work.source);
                    declaration->semanticInfo().Type = work.expected.value;
                    declaration->semanticInfo().Annotated = true;
                    declaration->semanticInfo().Error = !initialized;
                    if (auto* item = dynamic_cast<types::SingleDeclarationNode*>(declaration)) {
                        item->getDeclarator()->semanticInfo().Type = work.expected.value;
                        item->getDeclarator()->getIdentifier()->semanticInfo().Type = work.expected.value;
                    }
                    if (work.expression) {
                        auto& metadata = work.expression->semanticInfo();
                        metadata.Type = work.expected.value;
                        metadata.Annotated = true;
                    }
                }
            } else if (work.expression) {
                auto* expression = work.expression;
                // Defaults use initializer syntax, but their expected type is a
                // parameter type, not a separately registered object symbol.
                if (work.use == ExpressionUse::Default) {
                    while (auto* list = dynamic_cast<types::InitializerNode*>(expression)) {
                        if (list->expressions().size() != 1) break;
                        expression = list->expressions().front();
                    }
                }
                work.value = Checker.checkValue(expression);
                if (work.use == ExpressionUse::Default) {
                    work.expression->semanticInfo().Type = work.value.type.value;
                    work.expression->semanticInfo().Annotated = true;
                }
            }
        }
        for (const auto& selection : Switches) {
            const auto& condition = Expressions[selection.condition].value;
            if (TypeSystem.isIntegral(condition.type)) for (auto index : selection.cases)
                Expressions[index].expected = Checker.promotedType(condition.type);
        }
        // Rewriting follows typing so cached intrinsic facts remain available
        // to read-only checks. Each work item's owner replaces the actual AST
        // slot, not merely the queue pointer (important for IR traversal).
        if (SymbolTable.diagnostics().empty()) for (auto& work : Expressions) {
            if (!work.expression) continue;
            select(work.context);
            TypeId target;
            if ((work.use == ExpressionUse::Return || work.use == ExpressionUse::Default) &&
                !TypeSystem.isVoid(work.expected) &&
                Checker.convertible(work.value.type, work.expected, work.value)) target = work.expected;
            if (work.use == ExpressionUse::Condition && work.value.type.value &&
                (TypeSystem.arithmetic(work.value.type) || TypeSystem.isIntegral(work.value.type) ||
                 TypeSystem.get(TypeSystem.unConst(work.value.type)).kind == TypeKind::Pointer))
                target = TypeSystem.builtin(Builtin::Bool);
            if (work.use == ExpressionUse::Case && TypeSystem.isIntegral(work.value.type)) target = work.expected;
            if (work.use == ExpressionUse::Switch && TypeSystem.isIntegral(work.value.type))
                target = Checker.promotedType(work.value.type);
            auto* original = work.expression;
            work.expression = Checker.materializeConversions(original, work.use != ExpressionUse::Initializer && work.source != original, target);
            auto* owner = const_cast<types::ASTNode*>(work.source);
            if (auto* parameter = dynamic_cast<types::FunctionParameter*>(owner)) owner = parameter->getDeclaration();
            if (owner) owner->rewriteExpressions([&](auto* child) { return child == original ? work.expression : child; });
        }
        Checker.endAnnotation();
        Phase = SemanticPhase::Annotated;
        refreshDiagnostics();
        return Diagnostics.empty();
    }
}
```

## `src/semantic/checks.cc`

```cpp
#include "semantic_analyzer.hh"
#include <algorithm>
#include <unordered_map>

namespace baka::semantic {
    namespace {
        // This helper reads bound IDs and annotations only. In particular, &local
        // is not a constant address for a static initializer simply because its
        // syntax contains '&'.
        bool StaticAddress(const types::ExpressionNode* expression, const Symbols& symbols) {
            if (!expression) return false;
            if (auto* cast = dynamic_cast<const types::ImplicitCastNode*>(expression))
                return StaticAddress(cast->getOperand(), symbols);
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                const auto& payload = primary->getExpression();
                if (auto* name = std::get_if<types::IdentifierNode*>(&payload); name && *name) {
                    if ((*name)->getBindings().size() != 1) return false;
                    const auto& symbol = symbols.get({(*name)->getBindings().front()});
                    return symbol.isStatic || symbols.scope(symbol.scope).kind == ScopeKind::Global ||
                           symbol.kind == SymbolKind::Function || symbol.kind == SymbolKind::BuiltinFunction;
                }
                if (auto* literal = std::get_if<types::ConstantNode*>(&payload); literal && *literal)
                    return dynamic_cast<const types::ConstantTNode<base::SLString>*>(*literal);
            }
            if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) return StaticAddress(member->getBase(), symbols);
            if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression))
                return index->getIndex()->semanticInfo().ConstantExpression && StaticAddress(index->getBase(), symbols);
            return false;
        }
        bool ConstantInitializer(const types::ExpressionNode* expression, const Symbols& symbols, const Types& types) {
            if (!expression) return true;
            if (auto* cast = dynamic_cast<const types::ImplicitCastNode*>(expression))
                return ConstantInitializer(cast->getOperand(), symbols, types);
            if (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                for (auto* child : list->expressions()) if (!ConstantInitializer(child, symbols, types)) return false;
                return true;
            }
            const auto& metadata = expression->semanticInfo();
            if (metadata.ConstantExpression || metadata.NullPointerConstant) return true;
            if (auto* unary = dynamic_cast<const types::FactorNode*>(expression);
                unary && unary->getUnaryOp() == types::ASTUnaryOp::OP_ADDR_OF)
                return StaticAddress(unary->getOperand(), symbols);
            auto kind = types.get(types.unConst({metadata.Type})).kind;
            if ((kind == TypeKind::Array || kind == TypeKind::Function) && StaticAddress(expression, symbols)) return true;
            if (auto* operation = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
                auto op = operation->getBinaryOp();
                if ((op == types::OP_ADD || op == types::OP_SUB) &&
                    types.get(types.unConst({metadata.Type})).kind == TypeKind::Pointer) {
                    return (ConstantInitializer(operation->getLeft(), symbols, types) &&
                            operation->getRight()->semanticInfo().ConstantExpression) ||
                           (op == types::OP_ADD && operation->getLeft()->semanticInfo().ConstantExpression &&
                            ConstantInitializer(operation->getRight(), symbols, types));
                }
            }
            // Array/function designators and string literals can denote static
            // addresses; scalar global variable reads are not constant values.
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                auto payload = primary->getExpression();
                if (auto* literal = std::get_if<types::ConstantNode*>(&payload); literal && *literal)
                    return dynamic_cast<const types::ConstantTNode<base::SLString>*>(*literal);
            }
            return false;
        }
        int64_t CaseValue(int64_t value, TypeId type, const Types& types) {
            auto info = types.arithmetic(type);
            if (!info || info->bits < 32) info = types.arithmetic(types.builtin(Builtin::Int));
            if (info->bits == 64) return value;
            auto bits = static_cast<uint64_t>(value) & ((uint64_t{1} << info->bits) - 1);
            if (info->isSigned && (bits & (uint64_t{1} << (info->bits - 1))))
                bits |= ~((uint64_t{1} << info->bits) - 1);
            return static_cast<int64_t>(bits);
        }
    }

    // This method is const by construction. It cannot resolve names, register
    // declarations, annotate nodes, infer types, or evaluate expressions. All
    // expression facts and control-flow targets arrive from the earlier queues.
    std::vector<Diagnostic> SemanticAnalyzer::checkReadOnly() const {
        std::vector<Diagnostic> diagnostics;
        auto report = [&](std::string code, std::string message, const types::ASTNode* source,
                          const types::ASTNode* previous = nullptr) {
            diagnostics.push_back({std::move(code), std::move(message), source, previous});
        };
        for (const auto& [node, message] : ResolutionErrors) report("control-flow", message, node);
        for (auto* node : InvalidNodes) report("function", "nested functions or unsupported statement are not allowed", node);
        for (const auto& work : Expressions) {
            const auto& value = work.value;
            switch (work.use) {
            case ExpressionUse::Condition:
                if (!value.error && !TypeSystem.arithmetic(value.type) && !TypeSystem.isIntegral(value.type) &&
                    TypeSystem.get(TypeSystem.unConst(value.type)).kind != TypeKind::Pointer)
                    report("condition", "condition requires a scalar value", work.expression);
                break;
            case ExpressionUse::Initializer:
                if (work.symbol.value) {
                    auto type = SymbolTable.get(work.symbol).type;
                    if (!work.expression && TypeSystem.isConst(type))
                        report("initializer", "const object requires an initializer", work.source);
                    if (!TypeSystem.layout(type)) report("complete", "object requires a complete storage type", work.source);
                    if (work.staticStorage && work.expression && !ConstantInitializer(work.expression, SymbolTable, TypeSystem))
                        report("initializer", "static storage initializer must be a constant expression", work.source);
                }
                break;
            case ExpressionUse::Default:
                if (!value.error && !Checker.convertible(value.type, work.expected, value))
                    report("default-argument", "default argument is not convertible to the parameter type", work.expression);
                break;
            case ExpressionUse::Return:
                if (TypeSystem.isVoid(work.expected)) {
                    if (work.expression) report("return", "void function cannot return a value", work.expression);
                } else if (!work.expression) report("return", "non-void function must return a value", work.source);
                else if (!value.error && !Checker.convertible(value.type, work.expected, value))
                    report("return", "returned expression is not convertible to the function result", work.expression);
                break;
            default: break;
            }
        }
        for (const auto& selection : Switches) {
            const auto& condition = Expressions[selection.condition].value;
            if (condition.error) continue; // already diagnosed; avoid a type cascade
            if (!TypeSystem.isIntegral(condition.type)) {
                report("switch", "switch requires an integral condition", Expressions[selection.condition].expression);
                continue;
            }
            std::unordered_map<int64_t, const types::ASTNode*> cases;
            for (auto index : selection.cases) {
                const auto& work = Expressions[index];
                const auto& value = work.value;
                if (value.error) continue;
                if (!value.constantExpression || value.constant.state != ConstantState::Value || !TypeSystem.isIntegral(value.type)) {
                    report("case", "case label requires an integral constant", work.expression);
                    continue;
                }
                auto normalized = CaseValue(value.constant.value, condition.type, TypeSystem);
                auto [prior, inserted] = cases.emplace(normalized, work.expression);
                if (!inserted) report("case", "duplicate case label after conversion", work.expression, prior->second);
            }
            for (size_t i = 1; i < selection.defaults.size(); ++i)
                report("default", "duplicate default label", selection.defaults[i], selection.defaults.front());
        }
        for (const auto& flow : Flows) {
            std::unordered_map<std::string, const LabelWork*> labels;
            for (const auto& label : flow.labels) {
                auto name = std::string(label.label->getTag()->GetName());
                auto [prior, inserted] = labels.emplace(name, &label);
                if (!inserted) report("label", "duplicate label '" + name + "'", label.label, prior->second->label);
            }
            for (const auto& jump : flow.jumps) {
                if (!jump.target) {
                    std::string message = "no enclosing loop or switch for break";
                    if (dynamic_cast<const types::GotoStatementNode*>(jump.statement)) message = "goto references an undeclared label";
                    else if (dynamic_cast<const types::ContinueStatementNode*>(jump.statement)) message = "no active loop for continue or its named target";
                    else if (auto* statement = dynamic_cast<const types::BreakStatementNode*>(jump.statement); statement && statement->getLabel())
                        message = "no active loop label for break";
                    report("jump", message, jump.statement);
                } else if (auto* go = dynamic_cast<const types::GotoStatementNode*>(jump.statement)) {
                    const auto* target = labels.at(std::string(go->getLabel()->GetName()));
                    for (auto symbol : target->initialized) if (std::find(jump.initialized.begin(), jump.initialized.end(), symbol) == jump.initialized.end()) {
                        report("goto", "goto jumps past initialization of '" + SymbolTable.get(symbol).name + "'", go);
                        break;
                    }
                }
            }
        }
        return diagnostics;
    }

    bool SemanticAnalyzer::check() {
        if (Phase != SemanticPhase::Annotated) return false;
        auto findings = checkReadOnly();
        Diagnostics.insert(Diagnostics.end(), findings.begin(), findings.end());
        Phase = SemanticPhase::Checked;
        return Diagnostics.empty();
    }
}
```

## `src/semantic/conversions.cc`

```cpp
#include "expressions.hh"
#include "memory/custom_arenas.hh"
#include <cmath>
#include <limits>

namespace baka::semantic {
    namespace {
        bool Assignment(types::ASTBinaryOp op) {
            return op >= types::OP_ASSIGN && op <= types::OP_RSHIFT_ASSIGN;
        }
        bool Comparison(types::ASTBinaryOp op) { return op >= types::OP_EQ && op <= types::OP_GE; }
        types::ASTBinaryOp Underlying(types::ASTBinaryOp op) {
            switch (op) {
            case types::OP_ADD_ASSIGN: return types::OP_ADD;
            case types::OP_SUB_ASSIGN: return types::OP_SUB;
            case types::OP_MUL_ASSIGN: return types::OP_MUL;
            case types::OP_DIV_ASSIGN: return types::OP_DIV;
            case types::OP_MOD_ASSIGN: return types::OP_MOD;
            case types::OP_AND_ASSIGN: return types::OP_BIT_AND;
            case types::OP_OR_ASSIGN: return types::OP_BIT_OR;
            case types::OP_XOR_ASSIGN: return types::OP_BIT_XOR;
            case types::OP_LSHIFT_ASSIGN: return types::OP_LSHIFT;
            case types::OP_RSHIFT_ASSIGN: return types::OP_RSHIFT;
            default: return op;
            }
        }
    }

    void ExpressionChecker::planConversion(const types::ExpressionNode* expression, TypeId target) {
        if (!Annotating || !expression) return;
        // InitializerNode is syntax, never a runtime operand. Push a scalar
        // destination down through transparent/single-element brace wrappers.
        while (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
            if (list->expressions().size() != 1) return;
            expression = list->expressions().front();
        }
        ConversionTargets[expression] = TypeSystem.unConst(target);
    }

    bool ExpressionChecker::isBoundInstanceFunction(const types::ExpressionNode* expression) const {
        while (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
            if (comma->getExpressions().size() != 1) break;
            expression = comma->getExpressions().front();
        }
        if (!expression) return false;
        const auto& metadata = expression->semanticInfo();
        if (!metadata.MemberOwner || !metadata.MemberIndex || !metadata.Type ||
            TypeSystem.get(TypeSystem.unConst({metadata.Type})).kind != TypeKind::Function) return false;
        const auto& members = TypeSystem.get({metadata.MemberOwner}).members;
        if (metadata.MemberIndex > members.size()) return false;
        const auto& member = members[metadata.MemberIndex - 1];
        return !member.isStatic && TypeSystem.get(TypeSystem.unConst(member.type)).kind == TypeKind::Function;
    }

    types::ExpressionNode* ExpressionChecker::wrapConversion(types::ExpressionNode* expression, TypeId target,
                                                            types::ImplicitConversionKind kind) {
        auto* cast = base::ASTNodeArena::getInstance().Alloc<types::ImplicitCastNode>(expression, expression->semanticInfo().Type, target.value, kind);
        cast->semanticInfo() = expression->semanticInfo();
        cast->semanticInfo().Type = target.value;
        cast->semanticInfo().LValue = false;
        cast->semanticInfo().Annotated = true;
        auto& metadata = cast->semanticInfo();
        auto arithmetic = TypeSystem.arithmetic(target);
        if (kind != types::ImplicitConversionKind::LValueToRValue && arithmetic) {
            std::optional<long double> number;
            if (metadata.HasFloatingConstant) number = metadata.FloatingConstant;
            else if (metadata.HasConstant) number = static_cast<long double>(metadata.Constant);
            if (target == TypeSystem.builtin(Builtin::Bool) && number) {
                metadata.Constant = *number != 0; metadata.HasConstant = true;
                metadata.HasFloatingConstant = false;
            } else if (arithmetic->floating && number) {
                metadata.FloatingConstant = target == TypeSystem.builtin(Builtin::Float) ?
                    static_cast<long double>(static_cast<float>(*number)) :
                    static_cast<long double>(static_cast<double>(*number));
                metadata.HasFloatingConstant = true; metadata.HasConstant = false;
            } else if (!arithmetic->floating) {
                if (metadata.HasFloatingConstant) {
                    auto number = metadata.FloatingConstant;
                    // Avoid undefined host casts at the integer limits. Runtime
                    // conversion remains explicit even when we cannot fold it.
                    metadata.HasConstant = std::isfinite(number) &&
                        number >= static_cast<long double>(std::numeric_limits<int64_t>::min()) &&
                        number < -static_cast<long double>(std::numeric_limits<int64_t>::min());
                    if (metadata.HasConstant) metadata.Constant = static_cast<int64_t>(number);
                    metadata.HasFloatingConstant = false;
                    if (!metadata.HasConstant) metadata.ConstantExpression = false;
                }
                if (metadata.HasConstant && arithmetic->bits < 64) {
                    auto bits = static_cast<uint64_t>(metadata.Constant) & ((uint64_t{1} << arithmetic->bits) - 1);
                    if (arithmetic->isSigned && (bits & (uint64_t{1} << (arithmetic->bits - 1))))
                        bits |= ~((uint64_t{1} << arithmetic->bits) - 1);
                    metadata.Constant = static_cast<int64_t>(bits);
                }
            }
            metadata.NullPointerConstant = !arithmetic->floating && metadata.ConstantExpression &&
                metadata.HasConstant && metadata.Constant == 0;
        } else if (isPointer(target)) {
            metadata.NullPointerConstant = false;
            metadata.HasConstant = metadata.HasFloatingConstant = false;
        }
        // Binding identity belongs to the operand, not the converted value.
        cast->semanticInfo().Symbol = 0;
        auto span = expression->getSourceSpan();
        if (span.Valid) cast->setSourceSpan(span.Line, span.Col);
        return cast;
    }

    types::ExpressionNode* ExpressionChecker::convertValue(types::ExpressionNode* expression, TypeId target) {
        using K = types::ImplicitConversionKind;
        if (!expression || expression->semanticInfo().Error || !expression->semanticInfo().Type) return expression;
        if (isBoundInstanceFunction(expression)) return expression;
        TypeId source{expression->semanticInfo().Type};
        auto kind = TypeSystem.get(TypeSystem.unConst(source)).kind;
        if (kind == TypeKind::Array) {
            expression = wrapConversion(expression, decay(source), K::ArrayToPointer);
        } else if (kind == TypeKind::Function) {
            expression = wrapConversion(expression, decay(source), K::FunctionToPointer);
        } else if (expression->semanticInfo().LValue) {
            expression = wrapConversion(expression, TypeSystem.unConst(source), K::LValueToRValue);
        }
        source = {expression->semanticInfo().Type};
        target = TypeSystem.unConst(target);
        if (!target.value || source == target) return expression;
        K conversion = K::NumericConversion;
        if (target == TypeSystem.builtin(Builtin::Bool)) conversion = K::BooleanConversion;
        else if (isPointer(target) && !isPointer(source)) conversion = K::NullToPointer;
        else if (isPointer(source) && isPointer(target)) {
            auto from = TypeSystem.get(TypeSystem.unConst(source)).element;
            auto to = TypeSystem.get(target).element;
            conversion = TypeSystem.unConst(from) == TypeSystem.unConst(to) ? K::QualificationConversion : K::PointerConversion;
        } else if (TypeSystem.unConst(source) == target) conversion = K::QualificationConversion;
        else if (isInteger(source) && promote(source) == target) conversion = K::IntegralPromotion;
        return wrapConversion(expression, target, conversion);
    }

    // Materialize only after successful typing. This walk is over collected
    // expression work, never another declaration/name-resolution traversal. Each
    // parent chooses value/address/unevaluated context for its children.
    types::ExpressionNode* ExpressionChecker::materializeImpl(types::ExpressionNode* expression, bool value, TypeId target) {
        if (!expression || expression->semanticInfo().Error) return expression;
        if (dynamic_cast<types::ImplicitCastNode*>(expression) || dynamic_cast<types::CompoundAssignmentNode*>(expression))
            return value ? convertValue(expression, target) : expression;
        if (auto it = ConversionTargets.find(expression); it != ConversionTargets.end() && !target.value) target = it->second;
        const TypeId own{expression->semanticInfo().Type};
        const auto boolean = TypeSystem.builtin(Builtin::Bool);
        if (auto* call = dynamic_cast<types::FunctionCallPostfixExpr*>(expression)) {
            TypeId signature{call->semanticInfo().CallSignature};
            if (signature.value) {
                const auto info = TypeSystem.get(signature);
                auto rewriteArgument = [&](types::ExpressionNode* argument, size_t i) {
                    TypeId expected;
                    if (i < info.parameters.size()) expected = info.parameters[i];
                    else if (argument && argument->semanticInfo().Type) {
                        auto type = TypeSystem.unConst(decay(TypeId{argument->semanticInfo().Type}));
                        // Default argument promotions apply only to ellipsis args.
                        expected = type == TypeSystem.builtin(Builtin::Float) ? TypeSystem.builtin(Builtin::Double) : promote(type);
                        if (!isArithmetic(type)) expected = {};
                    }
                    return materializeImpl(argument, true, expected);
                };
                // Bound instance methods retain their receiver and selected member
                // identity. Static/free functions decay; pointer fields are loaded.
                auto rewriteCallee = [&](types::ExpressionNode* callee) {
                    if (call->semanticInfo().Construction) return callee;
                    return materializeImpl(callee, !isBoundInstanceFunction(callee));
                };
                // The outer comma node is an argument-list container. A nested
                // comma expression is one argument and stays nested.
                auto* arguments = call->getArguments();
                if (auto* list = dynamic_cast<types::CommaExpressionNode*>(arguments)) {
                    size_t i = 0;
                    list->rewriteExpressions([&](auto* child) { return rewriteArgument(child, i++); });
                } else if (arguments) {
                    call->rewriteExpressions([&](auto* child) {
                        return child == arguments ? rewriteArgument(child, 0) :
                            rewriteCallee(child);
                    });
                    return value ? convertValue(expression, target) : expression;
                }
                // A constructor's type-name callee is not a function designator.
                if (!call->semanticInfo().Construction) call->rewriteExpressions([&](auto* child) {
                    return child == arguments ? child : rewriteCallee(child);
                });
            }
        } else if (auto* binary = dynamic_cast<types::BinaryExpressionNode*>(expression)) {
            auto op = binary->getBinaryOp();
            auto* left = binary->getLeft(); auto* right = binary->getRight();
            TypeId lt{left->semanticInfo().Type}, rt{right->semanticInfo().Type};
            lt = decay(lt); rt = decay(rt);
            if (Assignment(op)) {
                auto* location = materializeImpl(left, false);
                if (op == types::OP_ASSIGN) {
                    binary->rewriteExpressions([&](auto* child) {
                        return child == left ? location : materializeImpl(child, true, TypeSystem.unConst(lt));
                    });
                } else {
                    // A saved-address placeholder avoids duplicating a[i++] or
                    // *get_pointer() when spelling the promoted read and store.
                    auto* load = base::ASTNodeArena::getInstance().Alloc<types::CompoundTargetNode>();
                    load->semanticInfo() = location->semanticInfo();
                    auto targetSpan = location->getSourceSpan();
                    if (targetSpan.Valid) load->setSourceSpan(targetSpan.Line, targetSpan.Col);
                    auto underlying = Underlying(op);
                    bool shift = underlying == types::OP_LSHIFT || underlying == types::OP_RSHIFT;
                    TypeId computation = isPointer(lt) ? lt : shift ? promote(lt) : usualArithmetic(lt, rt, expression);
                    auto* lhs = convertValue(load, computation);
                    auto* rhs = materializeImpl(right, true, isPointer(lt) || shift ? promote(rt) : computation);
                    auto* operation = base::ASTNodeArena::getInstance().Alloc<types::BinaryExpressionNode>(underlying, lhs, rhs);
                    operation->semanticInfo() = expression->semanticInfo();
                    operation->semanticInfo().Type = computation.value;
                    operation->semanticInfo().LValue = false;
                    auto span = expression->getSourceSpan();
                    if (span.Valid) operation->setSourceSpan(span.Line, span.Col);
                    auto* assigned = convertValue(operation, TypeSystem.unConst(lt));
                    auto* replacement = base::ASTNodeArena::getInstance().Alloc<types::CompoundAssignmentNode>(location, assigned);
                    replacement->semanticInfo() = expression->semanticInfo();
                    if (span.Valid) replacement->setSourceSpan(span.Line, span.Col);
                    expression = replacement;
                }
            } else {
                TypeId leftTarget, rightTarget;
                if (op == types::OP_LOGICAL_AND || op == types::OP_LOGICAL_OR) leftTarget = rightTarget = boolean;
                else if (op == types::OP_LSHIFT || op == types::OP_RSHIFT) { leftTarget = promote(lt); rightTarget = promote(rt); }
                else if (isArithmetic(lt) && isArithmetic(rt) && op != types::OP_COMMA) {
                    leftTarget = rightTarget = usualArithmetic(lt, rt, expression);
                } else if (Comparison(op)) {
                    if (isPointer(lt) && !isPointer(rt)) rightTarget = lt;
                    else if (isPointer(rt) && !isPointer(lt)) leftTarget = rt;
                    else if (isPointer(lt) && isPointer(rt)) {
                        auto lp = TypeSystem.get(lt).element, rp = TypeSystem.get(rt).element;
                        auto element = TypeSystem.isVoid(lp) ? lp : rp;
                        element = TypeSystem.constant(element, TypeSystem.isConst(lp) || TypeSystem.isConst(rp));
                        leftTarget = rightTarget = TypeSystem.pointer(element);
                    }
                } else if (isPointer(lt) && isInteger(rt)) rightTarget = promote(rt);
                else if (isPointer(rt) && isInteger(lt)) leftTarget = promote(lt);
                binary->rewriteExpressions([&](auto* child) {
                    return materializeImpl(child, true, child == left ? leftTarget : rightTarget);
                });
            }
        } else if (auto* unary = dynamic_cast<types::FactorNode*>(expression)) {
            auto op = unary->getUnaryOp();
            bool reads = op != types::ASTUnaryOp::OP_ADDR_OF && op != types::ASTUnaryOp::OP_INC && op != types::ASTUnaryOp::OP_DEC;
            TypeId expected;
            if (op == types::ASTUnaryOp::OP_LOGICAL_NOT) expected = boolean;
            if (op == types::ASTUnaryOp::OP_TILDE || op == types::ASTUnaryOp::OP_UNARY_PLUS || op == types::ASTUnaryOp::OP_UNARY_MINUS)
                expected = own;
            unary->rewriteExpressions([&](auto* child) { return materializeImpl(child, reads, expected); });
        } else if (auto* index = dynamic_cast<types::IndexPostfixExpr*>(expression)) {
            auto* offset = index->getIndex();
            TypeId offsetType{offset->semanticInfo().Type};
            index->rewriteExpressions([&](auto* child) {
                return materializeImpl(child, true, child == offset ? promote(offsetType) : TypeId{});
            });
        } else if (auto* ternary = dynamic_cast<types::TernaryOpNode*>(expression)) {
            auto* condition = ternary->getCondition();
            ternary->rewriteExpressions([&](auto* child) { return materializeImpl(child, true, child == condition ? boolean : own); });
        } else if (auto* initializer = dynamic_cast<types::InitializerNode*>(expression)) {
            initializer->rewriteExpressions([&](auto* child) { return materializeImpl(child, false, target); });
            if (target.value) initializer->semanticInfo().Type = TypeSystem.unConst(target).value;
            return expression; // brace/syntax containers never become runtime casts
        } else if (auto* comma = dynamic_cast<types::CommaExpressionNode*>(expression)) {
            // A single child is also the parser's parentheses/expression wrapper.
            // Leave its result conversion outside so sizeof/address-of keep it.
            const auto count = comma->getExpressions().size();
            comma->rewriteExpressions([&](auto* child) { return materializeImpl(child, count != 1); });
        } else if (dynamic_cast<types::SizeofNode*>(expression)) {
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, false); });
        } else if (dynamic_cast<types::UnaryPostfixExpr*>(expression) || dynamic_cast<types::MemberPostfixExpr*>(expression)) {
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, false); });
        } else {
            // Index, arrow and explicit cast operands are value uses. The explicit
            // cast itself remains distinct and performs its written conversion.
            expression->rewriteExpressions([&](auto* child) { return materializeImpl(child, true); });
        }
        // Initializer destinations also request value conversion when their parent
        // is a syntax container rather than an expression operator.
        return value || target.value ? convertValue(expression, target) : expression;
    }
}
```

# Appendix E — Bundled reference grammar, verbatim (not the baka grammar)

## `src/parser/ReferenceGrammerANSIC2011`

The source attribution/license heading is part of the file and is preserved. Filename says ANSIC2011; file itself identifies C99 TC3.

```text
Browsable C99 Grammar
CC-BY

Grammar extracted by Vadim Zaytsev, see the Grammar Zoo entry for details: c/c99/iso-9899-tc3/extracted
Source used for this grammar: ISO/IEC 9899:TC3, WG14/N1256 Committee Draft, September 2007, Annex A, pages 403–418
Summary

    Total 69 production rules with 201 top alternatives and 806 symbols.
    Vocabulary: 155 = 73 nonterminals + 82 terminals + 0 labels + 0 markers.
    Total 73 nonterminal symbols: 69 defined (translation-unit, external-declaration, function-definition, declaration-specifiers, storage-class-specifier, type-specifier, struct-or-union-specifier, struct-or-union, struct-declaration-list, struct-declaration, specifier-qualifier-list, type-qualifier, struct-declarator-list, struct-declarator, declarator, pointer, type-qualifier-list, direct-declarator, assignment-expression, conditional-expression, logical-OR-expression, logical-AND-expression, inclusive-OR-expression, exclusive-OR-expression, AND-expression, equality-expression, relational-expression, shift-expression, additive-expression, multiplicative-expression, cast-expression, unary-expression, postfix-expression, primary-expression, expression, argument-expression-list, type-name, abstract-declarator, direct-abstract-declarator, parameter-type-list, parameter-list, parameter-declaration, initializer-list, designation, designator-list, designator, constant-expression, initializer, unary-operator, assignment-operator, identifier-list, enum-specifier, enumerator-list, enumerator, typedef-name, function-specifier, declaration-list, declaration, init-declarator-list, init-declarator, compound-statement, block-item-list, block-item, statement, labeled-statement, expression-statement, selection-statement, iteration-statement, jump-statement), 1 root (translation-unit), 0 top (—), 4 bottom (string-literal, identifier 15, constant, enumeration-constant 2).
    Total 82 terminal symbols: 36 keywords ("typedef", "extern", "static" 5, "auto", "register", "void", "char", "short", "int", "long", "float", "double", "signed", "unsigned", "_Bool", "_Complex", "struct", "union", "const", "restrict", "volatile", "sizeof" 2, "enum" 3, "inline", "case", "default", "if" 2, "else", "switch", "while" 2, "do", "for" 2, "goto", "continue", "break", "return"), 0 letters (—), 0 numerics (—), 46 signs ("{" 8, "}" 8, ";" 11, "," 12, ":" 5, "*" 6, "(" 18, ")" 18, "[" 10, "]" 10, "?", "||", "&&", "|", "^", "&" 2, "==", "!=", "<", ">", "<=", ">=", "<<", ">>", "+" 2, "-" 2, "/", "%", "++" 2, "--" 2, "." 2, "->", "...", "=" 4, "~", "!", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=").

Syntax

translation-unit ::=
	external-declaration
	translation-unit external-declaration

external-declaration ::=
	function-definition
	declaration

function-definition ::=
	declaration-specifiers declarator declaration-list? compound-statement

declaration-specifiers ::=
	storage-class-specifier declaration-specifiers?
	type-specifier declaration-specifiers?
	type-qualifier declaration-specifiers?
	function-specifier declaration-specifiers?

storage-class-specifier ::=
	"typedef"
	"extern"
	"static"
	"auto"
	"register"

type-specifier ::=
	"void"
	"char"
	"short"
	"int"
	"long"
	"float"
	"double"
	"signed"
	"unsigned"
	"_Bool"
	"_Complex"
	struct-or-union-specifier
	enum-specifier
	typedef-name

struct-or-union-specifier ::=
	struct-or-union identifier? "{" struct-declaration-list "}"
	struct-or-union identifier

struct-or-union ::=
	"struct"
	"union"

struct-declaration-list ::=
	struct-declaration
	struct-declaration-list struct-declaration

struct-declaration ::=
	specifier-qualifier-list struct-declarator-list ";"

specifier-qualifier-list ::=
	type-specifier specifier-qualifier-list?
	type-qualifier specifier-qualifier-list?

type-qualifier ::=
	"const"
	"restrict"
	"volatile"

struct-declarator-list ::=
	struct-declarator
	struct-declarator-list "," struct-declarator

struct-declarator ::=
	declarator
	declarator? ":" constant-expression

declarator ::=
	pointer? direct-declarator

pointer ::=
	"*" type-qualifier-list?
	"*" type-qualifier-list? pointer

type-qualifier-list ::=
	type-qualifier
	type-qualifier-list type-qualifier

direct-declarator ::=
	identifier
	"(" declarator ")"
	direct-declarator "[" type-qualifier-list? assignment-expression? "]"
	direct-declarator "[" "static" type-qualifier-list? assignment-expression "]"
	direct-declarator "[" type-qualifier-list "static" assignment-expression "]"
	direct-declarator "[" type-qualifier-list? "*" "]"
	direct-declarator "(" parameter-type-list ")"
	direct-declarator "(" identifier-list? ")"

assignment-expression ::=
	conditional-expression
	unary-expression assignment-operator assignment-expression

conditional-expression ::=
	logical-OR-expression
	logical-OR-expression "?" expression ":" conditional-expression

logical-OR-expression ::=
	logical-AND-expression
	logical-OR-expression "||" logical-AND-expression

logical-AND-expression ::=
	inclusive-OR-expression
	logical-AND-expression "&&" inclusive-OR-expression

inclusive-OR-expression ::=
	exclusive-OR-expression
	inclusive-OR-expression "|" exclusive-OR-expression

exclusive-OR-expression ::=
	AND-expression
	exclusive-OR-expression "^" AND-expression

AND-expression ::=
	equality-expression
	AND-expression "&" equality-expression

equality-expression ::=
	relational-expression
	equality-expression "==" relational-expression
	equality-expression "!=" relational-expression

relational-expression ::=
	shift-expression
	relational-expression "<" shift-expression
	relational-expression ">" shift-expression
	relational-expression "<=" shift-expression
	relational-expression ">=" shift-expression

shift-expression ::=
	additive-expression
	shift-expression "<<" additive-expression
	shift-expression ">>" additive-expression

additive-expression ::=
	multiplicative-expression
	additive-expression "+" multiplicative-expression
	additive-expression "-" multiplicative-expression

multiplicative-expression ::=
	cast-expression
	multiplicative-expression "*" cast-expression
	multiplicative-expression "/" cast-expression
	multiplicative-expression "%" cast-expression

cast-expression ::=
	unary-expression
	"(" type-name ")" cast-expression

unary-expression ::=
	postfix-expression
	"++" unary-expression
	"--" unary-expression
	unary-operator cast-expression
	"sizeof" unary-expression
	"sizeof" "(" type-name ")"

postfix-expression ::=
	primary-expression
	postfix-expression "[" expression "]"
	postfix-expression "(" argument-expression-list? ")"
	postfix-expression "." identifier
	postfix-expression "->" identifier
	postfix-expression "++"
	postfix-expression "--"

primary-expression ::=
	identifier
	constant
	string-literal
	"(" expression ")"

expression ::=
	assignment-expression
	expression "," assignment-expression

argument-expression-list ::=
	assignment-expression
	argument-expression-list "," assignment-expression

type-name ::=
	specifier-qualifier-list abstract-declarator?

abstract-declarator ::=
	pointer
	pointer? direct-abstract-declarator

direct-abstract-declarator ::=
	"(" abstract-declarator ")"
	direct-abstract-declarator? "[" type-qualifier-list? assignment-expression? "]"
	direct-abstract-declarator? "[" "static" type-qualifier-list? assignment-expression "]"
	direct-abstract-declarator? "[" type-qualifier-list "static" assignment-expression "]"
	direct-abstract-declarator? "[" "*" "]"
	direct-abstract-declarator? "(" parameter-type-list? ")"

parameter-type-list ::=
	parameter-list
	parameter-list "," "..."

parameter-list ::=
	parameter-declaration
	parameter-list "," parameter-declaration

parameter-declaration ::=
	declaration-specifiers declarator
	declaration-specifiers abstract-declarator?

initializer-list ::=
	initializer
	initializer-list "," initializer

constant-expression ::=
	conditional-expression

initializer ::=
	assignment-expression
	"{" initializer-list "}"
	"{" initializer-list "," "}"

unary-operator ::=
	"&"
	"*"
	"+"
	"-"
	"~"
	"!"

assignment-operator ::=
	"="
	"*="
	"/="
	"%="
	"+="
	"-="
	"<<="
	">>="
	"&="
	"^="
	"|="

identifier-list ::=
	identifier
	identifier-list "," identifier

enum-specifier ::=
	"enum" identifier? "{" enumerator-list "}"
	"enum" identifier? "{" enumerator-list "," "}"
	"enum" identifier

enumerator-list ::=
	enumerator
	enumerator-list "," enumerator

enumerator ::=
	enumeration-constant
	enumeration-constant "=" constant-expression

typedef-name ::=
	identifier

function-specifier ::=
	"inline"

declaration-list ::=
	declaration
	declaration-list declaration

declaration ::=
	declaration-specifiers init-declarator-list? ";"

init-declarator-list ::=
	init-declarator
	init-declarator-list "," init-declarator

init-declarator ::=
	declarator
	declarator "=" initializer

compound-statement ::=
	"{" block-item-list? "}"

block-item-list ::=
	block-item
	block-item-list block-item

block-item ::=
	declaration
	statement

statement ::=
	labeled-statement
	compound-statement
	expression-statement
	selection-statement
	iteration-statement
	jump-statement

labeled-statement ::=
	identifier ":" statement
	"case" constant-expression ":" statement
	"default" ":" statement

expression-statement ::=
	expression? ";"

selection-statement ::=
	"if" "(" expression ")" statement
	"if" "(" expression ")" statement "else" statement
	"switch" "(" expression ")" statement

iteration-statement ::=
	"while" "(" expression ")" statement
	"do" statement "while" "(" expression ")" ";"
	"for" "(" expression? ";" expression? ";" expression? ")" statement
	"for" "(" declaration expression? ";" expression? ")" statement

jump-statement ::=
	"goto" identifier ";"
	"continue" ";"
	"break" ";"
	"return" expression? ";"

GrammarLabMaintained by Dr. Vadim Zaytsev a.k.a. @grammarware. Last updated in September 2015. [↑]
```

# Appendix F — Driver/diagnostic and allocation definitions, verbatim

## `src/driver/driver.hh`

```cpp
#pragma once


namespace baka {
    namespace driver {
        int run(int argc, char* argv[]);
    }
}
```

## `src/driver/gctx.hh`

```cpp
#pragma once
#include "types/driver/defs.hh"
#include "base/line_index.hh"
#include "mmap_file.hh"
#include "types/exceptions.hh"
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace baka {
    namespace driver {
        class Gctx;

        struct Gctx_t {
            bool DebugMode = true;
            bool ShouldOptimize = true;
            bool VerboseMode = true;
            BuildType BuildType_v;
            std::string SourceFilePath;

            Stage TillStage = Stage::SEMANTIC;

            // external stuff:
            std::shared_ptr<baka::base::MappedFile> MappedFilePtr;
            std::shared_ptr<baka::base::LineIndex> LineIndexPtr;

            // exceptions:
            std::vector<exceptions::CompilerLineError> CompilerLineErrors;
            std::vector<exceptions::CompilerLineWarning> CompilerLineWarnings;

            std::vector<exceptions::CompilerGlobalError> CompilerGlobalErrors;
            std::vector<exceptions::CompilerGlobalWarning> CompilerGlobalWarnings;

        private:
            Gctx_t() = default;

            friend Gctx;


            void Print() const;
        };

        constexpr size_t NO_LINE = 0;
        constexpr size_t NO_COL = 0;


        class Gctx {
            inline static std::shared_mutex mut;
            inline static Gctx_t gctx;

        public:
            static const Gctx_t& GetGctxRO() {
                return gctx;
            }

            static std::shared_lock<std::shared_mutex> GetROLock() {
                return std::shared_lock(mut);
            }


            template <class T>
                requires std::is_invocable_v<T, Gctx_t&>
            static auto ModifyGctx(T fn) {
                std::unique_lock lg(mut);
                return fn(gctx);
            }

            static void Print();

            static bool isVerbose();
            static bool isDebug();
            static Stage TillStage();
            static std::string_view GetSourceFilePath();
            static std::shared_ptr<base::LineIndex> GetLineIndex();

            // external stuff:
            static void AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr);
            static void AttachLineIndex(std::shared_ptr<base::LineIndex> LineIndexPtr);

            // exception stuff:
            static void GenerateLineWarning(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                        driver::Stage Stage, bool isAtEnd = false);
            static void GenerateLineError(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                      driver::Stage Stage, bool isAtEnd = false);

            static void GenerateGlobalWarning(std::string Message,
                                        driver::Stage Stage);
            static void GenerateGlobalError(std::string Message,
                                      driver::Stage Stage);

            static bool ErrorFound();

            static void ReserveDefectsVec(size_t ReserveSz) {
                ModifyGctx([&](Gctx_t& gctx) {
                    gctx.CompilerLineErrors.reserve(ReserveSz);
                    gctx.CompilerLineWarnings.reserve(ReserveSz);
                    gctx.CompilerGlobalErrors.reserve(ReserveSz);
                    gctx.CompilerGlobalWarnings.reserve(ReserveSz);
                });
            }
    };

}

}
```

## `src/driver/gctx.cc`

```cpp
#include "gctx.hh"
#include "line_index.hh"
#include "types/token/token.hh"
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// getters:

std::string_view baka::driver::Gctx::GetSourceFilePath() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().SourceFilePath;
}

bool baka::driver::Gctx::isVerbose() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().VerboseMode;
}

bool baka::driver::Gctx::isDebug() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().BuildType_v == BuildType::DEBUG;
}

baka::driver::Stage baka::driver::Gctx::TillStage() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().TillStage;
}

std::shared_ptr<baka::base::LineIndex> baka::driver::Gctx::GetLineIndex() {
    auto lg = Gctx::GetROLock();
    return Gctx::GetGctxRO().LineIndexPtr;
}

// attach globals:

void baka::driver::Gctx::AttachMappedFile(std::shared_ptr<base::MappedFile> MappedFilePtr) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.MappedFilePtr = std::move(MappedFilePtr);
    });
}

void baka::driver::Gctx::AttachLineIndex(std::shared_ptr<base::LineIndex> LineIndexPtr) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.LineIndexPtr = std::move(LineIndexPtr);
    });
}

// generate errors:

void baka::driver::Gctx::GenerateGlobalError(std::string Message, driver::Stage Stage) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.CompilerGlobalErrors.emplace_back(gctx.SourceFilePath, Message, Stage);
    });
}

void baka::driver::Gctx::GenerateGlobalWarning(std::string Message, driver::Stage Stage) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        gctx.CompilerGlobalWarnings.emplace_back(gctx.SourceFilePath, Message, Stage);
    });
}


void baka::driver::Gctx::GenerateLineError(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                       driver::Stage Stage, bool isAtEnd) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        types::TokenSourceLocation TokenLoc = {gctx.SourceFilePath, LineNo, ColNo};
        gctx.CompilerLineErrors.emplace_back(TokenLoc, LineCtx_v, Message, Stage, isAtEnd);
    });
}

void baka::driver::Gctx::GenerateLineWarning(size_t LineNo, size_t ColNo, base::LineCtx LineCtx_v, std::string Message,
                                         driver::Stage Stage, bool isAtEnd) {
    Gctx::ModifyGctx([&](Gctx_t& gctx) {
        types::TokenSourceLocation TokenLoc = {gctx.SourceFilePath, LineNo, ColNo};
        gctx.CompilerLineWarnings.emplace_back(TokenLoc, LineCtx_v, Message, Stage, isAtEnd);
    });
}

// utils:

bool baka::driver::Gctx::ErrorFound() {
    auto lg = Gctx::GetROLock();
    return !Gctx::GetGctxRO().CompilerLineErrors.empty() || !Gctx::GetGctxRO().CompilerLineWarnings.empty();
}


void baka::driver::Gctx::Print() {
    auto lg = Gctx::GetROLock();
    auto gctx = Gctx::GetGctxRO();

    std::cout << "Gctx::VerboseMode: " << gctx.VerboseMode << std::endl;
    std::cout << "Gctx::BuildType: " << to_string_view(gctx.BuildType_v) << std::endl;
    std::cout << "Gctx::UptoStage: " << to_string_view(gctx.TillStage) << std::endl;
    std::cout << "Gctx::SourceFile: " << gctx.SourceFilePath << std::endl;
}
```

## `src/driver/args.cc`

```cpp
#include "types/driver/defs.hh"
#include "gctx.hh"
#include <cstddef>
#include "types/exceptions.hh"


// cli args cases:
// --lex
// --parse
// --codegen
//
// --build_type
// --verbose
// passed by tf: '{self.cc} {options} {source_file}'


namespace baka {
    namespace driver {
        void ParseArgs(int argc, char** argv) {
            if (argc == 1) {
                throw baka::exceptions::DriverError("no input file");
            }
            std::size_t idx = 1;
            // TODO: maybe change these defaults later
            Stage UptoStage = Stage::SEMANTIC;
            baka::driver::BuildType BuildType = BuildType::DEBUG;
            std::string SourceFile;
            bool VerboseMode = false;
            bool SourceFound = false;

            while (idx < argc) {
                std::string_view curr = argv[idx];

                if (curr == "--lex") {
                    UptoStage = Stage::LEX;
                }
                else if (curr == "--parse") {
                    UptoStage = Stage::PARSE;
                }
                else if (curr == "--semantic") {
                    UptoStage = Stage::SEMANTIC;
                }
                else if (curr == "--codegen") {
                    UptoStage = Stage::CODEGEN;
                }
                else if (curr == "--build_type") {
                    idx++;
                    if (idx < argc) {
                        std::string_view type = argv[idx];

                        if (type == "release") {
                            BuildType = BuildType::RELEASE;
                        }
                        else if (type == "debug") {
                            BuildType = BuildType::DEBUG;
                        }
                        else {
                            throw baka::exceptions::DriverError("invalid built type for --build_type");
                        }
                    }
                    else {
                        throw baka::exceptions::DriverError("no built type specified for --build_type");
                    }
                }
                else if (curr == "--verbose") {
                    VerboseMode = true;
                }
                else {
                    if (idx == argc - 1) {
                        // source file
                        SourceFile = std::string(argv[idx]);
                        SourceFound = true;
                    }
                    else {
                        throw baka::exceptions::DriverError("invalid argument");
                    }
                }
                idx++;
            }

            if (!SourceFound) {
                throw baka::exceptions::DriverError("no source file specified");
            }


            Gctx::ModifyGctx([&](Gctx_t& gctx) {
                gctx.SourceFilePath = std::move(SourceFile);
                gctx.BuildType_v = BuildType;
                gctx.TillStage = UptoStage;
                gctx.VerboseMode = VerboseMode;
            });

            if (Gctx::isVerbose()) {
                Gctx::Print();
            }
        }
    }
}
```

## `src/driver/driver.cc`

```cpp
#include "driver.hh"
#include "gctx.hh"
#include "args.cc"
#include <filesystem>
#include <iostream>
#include <memory>
#include <ostream>
#include <string_view>
#include <vector>
#include "heuristics.hh"
#include "lexer/lexer.hh"
#include "base/base.hh"
#include "parser/parser.hh"
#include "semantic/semantic_analyzer.hh"
#include "types/driver/defs.hh"
#include "line_index.hh"
#include "mmap_file.hh"
#include "types/exceptions.hh"


namespace {
    void ValidateSourceFile() {
        std::filesystem::path path = baka::driver::Gctx::GetSourceFilePath();

        if (!baka::base::ValidateFile(path)) {
            throw baka::exceptions::DriverError("source file does not exist: " + path.string());
        }
    }

    std::string_view ReadSourceFile() {
        std::filesystem::path path = baka::driver::Gctx::GetSourceFilePath();

        std::shared_ptr<baka::base::MappedFile> MappedFilePtr = std::make_shared<baka::base::MappedFile>(path);
        baka::driver::Gctx::AttachMappedFile(MappedFilePtr);

        std::shared_ptr<baka::base::LineIndex> LineIndexPtr = std::make_shared<baka::base::LineIndex>(
            MappedFilePtr->View());
        baka::driver::Gctx::AttachLineIndex(LineIndexPtr);

        return MappedFilePtr->View();
    }

    size_t PrintErrorsIfAny() {
        auto lg = baka::driver::Gctx::GetROLock();
        auto& gctx = baka::driver::Gctx::GetGctxRO();

        auto& LineErrors = gctx.CompilerLineErrors;
        auto& LineWarnings = gctx.CompilerLineWarnings;
        auto& GlobalErrors = gctx.CompilerGlobalErrors;
        auto& GlobalWarnings = gctx.CompilerGlobalWarnings;

        for (auto& i : GlobalWarnings) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;
        for (auto& i : LineWarnings) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;

        for (auto& i : GlobalErrors) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;
        for (auto& i : LineErrors) {
            std::cout << i.getMessage() << '\n';
        }
        std::cout << std::flush;

        return GlobalErrors.size() + LineErrors.size();
    }

}


int baka::driver::run(int argc, char* argv[]) {
    ParseArgs(argc, argv);

    ValidateSourceFile(); // check if exists and has perms

    bool isVerbose = Gctx::isVerbose();

    std::string_view SourceCode;
    {
        // read source code
        SourceCode = ReadSourceFile();

        auto OptimisticDefects = heuristics::OptimisticCountDefects(SourceCode);
        if(isVerbose) {
            std::cout << "Driver::OptimisticDefects: " << OptimisticDefects << std::endl;
        }
        Gctx::ReserveDefectsVec(OptimisticDefects);
        if(isVerbose) {
            std::cout << "Driver::SourceCode: " << '\n' << SourceCode << std::endl;
        }
    }

    // Preprocessor(SourceCode); dont plan to do this

    lexer::Lexed Lexed;

    {
        // lexing
        if (Gctx::TillStage() >= Stage::LEX) {
            Lexed = lexer::Tokenize(SourceCode);
        }
        else {
            return 0;
        }
        if (isVerbose) {
            std::cout << "Lexer::LexerOutput: " << Lexed.Tokens.size() << " Tokens" << '\n';
        }
        PrintTokenTableHeader(std::cout);
        for (const auto& token : Lexed.Tokens) {
            std::cout << token << '\n';
        }
        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    {
        // post lexer
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            // need to stop here
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Lexer: Lexing stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    types::ASTNode* Root = nullptr;

    {
        // parsing
        if (Gctx::TillStage() < Stage::PARSE) {
            return 0;
        }


        parser::Parser Parser(Lexed.Tokens, Lexed.Locations);
        Root = Parser.Parse();

        if(isVerbose) {
            std::cout << "Parser::AST: " << '\n';
            Root->Print();
            std::cout << std::endl;
        }
    }

    {
        // post parser
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            // need to stop here
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Parser: Parsing stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    {
        // semantic checking
        if (Gctx::TillStage() < Stage::SEMANTIC) {
            return 0;
        }

        auto* Program = dynamic_cast<types::ProgramNode*>(Root);
        if (!Program) {
            Gctx::GenerateGlobalError("semantic stage requires a program AST", Stage::SEMANTIC);
        } else {
            semantic::SemanticAnalyzer Analyzer;
            bool Analyzed = Analyzer.analyze(*Program);
            if (Analyzed) {
                std::cout << "Semantic::AST: " << '\n';
                Program->Print();
                std::cout << std::endl;
            }
            if (!Analyzed && Analyzer.diagnostics().empty()) {
                Gctx::GenerateGlobalError("semantic analysis failed", Stage::SEMANTIC);
            }

            for (const auto& Entry : Analyzer.diagnostics()) {
                auto* Source = reinterpret_cast<const types::ASTNode*>(Entry.source);
                auto Span = Source ? Source->getSourceSpan() : types::ASTSourceSpan{};
                size_t LineNo = Span.Valid ? Span.Line : 1;
                size_t ColNo = Span.Valid ? Span.Col : 1;
                base::LineCtx Context{LineNo - 1, Gctx::GetLineIndex().get()};
                Gctx::GenerateLineError(LineNo, ColNo, Context, Entry.message, Stage::SEMANTIC);
                auto* Previous = reinterpret_cast<const types::ASTNode*>(Entry.previous);
                if (Previous) {
                    auto PreviousSpan = Previous->getSourceSpan();
                    if (PreviousSpan.Valid) {
                        base::LineCtx PreviousContext{PreviousSpan.Line - 1, Gctx::GetLineIndex().get()};
                        Gctx::GenerateLineWarning(PreviousSpan.Line, PreviousSpan.Col, PreviousContext,
                                                  "previous declaration is here", Stage::SEMANTIC);
                    }
                }
            }
        }
    }

    {
        // post semantic
        size_t ErrorCount = PrintErrorsIfAny();

        if (ErrorCount > 0) {
            std::cout << "Compilation failed due to " << ErrorCount << " error(s)" << '\n';
            return 1;
        }
        else {
            if (isVerbose) {
                std::cout << "Semantic: Checking stage completed" << '\n';
            }
        }

        if (isVerbose) {
            std::cout << std::endl;
        }
    }

    return 0;
}
```

## `src/types/exceptions.hh`

```cpp
#pragma once

#include "driver/defs.hh"
#include "line_index.hh"
#include "types/token/token.hh"
#include <cstdio>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>

#define RED_ANSII "\033[38;2;255;0;0m"
#define YELLOW_ANSII "\033[38;2;253;218;3m"
#define CLEAR_ANSII "\033[0m"

#define WHAT const char* what() const noexcept override

#define GEN_ERROR_CLASS(name) class name : public std::exception { \
    public: \
        std::string msg; \
        name(std::string msg) : msg(std::move(msg)) {} \
        const char* what() const noexcept override { \
            return msg.c_str(); \
        } \
    }
#define LINE_DIAGNOSTIC_CLASS(name, ansii, error_type_str)     class name : public Diagnostic { \
    types::TokenSourceLocation sourceLocation; \
    base::LineCtx LineCtx_v; \
    std::string Message; \
    driver::Stage Stage; \
    bool isAtEnd  = false; \
    public: \
    name(types::TokenSourceLocation sourceLocation, base::LineCtx LineCtx_v, std::string Message, driver::Stage Stage, bool isAtEnd) : sourceLocation(sourceLocation), LineCtx_v(LineCtx_v), Message(std::move(Message)), Stage(Stage), isAtEnd(isAtEnd) {} \
    types::TokenSourceLocation getSourceLocation() const override { \
        return sourceLocation; \
    } \
    std::string getMessage() const override { \
        std::ostringstream os; \
        os << sourceLocation.FilePath << ":" << sourceLocation.LineNo << ":" << sourceLocation.Col << ": "  \
        << ansii << error_type_str << ": " << CLEAR_ANSII \
        << Message << " (" << to_string_view(Stage) << ")\n" \
        << '\t'; \
        auto LenghtBef = os.tellp(); \
        os << LineCtx_v; \
        auto LenghtAft = os.tellp(); \
        os << '\t'; \
        for(int i = 0; i < (isAtEnd ? (LenghtAft - LenghtBef - 1) : sourceLocation.Col-1); i++) os << ' '; \
        os << ansii << '^' << CLEAR_ANSII; \
        \
        return os.str(); \
    } \
}


#define LINELESS_DIAGNOSTIC_CLASS(name, ansii, error_type_str)     class name : public Diagnostic { \
    std::string Message; \
    driver::Stage Stage; \
    std::string_view FilePath; \
    public: \
    name(std::string_view FilePath, std::string Message, driver::Stage Stage) :  Message(std::move(Message)), Stage(Stage), FilePath(FilePath) {} \
    types::TokenSourceLocation getSourceLocation() const override { \
        return {}; \
    } \
    std::string getMessage() const override { \
        std::ostringstream os; \
        os << FilePath << ": "  \
        << ansii << error_type_str << ": " << CLEAR_ANSII \
        << Message << " (" << to_string_view(Stage) << ")\n"; \
        return os.str(); \
    } \
}


namespace baka {
    namespace exceptions {
        GEN_ERROR_CLASS(DriverError);


        class Diagnostic {
        public:
            virtual types::TokenSourceLocation getSourceLocation() const = 0;
            virtual std::string getMessage() const = 0;
        };


        LINE_DIAGNOSTIC_CLASS(CompilerLineError, RED_ANSII, "error");
        LINE_DIAGNOSTIC_CLASS(CompilerLineWarning, YELLOW_ANSII, "warning");

        LINELESS_DIAGNOSTIC_CLASS(CompilerGlobalError, RED_ANSII, "error");
        LINELESS_DIAGNOSTIC_CLASS(CompilerGlobalWarning, RED_ANSII, "error");
    }

}
```

## `src/base/line_index.hh`

```cpp
#pragma once
#include <atomic>
#include <cassert>
#include <cstddef>
#include <deque>
#include <ostream>
#include <string_view>

namespace baka {
    struct LineInfo {
        size_t StartIdx;
        size_t EndIdx; // inclusive
    };

    namespace base {
        inline static std::atomic<int> LineIndexIdCt = 0;

        struct LineCtx;

        class LineIndex {
            int LineIndexId;

            std::string_view SourceCode;
            std::deque<LineInfo> LineInfos; // need cuz dont wanna invalidate

            bool lineRunning;
            size_t lineStartIdx;
            size_t lineIdx;

        public:
            LineIndex(std::string_view SourceCode);

            void startLine(size_t startIdx);
            void endLine(size_t endIdx);

            size_t CurrentLineIdx() const;
            LineCtx CurrentLine();

            std::string_view GetLine(size_t lineIdx) const;
            std::string_view GetSourceCode() const;

        private:
            std::string_view GetLine(const LineCtx& lCtx) const;
            friend LineCtx;
        };


        struct LineCtx {
            size_t LineIdx;
            LineIndex* LineIndexPtr; // make sure it came from same line index

            std::string_view Display() const;
        };

        inline std::ostream& operator<<(std::ostream& os, const LineCtx& lCtx) {
            os << lCtx.Display();
            return os;
        }
    }
}
```

## `src/base/mmap_file.hh`

```cpp
#pragma once
#include <cstdio>
#include <filesystem>
#include <syscall.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace baka {
    namespace base {
        class MappedFile {
            size_t Length;
            int fd;
            char* Data;

        public:
            MappedFile(std::filesystem::path path);
            std::string_view View() const;
            ~MappedFile();

            // i dont care about these:
            MappedFile(const MappedFile& other) = delete;
            MappedFile(const MappedFile&& other) = delete;
            MappedFile& operator=(const MappedFile& other) = delete;
            MappedFile& operator=(const MappedFile&& other) = delete;
        };
    }
}
```

## `src/base/memory/arena.hh`

```cpp
#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <vector>

namespace baka {

namespace detail {
    template<size_t SZ>
    struct alignas(std::max_align_t) ArenaChunk {
        std::array<std::byte, SZ> data;
    };

    // need for calling dtors
    // type erasure container
    class TEActivationRecord {
        public:
            virtual void cleanup() = 0;
    };

    // linked list of activation records
    struct ActivationRecordNode {
        ActivationRecordNode* Next;
        TEActivationRecord* ActivationRecord;

        static ActivationRecordNode dummy() {
            return ActivationRecordNode{
                .Next = nullptr,
                .ActivationRecord = nullptr,
            };
        }
    };

    template<class T>
    class TypedActivationRecord_ : public TEActivationRecord {
        T* ptr_;

        public:
        TypedActivationRecord_(T* ptr) : ptr_(ptr) {};

        virtual void cleanup() override {
            std::destroy_at<T>(ptr_);
        }
    };

    template<class T>
    class TypedArrayActivationRecord_ : public TEActivationRecord {
        T* _ptr;
        size_t n;

        public:
        TypedArrayActivationRecord_(T* ptr, size_t n) : _ptr(ptr), n(n) {};

        virtual void cleanup() override {
            for(auto i = 0uz; i < n ; i++) {
                std::destroy_at<T>(_ptr + i);
            }
        }
    };
}

namespace base {

    template<size_t SZ = 1024>
    class Arena {
        std::vector<std::unique_ptr<detail::ArenaChunk<SZ>>> InUseChunks;
        detail::ArenaChunk<SZ>* ActiveChunk;
        size_t UsedBytes;
        std::unique_ptr<detail::ActivationRecordNode> ActivationRecordHead; // dummy head node is raii
        detail::ActivationRecordNode* ActivationRecordTail;


        void NewChunkAlloc() {
            InUseChunks.emplace_back(std::make_unique<detail::ArenaChunk<SZ>>());
            ActiveChunk = InUseChunks.back().get();
            UsedBytes = 0;
        }

        public:

            Arena() : ActivationRecordHead(std::make_unique<detail::ActivationRecordNode>(detail::ActivationRecordNode::dummy())) {
                NewChunkAlloc();
                ActivationRecordTail = ActivationRecordHead.get();
            }

            ~Arena() {
                // call cleanup on all activation records
                detail::ActivationRecordNode* curr = ActivationRecordHead.get();
                while(curr != nullptr) {
                    if(curr->ActivationRecord == nullptr) {
                        curr = curr->Next;
                        continue;
                    }

                    curr->ActivationRecord->cleanup();

                    curr = curr->Next;
                }
            }

            // dont need this stuff
            Arena(const Arena& other) = delete;
            Arena(Arena&& other) = delete;
            Arena& operator=(const Arena& other) = delete;
            Arena& operator=(Arena&& other) = delete;


            template<class T, class ...Args>
            requires (sizeof(T) <= SZ)
            std::span<T> AllocArray(size_t N ,Args&& ...args) {
                if(N * sizeof(T) > SZ) {
                    throw std::bad_alloc();
                }

                detail::ActivationRecordNode* NewActivationRecordLLNode = nullptr;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    // if not trivially destructible then push activation record

                    NewActivationRecordLLNode = Alloc<detail::ActivationRecordNode>(nullptr, nullptr);
                    // assingn the activation record later

                    ActivationRecordTail->Next = NewActivationRecordLLNode;
                    ActivationRecordTail = NewActivationRecordLLNode;
                }

                int ct = 1;
                do {
                    size_t NearestAlignIdx = UsedBytes + (alignof(T) - (UsedBytes % alignof(T))) % alignof(T); // ciel div
                    size_t PossibleNewIdx = NearestAlignIdx + (sizeof(T) * N);

                    if(PossibleNewIdx <= SZ) {
                        std::byte* base = &ActiveChunk->data[0];
                        T* startPtr = reinterpret_cast<T*>(base + NearestAlignIdx);

                        for(auto i = 0; i < N; i++) {
                            std::construct_at<T>(startPtr + i, std::forward<Args>(args)...);
                        }

                        UsedBytes = PossibleNewIdx;

                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            detail::TEActivationRecord* NewTEActivationRecord = Alloc<detail::TypedArrayActivationRecord_<T>>(startPtr, N);
                            assert(NewActivationRecordLLNode != nullptr);

                            NewActivationRecordLLNode->ActivationRecord = NewTEActivationRecord;
                        }


                        std::span<T> result(startPtr, N);
                        return result;
                    } else {
                        NewChunkAlloc();
                    }
                } while(ct--);

                throw std::bad_alloc();
            }

            template<class T, class ...Args>
            requires (sizeof(T) <= SZ)
            T* Alloc(Args&&... args) {

                // if constexpr alloc activation record
                detail::ActivationRecordNode* NewActivationRecordLLNode = nullptr;
                if constexpr (!std::is_trivially_destructible_v<T>) {
                    // if not trivially destructible then push activation record

                    NewActivationRecordLLNode = Alloc<detail::ActivationRecordNode>(nullptr, nullptr);
                    // assingn the activation record later

                    ActivationRecordTail->Next = NewActivationRecordLLNode;
                    ActivationRecordTail = NewActivationRecordLLNode;
                }

                int ct = 1;
                do {
                    size_t NearestAlignIdx = UsedBytes + (alignof(T) - (UsedBytes % alignof(T))) % alignof(T); // ciel div
                    size_t PossibleNewIdx = NearestAlignIdx + sizeof(T);

                    if(PossibleNewIdx <= SZ) {
                        std::byte* base = &ActiveChunk->data[0];
                        T* retPtr = reinterpret_cast<T*>(base + NearestAlignIdx);

                        std::construct_at<T>(retPtr, std::forward<Args>(args)...);
                        UsedBytes = PossibleNewIdx;


                        if constexpr (!std::is_trivially_destructible_v<T>) {
                            detail::TEActivationRecord* NewTEActivationRecord = Alloc<detail::TypedActivationRecord_<T>>(retPtr);
                            assert(NewActivationRecordLLNode != nullptr);

                            NewActivationRecordLLNode->ActivationRecord = NewTEActivationRecord;
                        }

                        return retPtr;
                    } else {
                        NewChunkAlloc();
                    }
                } while(ct--);

                throw std::bad_alloc();
            }
    };
}
}
```

## `src/CMakeLists.txt`

```cpp
cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME baka)
set(PROJECT_VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)

project(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    LANGUAGES CXX
)

set(PROJECT_SOURCES
    main.cc
)

# for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# compiler intrinsics
set(CMAKE_CXX_EXTENSIONS ON)

# main bin
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

set(COMPILE_OPTIONS
    -Wall
    -Wextra
    -Wpedantic
    -O2
)

# compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    ${COMPILE_OPTIONS}
)

include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(base)
add_subdirectory(driver)
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(semantic)


target_link_libraries(${PROJECT_NAME} PRIVATE
    driver
    base
    lexer
    parser
    semantic
)

# Each semantic pass has a structural positive/negative test. The same target
# uses the real lexer/parser and semantic library, so phase contracts are tested
# independently of the command-line output format.
include(CTest)
if(BUILD_TESTING)
    add_executable(semantic_pass_tests ../custom_tests/semantic/pass_tests.cc)
    target_link_libraries(semantic_pass_tests PRIVATE semantic lexer driver)
    add_executable(semantic_conversion_tests ../custom_tests/semantic/conversion_tests.cc)
    target_link_libraries(semantic_conversion_tests PRIVATE semantic lexer driver)
    add_executable(semantic_member_call_tests ../custom_tests/semantic/member_call_tests.cc)
    target_link_libraries(semantic_member_call_tests PRIVATE semantic lexer driver)
    add_test(NAME semantic_member_calls COMMAND semantic_member_call_tests)
    foreach(scenario numeric contexts calls compound pointers metadata)
        add_test(NAME semantic_conversions_${scenario} COMMAND semantic_conversion_tests ${scenario})
    endforeach()
    foreach(phase registration resolution annotation checks)
        add_test(NAME semantic_${phase} COMMAND semantic_pass_tests ${phase})
        add_test(NAME semantic_${phase}_error COMMAND semantic_pass_tests ${phase}_error)
    endforeach()
    add_test(NAME semantic_resolution_loop_targets COMMAND semantic_pass_tests resolution_loop_targets)
    add_test(NAME semantic_resolution_this COMMAND semantic_pass_tests resolution_this)
    add_test(NAME semantic_annotation_scoped_type COMMAND semantic_pass_tests annotation_scoped_type)
endif()
```

## `src/semantic/CMakeLists.txt`

```cpp
cmake_minimum_required(VERSION 3.10)

add_library(semantic
    types.cc
    symbols.cc
    declarations.cc
    ast_declarations.cc
    aggregates.cc
    expressions.cc
    conversions.cc
    semantic_analyzer.cc
    registration.cc
    resolution.cc
    annotation.cc
    checks.cc
)

target_link_libraries(semantic PUBLIC
    base
    parser
)

target_include_directories(semantic PUBLIC
    ${CMAKE_SOURCE_DIR}
)
```

# Appendix G — Test-harness assembly definitions (not compiler IR), verbatim

## `test_framework/parser/asm.py`

```python
"""Representation of assembly programs"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import List, Optional, Union


# Operands ########################
class Immediate(int):
    """Immediate operands like $3"""

    def __str__(self) -> str:
        return f"${super().__str__()}"


class Register(Enum):
    AX = auto()
    BX = auto()
    CX = auto()
    DX = auto()
    DI = auto()
    SI = auto()
    R8 = auto()
    R9 = auto()
    R10 = auto()
    R11 = auto()
    R12 = auto()
    R13 = auto()
    R14 = auto()
    R15 = auto()
    BP = auto()
    SP = auto()
    IP = auto()
    XMM0 = auto()
    XMM1 = auto()
    XMM2 = auto()
    XMM3 = auto()
    XMM4 = auto()
    XMM5 = auto()
    XMM6 = auto()
    XMM7 = auto()
    XMM8 = auto()
    XMM9 = auto()
    XMM10 = auto()
    XMM11 = auto()
    XMM12 = auto()
    XMM13 = auto()
    XMM14 = auto()
    XMM15 = auto()

    def __str__(self) -> str:
        return f"%{self.name}"


class Operator(Enum):
    """+,-, and @ operators, which can appear in memory displacement expressions like:
    -4(%rbp)
    foo+10(%rbp)
    bar@GOTPCREL(%rip)
    """

    PLUS = auto()
    MINUS = auto()
    AT = auto()

    def __str__(self) -> str:
        if self == Operator.PLUS:
            return "+"
        if self == Operator.MINUS:
            return "-"
        if self == Operator.AT:
            return "@"
        raise NotImplementedError("what operator is this???")


# Expression representing an offset from some register
# all we do with these is compare them so we don't need a more structured representation here
# use List here for backwards compatibility with Python 3.8
Expr = List[Union[int, str, Operator]]


@dataclass
class Memory:
    """Memory operands (including RIP-relative, stack, indexed)"""

    disp: Optional[Expr] = None
    base: Optional[Register] = None
    idx: Optional[Register] = None
    scale: int = 1  # defaults to 1 if not specified

    def __str__(self) -> str:
        disp_str = "".join(map(str, self.disp or []))
        if (not self.idx) and self.scale == 1:
            return f"{disp_str}({self.base or ''})"
        return f"{disp_str}({self.base or ''}, {self.idx or ''}, {self.scale or ''})"


# type alias for target of jump or call instruction
Target = str

# an operand is a memory address, register, immediate value, or target
Operand = Union[Memory, Register, Immediate, Target]

# Instructions ########################


class Opcode(Enum):
    """All instructions we recognize

    Simplified, e.g. we don't distinguish between different conditional jumps
    consider further simplification, like using the same opcode for all unary computations
    (not, shr, neg, etc)"""

    # data movement/memory manipulation
    MOV = auto()
    PUSH = auto()
    POP = auto()
    LEA = auto()
    # conversions
    MOVS = auto()
    MOVZ = auto()
    CVTTSD2SI = auto()
    CVTSI2SD = auto()
    # binary
    ADD = auto()
    SUB = auto()
    IDIV = auto()
    DIV = auto()
    IMUL = auto()
    AND = auto()
    OR = auto()
    XOR = auto()
    # unary
    SHR = auto()
    NOT = auto()
    NEG = auto()
    CDQ = auto()
    CDQE = auto()  # not used in book, included here to distinguish from cdq
    # control flow and conditionals
    JMP = auto()
    JMPCC = auto()
    SETCC = auto()
    CMP = auto()
    CMOV = auto()
    TEST = auto()
    CALL = auto()
    RET = auto()
    LEAVE = auto()
    UNKNOWN = auto()

    def __str__(self) -> str:
        return self.name.lower()


@dataclass
class Instruction:
    """An assembly instruction consists of an opcode and a list of operands"""

    opcode: Opcode
    operands: list[Operand]

    def __str__(self) -> str:
        str_operands = ", ".join(map(str, self.operands))
        return f"\t{self.opcode} {str_operands}"


class Label(str):
    """A label within an assembly function"""

    def __str__(self) -> str:
        return super().__str__() + ":"


AsmItem = Union[Label, Instruction]


@dataclass
class AssemblyFunction:
    """An assembly function consists of a name and a list of instructions and internal labels"""

    name: Label
    instructions: list[AsmItem]

    def __str__(self) -> str:
        return f"{self.name}\n" + "\n".join(map(str, self.instructions))
```

# Appendix H — Documentation/interface/design quotes, verbatim

## `README.md`

Current repository document; inspect the verified-source caveats in sections 6–7.

````markdown
# baka
baka is a compiler for a C++ like language.

## [**Try it on Compiler Explorer**](https://baka.onetincan.foo/z/vcTGT3)

### Usage

You can build the compiler by running the following command:
```bash
make build
```
baka does not have a preprocessor and thus does not support preprocessor directives.

Running `./build/baka path/to/file.cpp` performs semantic analysis by default and prints the annotated AST on success. Use `--lex` or `--parse` to stop earlier.

#### Make Recipes
- `make parse file=<source_code_file_path>` : Runs the compiler in verbose mode upto the parser stage.
    For instance, to run the parser on `test.c` and stop after the parser stage, you can use:
    ```bash
    make parse file=test.c
    ```

- `make lex file=<source_code_file_path>` : Runs the compiler in verbose mode upto the lexer stage.
- `make semantic file=<source_code_file_path>` : Runs through semantic analysis for the supported language subset.
- `make test-semantic` : Runs structural tests for every semantic pass and semantic acceptance/rejection programs.
- `make parse-folder dir=<directory>` : Runs the parser on all files (excluding `.out` files) in the given directory recursively. Each file's output is written to an `.out` file with the same name and location (e.g. `test/foo.cpp` -> `test/foo.cpp.out`). Reports a summary of files parsed and errors at the end.
- `make test` : Run tests in the `/test` directory and compare the verbose output to the `.expected` output files.
- `make build` : Build the binary and put it in `/build/baka`
- `make clean` : Clean up the build artifacts

#### Command Line Flags:
- `--lex` - run only the lexer and print/output tokens
- `--parse` - run up through parsing and output the AST
- `--semantic` - run declaration registration, resolution/renaming, type annotation, and read-only semantic checks
- `--build_type` - set the build type (e.g. `debug` / `release`)
- `--verbose` - print detailed logs during compilation


To use the compiler, run it with the desired flags and provide the source code file as an argument.
```bash
./baka <options> <source_code_file>
```


#### Lexing

 To run the lexer you can use the `--lex` flag to stop the compiler after the lexing phase.
 To see more verbose logs about the various tokens identified, you can use the `--verbose` flag.

 ```bash
 ./baka --verbose --lex --build_type release <source_code_file>
 ```


#### Parsing

 To run the parser you can use the `--parse` flag to stop the compiler after the parsing phase.
 To see more verbose logs about the AST created, you can use the `--verbose` flag.

 ```bash
 ./baka --verbose --parse --build_type release <source_code_file>
 ```


### Featureset
We are hoping we can support the following featureset:

**Basic Features**
- All arithmetic and logical operators
- if-else
- for loop
- while loop
- do-while loop
- switch cases
- arrays (integer and char)
- pointers
- structures
- printf and scanf
- function calls with arguments
- goto, break and continue
- static keyword

**Advanced Features**
- Recursive function calls
- Classes and objects
- Inheritance
- Function calls with variable arguments
- Function pointers
- Command line input
- typedef
- enum, union
- File manipulation
- Multi-level pointers
- Multi-dimensional arrays

**Extra Non C++ Features**
- Loop Labels:
    Allows programmers to break or continue out until a specific loop. This feature lets you annotate loops with labels and write cleaner and more readable nested loop control flow.

#### A scenario where this makes code cleaner:

##### Without Loop Annotation Labels:
```
bool found = false;
while(stream.hasItems()){
    int val = stream.getNext();
    for(int i = 0; i < len; i++){
        if(val == candidates[i]){
            found = true;
            break;
        }
    }
    if(found){
        // extra break needed to break out of outer loop
        break;
    }
}
if(found){
    // An item in the stream is in the array `candidates`
}
```


##### With Loop Annotation Labels:
```
bool found = false;
while(stream.hasItems(); LoopLabel1){
    int val = stream.getNext();
    for(int i = 0; i < len; i++){
        if(val == candidates[i]){
            found = true;
            break LoopLabel1;
        }
    }
}
if(found){
    // An item in the stream is in the array `candidates`
}
```

### Assumptions
There are certain features we are not supporting:
- Unnamed Parameters in Function Signature
- Preprocessor directives
- `signed`
- `const` pointer
- Multiple inheritance
- Access Modifiers: Everything is `public` by default.
- `namespace` and scope resolution(`::`)
- `enum class`

### Stages Status
- [x] Lexer
- [x] Parser
- [x] Semantic Analysis for the supported subset ([passes and policies](src/semantic/README.md), [remaining TODOs](src/semantic/TODO.md))
- [ ] Optimization Passes
- [ ] Codegen


#### Made By:
- Satindra Ramalingam (24114087)
- Shaurya Goel (24114090)
- Tanmay Arya (24114099)
- Abhinav Shresth (24115008)
````

## `context_A.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

````markdown
# context_A.md — Teammate A handoff (types, symbols, declarations, aggregates)

Baseline commit: `b6f9a26` (`format testcases`). All work below is **uncommitted** on branch `saturn/slop` (applied from a master stash). The compiler driver does **not** call the semantic layer yet: `--parse` is still parser-only. Do not treat parser acceptance as semantic completion.

## 1. Summary

Teammate A implemented the semantic foundation for the C++-like subset compiler: a header-only C++23 service package under `src/semantic/` (type model with LP64 layout, hashed type interning, symbols/scopes, declaration checking, aggregate metadata) plus adapters from the existing AST to those services. It also repaired A-owned parser/AST code so the semantic layer receives input the parser previously discarded (enum initializers, omitted array bounds, union names, all ctors/dtors, member order). `src/base/symbol_table/` was switched from `__gnu_pbds::gp_hash_table` to node-based `std::unordered_map` so entry pointers stay stable across rehashes. Everything is verified: two service/AST suites and 15 parser regression checks pass, and the historical audits reproduce exactly (95/18 original, 94/14 extended, 0 crashes).

## 2. Changes by file

### New: `src/semantic/` (header-only; B/C include, no build-system changes)
- `types.hh` — central type model:
  - ID wrappers `TypeId`/`SymbolId`/`ScopeId` (`uint32_t`, defaulted `==`; value 0 is always error/null).
  - `TypeKind`, `Builtin`, `Completion{Forward,Defining,Complete}`, `BoundKind{Unspecified,Constant}`, `Bound`, `Member` (name/type/symbol/isStatic/isType/isConstructor/isDestructor/isConstMethod/source), `Type`, `Layout`, `QueryState`, `LayoutResult`.
  - Class `Types`: LP64 layout (`ComputeLayout`, `layout`), sizes/ranks/signedness (`arithmetic`, `sizeType`), type constructors (`pointer`, `array`, `function`, `constant` (const-qualify), `nominal`), aggregate editing (`beginDefinition`, `setBase`, `addMember`, `bindMemberSymbol`, `finishDefinition`), queries (`validDerived`, `lookupMember`, `derivesFrom`, `recursivelyConst`, `subobjects`, `instanceMembers`).
  - Hash-based interning (see §3): `MixHash`, `HashVector`, `CanonicalHash`, `CanonicalEqual`, `InternHash`, `InternEqual`, `InternSet InternTable`.
  - `Types(const Types&) = delete` — one instance lives through checking/lowering.
- `symbols.hh` — `Diagnostic{code,message,source,previous}` (diagnostic codes have **no** prefix; e.g. `"undeclared"`, `"redeclaration"`, `"bound"`), `Symbol`, `Scope`, `Declaration`, `SymbolKind{Object,Function,Alias,Type,Enumerator,Parameter,BuiltinFunction}`, `ScopeKind{Global,Block,Function,Prototype,Aggregate,For}`, `Linkage`. Class `Symbols`: scope stack (`enter`/`leave`), `lookup`/`lookupCurrent`/`bind`, `declare` (redeclaration/linkage/overload rules), `completeObject` (inferred array bounds), `setConstant`, `objectsLeaving` (cleanup order), `installTypes`, `installLibrary` (printf/scanf only), `diagnose`/`diagnostics`.
- `declarations.hh` — class `Declarations`: `ConstantState{Value,Nonconstant,Unavailable,Error}`, `IntegerResult`, `DeclarationContext{parameter,member,prototype,storageRequired}`, `SpecialMembers`; API: `specifier`, `boundArray`, `validate`, `object`, `functionDefinition`, `alias`, `aggregate`, `beginAggregate`, `member`, `base`, `finishAggregate`, `enumerator`, `nextEnumerator`, `specialMembers`, `memberAccess`, `thisType`.
- `ast_declarations.hh` — `EvaluationContext{purpose,scope,expected}`, abstract `DeclarationEvaluator` (B implements `IntegerResult integer(const types::ExpressionNode*, EvaluationContext)`), class `ASTDeclarations`: `specifier`, `typeName`, `declarator` (pointer/array/function suffix order), `parameters`, `declarationList` (with `InitializerCheck` callback), `aliases`, `enumeration`.
- `aggregates.hh` — `MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>` (B supplies); class `ASTAggregates`: `forward`, `definition(...)` overloads for struct/class/union AST nodes.
- `.clang-format` — scopes formatting rules to this module only.

### Modified: `src/base/symbol_table/symbol_table.hh`
- `gp_hash_table` → `std::unordered_map` (node storage ⇒ stable entry addresses; `AddEntry` no longer asserts on duplicates, returns `false`).
- Added `LookupCurrent` (current-scope-only); `ExitScope` guards empty stack; `AddEntry`/`RegisterOrReplaceType` auto-open a scope when the table is empty.

### Modified: A-owned parser files
- `src/parser/class.cc`, `src/parser/struct.cc` — name checks now use `TypeLookup.LookupCurrent(...)` (current-scope) instead of outward `LookupType`; `SetIsStructDefined` only when actually a struct.
- `src/parser/enums.cc` — removed a stray file-scope `enum {BALLS,EAT}`; named enums recheck via `LookupCurrent` and only `AddType` when new; `K_INT`/`OP_ASSIGN` now parses an optional enumerator initializer expression into the AST.
- `src/parser/union.cc` — union names registered via current-scope check, `union U;` forward declarations return `UnionNode(nullptr, name)`, named node created with name.
- `src/parser/structBody.cc` — null `ParentName` guarded (anonymous aggregates no longer crash on ctor detection); anonymous destructor diagnosed and redirected with placeholder parent name.
- `src/parser/declaration.cc` — `[]` with no size appends a null `ArraySize` (bound `Unspecified`), in both declarator paths; removed the "identifier cannot be a type name" error so a declaration may shadow an outer type name.
- `src/parser/type.cc` — central invalid-`unsigned` guard in `ParseTypeIdentifier`: diagnoses "unsigned requires char, short, int, or long" and returns an error-marked `int` identifier instead of tripping legacy AST asserts (covers B's function/parameter parsers too).
- `src/parser/typedef.cc` — `RegisterOrReplaceType` only when the name is not already declared in the current scope (preserves typedef type metadata; compatible alias repeats stay valid).

### Modified: AST headers (A owned per TASK_SPLIT.md)
- `declaration.hh` — `DeclarationList`/`FunctionParameter`/`DeclarationIdentifierNode`/`SingleDeclarationNode` gained const accessors (`getIsStatic`, `getIsConst`, `getModifier`, `getTypeName`, `getDeclarations`, `getParameters`, `getDeclarator`, `getIdentifier`, `getArraySizes`, `getFunctionParameters`, `getInnerDeclaration`, `getVariableName`(null-safe), `getInitializer`); legacy ctor asserts for invalid unsigned removed (parser now diagnoses); `FunctionParameterList` gained `getParameters`, `isVariadic`, constructor/a setter `setVariadic` (**B must set this when it consumes `...`** — currently nothing sets it).
- `enums.hh` — `EnumNode` stores parallel `std::vector<ExpressionNode*> Initializers`; `addEnumValue(value, initializer = nullptr)`; `getInitializers()`.
- `structBody.hh` — `Constructor`/`Destructor` are now **vectors** (`Constructors`, `Destructors`) and everything appends to a source-order `std::vector<ASTNode*> Members`; `getMembers()`; wide accessor set (`getDeclarations`, `getFunctions`, `getTypedefs`, `getConstructors`, `getDestructors`).
- `struct.hh`, `class.hh` — `getName`, `getBaseName`, `getBody` (+ `getName` on declaration nodes).
- `typedef.hh` — ctor init order fixed (isConst initialized first), accessors `getIsConst`, `getModifier`, `getTypeName`, `getVariables`.
- `typenode.hh` — ctor assert removed; accessors `getIsConst`, `getModifier`, `getTypeName`.
- `union.hh` — `UnionNode(body, name = nullptr)` optional name; `getName`, `getBody` (null-safe Print).

### New: `semantic_audit/teammate_a/`
- `POLICY.md` — the binding language/type contract (LP64, canonical vs nominal types, declaration order rules, array bound policy, aggregate/layout rules, enums, printf/scanf).
- `run.py` — compiles `services_test.cc` and `ast_test.cc` (`c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -Isrc -Isrc/base`), runs them, then 15 parser regressions against `build/baka --parse --verbose`; excludes `valid_aggregate_shadow.cpp` as a known C gap.
- `services_test.cc`, `ast_test.cc` — service checks incl. interning stress (24-param vector hash, rotated/mutated params, slot reuse after duplicate pop, 512-element params, name-equivalence of structurally identical anonymous structs), parser symbol-table stability over 10 000 inserts.
- `programs/` — 6 targeted parser fixtures (`valid_aggregate_shadow.cpp` is the known-failing C-integration case).
- `verification.json` — compact audit results (parser counts, integration gaps).

### Coverage map: the 24 A packages (A01–A24) and who integrates each

| Package | Implemented deliverable | Integration responsibility |
| --- | --- | --- |
| A01 | LP64 widths/ranks/signedness/alignment, sizeof type, explicit supported subset | B/C consume POLICY.md |
| A02 | Canonical builtin/pointer/array/function/const-qualified/error types, nominal enum/aggregate IDs | shared `Types` instance |
| A03 | Stable SymbolId/ScopeId arenas, current/outward lookup, lexical hiding; parser storage node-based | C owns scope traversal |
| A04 | Per-declarator bound → binding → initializer order; no later-local predeclaration | B evaluator/initializer callbacks, C traversal |
| A05 | Prototype/parameter registration, duplicates, shared parameter/outer-body scope contract | C must reuse the Function scope for the outer body |
| A06 | Kinds, definitions, compatible repeats, overloads, linkage/storage conflicts | C classifies declaration versus body |
| A07 | Alias identity/qualification/completion and incompatible replacement errors; parser preserves existing type metadata | B/C use the alias service |
| A08 | Semantic specifier validation; invalid unsigned inputs diagnose without aborting | B still owns duplicate static/const checks in `function.cc` |
| A09 | Nested declarator binding, illegal derived types, object-with-body rejection | C calls `functionDefinition` for bodies |
| A10 | Prototype vs definition storage-completeness; arrays below pointers checked | B completes inferred array bounds |
| A11 | Parameter adjustment, const preservation on parameter objects, normalized overload signatures | B preserves variadic syntax and selects calls |
| A12 | Constant/unspecified array bounds only; nonconstant bounds rejected before type construction | B must classify bound constants correctly |
| A13 | Forward/Defining/Complete states, anonymous identity, recursive containment rejection, union forwards | C dispatches aggregate AST declarations |
| A14 | Ordered members/aliases/functions/special members, duplicate signatures/names, member symbols | B supplies function signatures via `MemberSignature` |
| A15 | Single complete nonunion base, cycle/duplicate checks, inherited lookup/hiding | B conversion and call rules use base queries |
| A16 | Checked layout, alignment/padding, union/base/field offsets, overflow/incomplete results | B `sizeof` and C lowering consume queries |
| A17 | Retained enum initializer AST, declaration-time constant callback, constants/conflicts/overflow | B implements `DeclarationEvaluator` |
| A18 | Every ctor/dtor retained; signature validation, generated special-member metadata, subobject lifetime order | B signature callback; C/B body and cleanup integration |
| A19 | Union names/forwards, static storage exclusion, nontrivial union restrictions | B/C manage the chosen active union alternative |
| A20 | Invalid unsigned and aggregate-state crash repairs; `[]` retention; safe malformed-declarator access | existing excluded spellings remain excluded |
| A21 | Flexible arrays/bit-fields/designators explicitly disabled; incomplete member storage rejected | B rejects unsupported initializer forms |
| A22 | Builtin type names and protected printf/scanf signatures/identities | additional file APIs need explicit agreed signatures |
| A23 | Dot/arrow/member/this/static/const receiver services, inherited overload candidates | B checks receiver value category and call viability |
| A24 | Service and AST suites, owned parser crash/valid controls, full audit comparisons | compiler-level semantic fixtures await C integration |

### Pre-existing, untouched (do **not** attribute to A)
- `source_code` scratch input edit, `.codex/config.toml`, the rest of `semantic_audit/` (CHECKLIST, TASK_SPLIT, TASK_OWNERSHIP, run.py, RESULTS, extended/ corpus).

## 3. Implementation details

### Type interning (structural equivalence) in `types.hh`
- Canonical types (pointer/array/function/const) are interned structurally:
  - `MixHash(size_t& Hash, size_t Value)` — 64-bit golden-ratio combiner (`Hash ^= v + HashSeed + (Hash<<6) + (Hash>>2)`).
  - `HashVector(Seed, vector<TypeId>)` — mixes length then every element *in order* ⇒ order- and length-sensitive.
  - `CanonicalHash` covers exactly `kind, builtin, element, bound{kind,count}, parameters, variadic`; `CanonicalEqual` checks the same field set — parity is a hard invariant.
  - `InternTable` is `std::unordered_set<uint32_t, InternHash, InternEqual>` over **arena indices**; `InternHash`/`InternEqual` capture `this`, hence `Types` is non-copyable/non-movable.
  - `InternType`: `AddType` pushes the candidate onto the `deque` arena, then the set insert either finds a duplicate (⇒ `TypeArena.pop_back()`; IDs stay contiguous) or keeps it.
- Nominal types (`nominal()` — struct/class/union/enum) are never interned (always `AddType`): structurally identical aggregates keep distinct IDs (name equivalence). Aliases point at the underlying ID.
- Const over arrays qualifies the element (`constant()` recursion); completion state is deliberately **not** hashed (an aggregate completing doesn't change its ID).
- Stress-verified: 4 000 randomized constructions replay exact IDs.

### Data flow (declaration-time)
```
AST (struct/class/union/enum/typedef/declaration nodes)
        │
ASTAggregates / ASTDeclarations          ← adapters, C calls per source order
        │ uses                     │ calls B via
Declarations checking        DeclarationEvaluator::integer() (array bounds, enumerator values)
        │                            │ results recorded in
Types arena + Symbols scopes/symbols/diagnostics
        │
B consumes: layout queries, member metadata, symbols; C consumes: symbols, scopes, objectsLeaving, diagnostics
```
- Source order is enforced: each declarator's bounds → insert symbol → initializer callback → next declarator. `int x = x;` sees the new x; `int x[sizeof(x)]` does not.
- Unspecified array bounds are inserted first; the initializer (B) infers them via `Symbols::completeObject`, after which A re-validates storage.
- Parameters adjust to pointers (`parameterObject`/`adjustParameter`); prototypes checked before definitions; function definitions require complete storage.

### Config/env
- None. New code is headers only; tests compile with the system `c++` (`-std=c++23 -Wall -Wextra -Wpedantic -Werror`).

## 4. Design decisions

Deliberate (can be changed, but coordinate):
- **LP64 fixed target**, layout = size/alignment/padding/offsets via `ComputeLayout`; constexpr-free runtime model. Rejected: consulting host ABI (POLICY.md A01).
- **Constant-only array bounds** (`BoundKind{Unspecified,Constant}`); runtime arrays/VLAs are not representable; nonconstant bounds are rejected by `boundArray` *before* a type is created. Rejected: runtime layout state.
- **Interned canonical vs nominal types**: structural identity for derived types, name identity for aggregates. Alternative hashing-everything-as-identity rejected (collisions conflate types; a hash cannot store layout/members). The prior O(n) scan intern was replaced by the hash table.
- **Node-based parse symbol table** replacing `pb_ds` hash; `AddEntry` returns bool instead of assert. Rejected: keeping rvalue/unstable pointers.
- **Current-scope name checks** in struct/class/enum/union registration: inner scopes may shadow outer types; `struct S variable;` at file scope still blocked by C-owned `ParseProgram` dispatch (see §6).
- **`unsigned` only on direct char/short/int/long spellings**; invalid spellings diagnose in `type.cc` (single guard covers B-owned parsers). No `signed`, no `long long`, no `(void)` param spelling, no references, no const-pointer spelling.
- **Diagnostics as data** in `Symbols::diagnostics()` with bare owner-free codes; services never fabricate valid results (error = ID 0 / `{}`).

Temporary shortcuts / known simplifications:
- `Types` non-copyable instead of a pointer-stabilizing design (fine: one instance per pipeline).
- `installLibrary` covers exactly printf/scanf; FILE/file APIs deliberately not guessed.
- Flexible arrays/bit-fields/designated init explicitly rejected by `Declarations::member` even though the extended parser now *accepts* the flexible-array syntax.
- Unverified: ast_test.cc coverage beyond what run.py executes (run.py passes both suites).

## 5. Interfaces and contracts (exact, stable)

### `src/semantic/types.hh` — class `Types` (one instance per pipeline, non-copyable)
```cpp
const Type& get(TypeId) const;      TypeId builtin(Builtin) const;
std::optional<ArithmeticInfo> arithmetic(TypeId) const;  // {bits,rank,isSigned,floating}
TypeId sizeType() const;            // ULong
TypeId unConst(TypeId) const;       // strips top-level const; identity otherwise
bool   isConst(TypeId) const;       bool isAggregate(TypeId) const;
bool   isIntegral(TypeId) const;    bool isVoid(TypeId) const;
TypeId constant(TypeId, bool = true);  // const-qualify (arrays qualify elements)
TypeId pointer(TypeId);                TypeId array(TypeId, Bound = {});
TypeId parameterObject(TypeId);        TypeId adjustParameter(TypeId);
TypeId function(TypeId result, std::vector<TypeId> params = {}, bool variadic = false);
TypeId nominal(TypeKind, std::string name = {});
bool   beginDefinition(TypeId);        bool setBase(TypeId, TypeId);
bool   addMember(TypeId, Member);      bool bindMemberSymbol(TypeId, size_t, SymbolId);
LayoutResult finishDefinition(TypeId); LayoutResult layout(TypeId) const;
bool   validDerived(TypeId) const;
MemberLookup lookupMember(TypeId receiver, const std::string&) const;
bool   derivesFrom(TypeId, TypeId) const;  bool recursivelyConst(TypeId) const;
std::vector<Subobject> subobjects(TypeId, bool destruction = false) const;
std::vector<size_t>    instanceMembers(TypeId, bool destruction = false) const;
```
Invariant: `TypeId{0}` / `SymbolId{0}` / `ScopeId{0}` are invalid; constructors return `{}` on error; equality of the same canonical structure is ID equality (hash interning).

### `src/semantic/symbols.hh` — class `Symbols`
```cpp
ScopeId enter(ScopeKind);              bool leave();
std::vector<SymbolId> lookup(const std::string&) const;       // outward
std::vector<SymbolId> lookupCurrent(const std::string&) const; // scope only
SymbolId bind(const std::string&, const void* source);        // value use; rejects overload sets/aliases
SymbolId declare(Declaration);         // Declaration{name,kind,type,definition,isStatic,linkage,source}
bool     completeObject(SymbolId, TypeId completed);  // [] → N inference; B calls after insertion
bool     setConstant(SymbolId, int64_t);
std::vector<SymbolId> objectsLeaving(ScopeId from, ScopeId destination) const; // cleanup order
TypeId lookupType(const std::string&, source);
void   installTypes();  void installLibrary();   // once, global scope
const std::vector<Diagnostic>& diagnostics() const;  void diagnose(code, message, source, previous);
```

### `src/semantic/declarations.hh` — class `Declarations`
```cpp
IntegerResult{ConstantState state; int64_t value;}
enum class ConstantState { Value, Nonconstant, Unavailable, Error };
TypeId specifier(std::string name, bool constant=false, bool isUnsigned=false,
                 std::optional<TypeKind> expected={}, const void* source);
TypeId boundArray(TypeId element, IntegerResult, const void* expr, bool omitted=false);
bool   validate(TypeId, DeclarationContext, const void* src) = validate(TypeId, {parameter,member,prototype,storageRequired}, src);
SymbolId object(Declaration, DeclarationContext);
SymbolId functionDefinition(Declaration);
SymbolId alias(std::string name, TypeId, const void* src);
TypeId  aggregate(std::string name, TypeKind, const void* src);
bool    beginAggregate(TypeId, src);  bool base(TypeId, TypeId, src);  bool finishAggregate(TypeId, src);
bool    member(TypeId owner, Member);
SymbolId enumerator(std::string, TypeId, IntegerResult, src);
IntegerResult nextEnumerator(std::optional<int64_t>) const;
SpecialMembers specialMembers(TypeId) const;   // {constructors,destructor,implicitDefault/implicitCopy/implicitDestructor,nontrivial}
Access memberAccess(TypeId receiver, std::string name, bool arrow, bool staticContext, src);
TypeId thisType(TypeId currentClass, bool staticContext, bool constMethod, src);
```

### `src/semantic/ast_declarations.hh` and `aggregates.hh`
```cpp
struct EvaluationContext{ enum Purpose{ArrayBound,Enumerator}; Purpose purpose; ScopeId scope; TypeId expected; };
class DeclarationEvaluator { virtual IntegerResult integer(const types::ExpressionNode*, EvaluationContext) = 0; };  // B implements
class ASTDeclarations {
    TypeId specifier(const types::IdentifierNode*, types::TypeSpecifierModifier, bool constant, const void* src);
    TypeId typeName(const types::TypeNode&);
    TypeId declarator(TypeId base, const types::DeclarationIdentifierNode*);       // errors → {}
    std::vector<SymbolId> parameters(const types::FunctionParameterList&);         // into C's current Function scope
    std::vector<SymbolId> declarationList(const types::DeclarationList&, InitializerCheck = {}); // InitializerCheck: std::function<void(SymbolId, const types::ExpressionNode*)>
    std::vector<SymbolId> aliases(const types::TypedefNode&);
    TypeId enumeration(const types::EnumNode&);
};
class ASTAggregates {
    TypeId forward(const types::IdentifierNode*, TypeKind, const void* src);
    TypeId definition(const types::StructDefinitionNode&, MemberSignature = {});   // + Class/Union overloads
    using MemberSignature = std::function<std::optional<Member>(const types::ASTNode*, TypeId)>;
};
```

Naming convention for diagnostics: bare dot-free codes like `"redeclaration"`, `"unsigned"`, `"union-lifetime"`; `source` is the offending AST node pointer, `previous` the earlier conflicting symbol. `Diagnostic::source`/`previous` are opaque handles for C's source-span map.
`.clang-format` file must be preserved by B/C edits under `src/semantic/`.

## 6. Handoff for B and C

### Teammate B — expressions, calls, constants, initialization (24 boxes)
Build on:
- `Types::arithmetic`/`sizeType`/`layout` for numeric semantics and `sizeof`; `Types::constant`/`unConst`/`isConst` for the qualification model (pointee qualification supported; const-pointer spelling excluded by the parser).
- The `ConstantState` contract: implement `DeclarationEvaluator::integer(expr, ctx)` — return `{ConstantState::Value, n}` only for true integral constants in the live environment; `Nonconstant`/`Unavailable`/`Error` otherwise; never a fabricated value. The evaluator style used by (wired into) `boundArray` and enumerator paths.
- Overload metadata: `memberAccess(...).candidates`, `lookupMember(...).indices`; `Type::variadic` (via `TypeKind::Function`) gives you the call-tail variadic marker.
- `Symbols::completeObject(id, TypeId)` to complete `T a[] = ...` inside the `declarationList` initializer callback before final storage validation, and `Symbols::setConstant`/`get(id).constant` for enumerator values.
- `MemberSignature` (required by §5) — you own expression/constant/cast/initializer/sizeof AST headers and `function.cc`, so you produce member signatures with `isStatic`/`isConstMethod`/`isConstructor`/`isDestructor` flags from your AST; no fabricated defaults exist.
- Enumerators' explicit initializer AST (`EnumNode::getInitializers`) is retained for you to evaluate.

Do **not** change:
- Existing `Type` field semantics (`element` for pointee/qualified/return), LP64 tables, enum rank/signedness, source-order semantics, diagnostic code set ("`bound`", "`redeclaration`", ...), and the `.clang-format` under `src/semantic/`.
- Parser unsigned guard in `src/parser/type.cc` (it already covers your `function.cc` inputs).

Policy choices written down, not negotiated live — change deliberately, with A's contract in mind (`POLICY.md`): constant-only array bounds; optional aggregate syntax (flexible arrays, bit-fields, designated initializers) disabled; zero-argument `()` means exactly zero parameters; single nonvirtual inheritance; the existing source exclusions (`long long`, unnamed parameters, `(void)` grammar, references, namespaces) stay unexpanded.

`installLibrary()` gives printf/scanf builtin `SymbolId`s with protected signatures — use those identities for optional format checking. Adding any other library declaration (FILE, file APIs) requires a jointly agreed signature/opaque-type policy first.

Known B tasks (unverified by A whether done):
1. Set `FunctionParameterList::setVariadic` where `function.cc` consumes `...` (flag exists; nothing sets it).
2. `TryParseFunction` still fails to diagnose repeated `static`/`const` specifiers.
3. Supply `MemberSignature` (function/constructor/destructor signatures with static/const/special-member flags — A preserves every special member in source order and substitutes nothing) and expression receivers (`this` via `thisType`), call ranking, promotions.
4. Type inference/initialization: defaulted special-member construction/copy/destruction *usability and deletion*, member initializers, global/static initialization, and const-initialization requirements are all B behavior — C must not bypass A's final storage validation.

### Teammate C — pipeline, statements, driver (24 boxes)
Build on:

**Integration sequence (exact) — implement in this order:**
1. Create one `Types`, one `Symbols`, one `Declarations`; call `installTypes()` once (plus `installLibrary()` only if using the agreed implicit-library policy). Keep them alive through semantic checking and lowering. Stop after lexer/parser errors. `Diagnostic::source`/`previous` are opaque AST/source handles for C's source-span map.
2. B implements `DeclarationEvaluator::integer` (purpose, current `ScopeId`, expected int type; return Error/Unavailable for invalid/unimplemented; never Value for floating or nonconstant expressions). Build `ASTDeclarations` with that evaluator injected.
3. Visit program/block contents in source order: `ASTDeclarations::declarationList` (with B's initializer callback), `aliases`, `enumeration`, and `ASTAggregates::definition` for A-owned nodes. Initializers see their own new symbol; subsequent declarators do not exist yet. B may complete `T a[] = ...` inside the callback via `Symbols::completeObject`.
4. For a function body, resolve its declaration type first, pass it to `Declarations::functionDefinition`, open a Function scope, register parameters through `ASTDeclarations::parameters`, and visit the OUTER compound body in that SAME scope. Further nested blocks and for-statements enter/leave separate scopes.
5. For aggregate definitions, B supplies `MemberSignature` for function/constructor/destructor nodes (static/const/special-member flags). `ASTAggregates::definition` stores ordered metadata and completes layout before C/B check member bodies; a missing callback result becomes an explicit diagnostic; the temporary member-declaration scope is closed after collection. Method-body name binding needs C/B to expose the class context and use member lookup.
6. Resolve expression names with `Symbols::lookup`/`bind`; retain overload sets for B. `bind` deliberately rejects unresolved overload sets as single value symbols; B handles call selection and expression receiver categories.
7. Attach diagnostics to source locations, stop lowering on any error, and combine `Symbols::objectsLeaving`, `Types::subobjects`, and B's initialization records for jump cleanup and lifetime restrictions. This API does not by itself validate goto legality or execute cleanup.

General properties:
- Stop-on-error: parser recovery marks error nodes (`hasError()`) — skip those; stop the pipeline before lowering on any semantic diagnostic.
- `Diagnostic::source`/`previous` are opaque AST/source handles — feed into your source-span map.

Do **not** change:
- `src/semantic/*` public signatures (listed in §5); `symbol_table.hh` node-storage/stability contract (`GetEntry` stable until scope exit; duplicate inserts return false); A-owned parser registration logic (`struct.cc/class.cc/enums.cc/union.cc/typedef.cc` current-scope rules).
- `semantic_audit/teammate_a/run.py` expectations (15 parser cases + the excluded `valid_aggregate_shadow.cpp`); audit snapshot files.

Known C tasks (the largest gap):
1. **Wire the semantic stage into the driver** — `--parse` is parser-only today; add the CLI/phase and stop on semantic diagnostics (C03/C04).
2. **Local aggregate dispatch** — compound-statement parsing routes `struct Item{...};` through declaration parsing and fails; `ParseProgram` treats every leading struct/class/union token as a type declaration, blocking `struct S variable;` at file scope. Fixture: `semantic_audit/teammate_a/programs/valid_aggregate_shadow.cpp` (currently failing; excluded from the 15).
3. Coordinate in C-owned parser files (`compoundStatement.cc`, `parser.cc`); A's aggregate parsers now support shadowing by current-scope rules.

Likely conflicts: A owns these files — if you must touch `structBody.cc`/`declaration.cc`/`enums.cc`/`union.cc`/`type.cc`/`typedef.cc`/AST headers above, send a patch through A's owner instead (per TASK_SPLIT.md §File ownership), except `struct.hh/class.hh/union.hh/enums.hh/declaration.hh` accessors which are shared-header changes B/C may request.

## 7. Status and open items

Done and verified:
- Service package compiles standalone (`-Wall -Wextra -Wpedantic -Werror`); both suites pass.
- 15 parser regressions pass; driver builds; historical audits reproduce (95/18 original, 94/14 extended, 0 crashes/0 timeouts).
- Hash-based interning stress-tested in-session: 4 000 randomized function-type constructions replay exact IDs.

Partial / open:
- Driver does not invoke the semantic layer (C task).
- `FunctionParameterList::setVariadic` is never set (B task).
- Local aggregate parsing (`valid_aggregate_shadow.cpp`) fails pre-service (C task).
- Repeated `static/const` specifiers in `TryParseFunction` underspotted (B task).
- File APIs/FILE opaque handling undefined (needs joint policy decision before anyone claims file semantics).
- `Types` is non-copyable/non-movable (InternTable captures `this`) — deliberate; relax only with a pointer-stabilizing redesign.
- `unverified`: none of the semantic checks run end-to-end through the compiler (no driver integration exists).

## 8. How to run and verify

```bash
# build driver (expected: built target baka, no warnings)
cmake --build build --parallel 2

# service suites + AST suite + 15 parser regressions (expected: two "all checks passed" lines, 17/17 checks)
python3 semantic_audit/teammate_a/run.py

# full audit runners (expect: lex 113 accepts, parse 95 accepts / 18 rejects; extended: 108, 94/14; 0 crashes/timeouts)
python3 semantic_audit/run.py --output /tmp/out-main
python3 semantic_audit/run.py --programs semantic_audit/extended/programs --output /tmp/out-ext

# compile semantic headers against the warnings policy as B/C should
c++ -std=c++23 -Wall -Wextra -Wpedantic -Werror -Isrc -Isrc/base -fsyntax-only src/semantic/aggregates.hh
```

Full token/AST logs land in `/tmp/out-*/`. The known-failure `valid_aggregate_shadow.cpp` is reported by `run.py` as the excluded C gap, not a check.
````

## `context_B.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

````markdown
# context_B.md — Teammate B handoff (expressions, calls, constants, initialization)

Branch: `saturn/slop`. Build: `cmake -S src -B build && cmake --build build --parallel 2`.
B implements the 24 B packages in [TASK_SPLIT.md](semantic_audit/TASK_SPLIT.md) on top of A's
type/symbol/declaration services. Everything lives under `src/semantic/`; the semantic stage is
reachable with `./build/baka --semantic <file>` (see [context_C.md](context_C.md) for wiring).

## 1. Summary

`ExpressionChecker` (`src/semantic/expressions.{hh,cc}`) is a full typed-expression, overload,
constant-evaluation, `sizeof`, and initialization checker. It is a `DeclarationEvaluator`, so A's
`ASTDeclarations` calls it back for enumerator/array-bound constants, and it supplies
`ASTAggregates::MemberSignature` via `memberSignature`. It never rewrites the AST: it returns typed
`ExprResult` records and reports through A's `Symbols::diagnose`. C drives it from
`SemanticAnalyzer` and owns the semantic stage/CLI.

Verified state: `./custom_test_hook parse` **57/57**, `python3 semantic_audit/teammate_a/run.py`
**17/17**, `python3 semantic_audit/run.py` no crashes (lex 113, parse 96/17), and no crashes or
timeouts classifying all 221 `semantic_audit/` fixtures with `--semantic`.

## 2. Result contract (B02)

```cpp
enum class ValueCategory { LValue, RValue };
struct ExprResult {
    TypeId type;                       // A's interned type (0 = error)
    ValueCategory category;           // lvalue/rvalue
    bool error;                       // poisoned; never produces a fabricated valid type
    bool constantExpression;          // valid integer constant-expression (B14)
    IntegerResult constant;           // {ConstantState, int64_t} when folded
    bool nullPointerConstant;         // eligible zero integral constant (B07)
    explicit operator bool() const;   // !error && type.value
};
```

Public surface: `checkValue` (rvalue/decay context), `checkCondition` (scalar conversion),
`integer(...) override` (B14 evaluator contract), `convertible(from,to,value)`, `initialize`,
`memberSignature`, `registerMinimumArity`, `setFunctionContext`.

## 3. B01–B24 coverage

| Package | Where | Notes |
| --- | --- | --- |
| B01 conversion/value policy | `isAssignable`, `promote`, `usualArithmetic`, `conversionRank`, `isNullPointerConstant` | arithmetic/enum/pointer/null conversions, lvalue vs rvalue categories. |
| B02 typed output | `ExprResult`, `makeValue`/`makeConstant`/`makeError` | error propagation; no AST rewrite. |
| B03 literals/strings | `checkConstant`, `StringLiteral` helper | int/char/unsigned/long long/bool/float/double/`nullptr`; `SLString` → `const char[N]`. |
| B04 conversions/unevaluated | `check(expr, shouldDecay)` | explicit decay control; `sizeof`/short-circuit type-check unevaluated operands without folding. |
| B05 arithmetic/logical/bitwise | `checkBinary`, `checkUnary` | integral-only bitwise/shift, short-circuit `&&`/`||` type-check both operands but only evaluate what is reached. |
| B06 pointer arithmetic | `checkBinary` (add/sub/diff), `checkIndex` | complete-object pointee checks, `ptrdiff`, result types. |
| B07 comparisons/null | `checkBinary` (relational/equality) | equality vs ordering separated; ordering rejects function/void pointers; `p==0` accepted, foldable zeros don't become null unless integral constant. |
| B08 address/index | `checkUnary` (`&`,`*`), `checkIndex` | array/function decay suppression, lvalue requirements, element type/category. |
| B09 assignment/modifiability | `checkAssignment`, `isModifiableLValue`, `flattenMembers` | recursive const fields, arrays not assignable, aggregate copies with array members allowed. |
| B10 compound/inc-dec | `checkAssignment` (compound), `checkUnary`/postfix | single left-side evaluation, correct result category. |
| B11 conditional/comma | `checkTernary`, `checkBinary` (`OP_COMMA`), comma-list branch | usual arithmetic/pointer composite; single-element comma wrappers preserved for constness; genuine comma is not a constant expression (C11 ICE rule). |
| B12 casts | `checkCast` | through A's `specifier`/`typeName`; rejects array/function and unsupported aggregate casts. |
| B13 sizeof | `checkSizeof` | type or expression; uses `Types::layout`; suppresses decay; result `ULong`. |
| B14 constant evaluator | `integer`, `check` constants | distinguishes ICE validity from foldability: comma/increment/assignment/function-call in evaluated position invalidate; unevaluated operands still type-check. |
| B15 bounds/VLA | `ASTDeclarations` + `integer` | numeric bounds only; VLAs are not supported (diagnosed through A's bound checks). |
| B16 scalar init | `initializeScalar` | brace shape, narrowing diagnostics, `const` requires initializer (C's rule via analyzer). |
| B17 aggregate/string init | `initializeAggregate`, `initializeArray`, `initializeList` | nested shape/count, member order, brace elision, string terminator/length inference, omitted elements. |
| B18 global/static init | `isConstantInitializer`, `initialize` | analyzer rejects non-constant static storage initializers using B's constant folding. |
| B19 calls/overloads | `checkCall`, `resolveCallee`, `functionCandidates`, `checkArguments`, `conversionRank` | arity/boundary/conversion, overload ranking, constructor candidates. |
| B20 defaults/variadics | `MinimumArity`, `registerMinimumArity`, `memberSignature` | trailing-default min-arity map; variadic accepted (scalar-only args, minimal promotions). |
| B21 object/construction | `checkPrimary` (`ThisNode`), `checkMember`, `checkCall` | receiver typing, `this`, const-member calls, object copy, ctor arguments. |
| B22 allocation | `checkBinary`/`checkUnary` unsupported-node path | `new`/`delete` are diagnosed unsupported (policy). |
| B23 builtin calls | A's `installLibrary` symbols + `checkCall` | ordinary signature checking; printf/scanf literal format checks not implemented (see gaps). |
| B24 regression checks | `semantic_audit/` + teammate A `run.py` | fixtures encode accept/reject by name; see §6. |

## 4. Files changed

- **New** `src/semantic/expressions.hh`, `src/semantic/expressions.cc` — the checker above.
- **B-owned parser fixes** (`src/parser/`):
  - `expressions.cc` — conditional middle operand parses assignment expression plus an optional
    comma list (unparenthesized comma in `a ? 2,3 : 4`); single-expression operands keep their
    original AST shape so existing golden ASTs stay stable.
  - `sizeof.cc` — expression operand parses an assignment expression plus an optional comma; fixes
    the single-expression comma wrapper that made `sizeof(f)` decay to a function pointer and slip
    past the function-type diagnostic.
  - `function.cc` — `...` now sets `Variadic` on `FunctionParameterList`; duplicate `static`/`const`
    specifiers diagnosed.
- **A-owned header request** (landed): `declaration.hh` `FunctionParameterList::setVariadic`,
  `ConstructorNode::getArgs/getBody`, `DestructorNode::getBody`.

## 5. Known gaps (policy-conditional, documented, not silently accepted)

- Variadic argument promotions are minimal (scalar only); no default-argument promotions beyond
  the checker's arithmetic conversions.
- No printf/scanf compile-time format checking (B23 optional part).
- VLA support is out of scope: bounds must be integer constant expressions.
- `new`/`delete` expression syntax/type checks are unsupported by policy (diagnosed).
- Warnings (missing return, unreachable, uninitialized, unused) are C's C21 items, not implemented.

## 6. Reproduce

```bash
cmake --build build --parallel 2
./custom_test_hook parse                     # 57 PASS
python3 semantic_audit/teammate_a/run.py     # {"checks":17,"passed":17}
python3 semantic_audit/run.py                # lex/parse snapshot, no crashes
for f in semantic_audit/programs/*.cpp semantic_audit/extended/programs/*.cpp; do
  ./build/baka --semantic "$f" >/dev/null 2>&1; done   # 0 crashes/timeouts
```

Fixtures whose name starts with `valid_` (or is an explicit positive control) must exit 0;
`wrong_`/`invalid_`/`undefined_`/`duplicate_`/`bad_`/`nonconstant_`/`nested_` fixtures must exit
non-zero with a `SEMANTIC` diagnostic.
````

## `context_C.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

````markdown
# context_C.md — Teammate C handoff (pipeline, statements, control flow)

Branch: `saturn/slop`. Build: `cmake -S src -B build && cmake --build build --parallel 2`.
C implements the 24 C packages in [TASK_SPLIT.md](semantic_audit/TASK_SPLIT.md): the semantic
stage/driver, program/statement traversal, control-flow targets, and the C-owned parser repairs.

## 1. Summary

`SemanticAnalyzer` (`src/semantic/semantic_analyzer.{hh,cc}`) owns A's services
(`Types`, `Symbols`, `Declarations`, `ASTDeclarations`, `ASTAggregates`) and B's
`ExpressionChecker`, walks the program, opens/closes A's scopes, records initialization/jump
state, and reports through A's `Symbols::diagnose`. The driver runs this as a new `SEMANTIC`
stage selected by `--semantic`, rendering diagnostics with source spans.

Verified state: `./custom_test_hook parse` **57/57**, `semantic_audit/teammate_a/run.py` **17/17**,
`semantic_audit/run.py` no crashes (lex 113, parse 96/17), and all 221 audit fixtures classified
with no crashes or timeouts.

## 2. Driver / stage integration (C03)

- `src/types/driver/defs.hh` — `Stage { LEX=0, PARSE=1, SEMANTIC=2, CODEGEN=3 }` and `to_string_view`.
- `src/driver/args.cc` — `--semantic` sets `UptoStage = Stage::SEMANTIC`.
- `src/driver/driver.cc` — after parsing, if `TillStage() >= Stage::SEMANTIC`, constructs
  `SemanticAnalyzer`, calls `analyze(*Program)`, and for every diagnostic casts
  `Entry.source`/`Entry.previous` (opaque `const void*`) back to `types::ASTNode*`, reads
  `getSourceSpan()`, and emits `Gctx::GenerateLineError(...)` / `GenerateLineWarning("previous
  declaration is here")` at `Stage::SEMANTIC`. If analysis fails with no diagnostic, a global
  error is emitted so silent failures cannot pass. The default stage stays `PARSE`, so parser and
  audit behavior is unchanged unless `--semantic` is requested.
- `src/semantic/CMakeLists.txt` — `add_library(semantic ...)` compiling all `src/semantic/*.cc`,
  linking `base`/`parser`; `src/CMakeLists.txt` adds the subdirectory; `src/driver/CMakeLists.txt`
  links `semantic`.

## 3. Traversal and context (C01, C04, C05, C11)

`SemanticAnalyzer` constructor order: `Types`, `Symbols`, `Declarations`, `ASTDeclarations(nullptr)`,
`ExpressionChecker`, `ASTAggregates`, then `ASTResolver.setEvaluator(&Checker)` and
`installTypes`/`installLibrary`.

Traverses: program/declaration lists (with per-declarator initializer callback), typedefs, enums,
struct/class forwards and definitions, unions, and functions (prototype + definition). Aggregate
bodies use class context for methods/ctors/dtors. Functions enter a `Function` scope, register
parameters, evaluate default arguments, set the return/class/static/const context, clear per-function
jump/label/initialization state, then analyze the body; state is restored and the scope trimmed on
exit. Error boundaries: failed nodes set `Failed` and stop the subtree; every failure path emits a
diagnostic (explicit malformed-declarator/function-body diagnostics plus the driver's global-error
fallback).

## 4. Statements and control flow (C06–C20)

- Statements: compound (nested block scopes), expression, empty, if/else-if/else, switch, for,
  while, do-while, return, goto, break, continue, labels, case/default.
- `for` opens a `For` scope, handles expression / declaration-list / typedef initializers, absent
  condition/update, and rejects function declarations in the initializer.
- Conditions (`while`/`do`/`for`/`if`) go through B's `checkCondition` (scalar conversion).
- `return` checks void vs value and B convertibility; `break`/`continue` track loop/switch depth.
- Named loop jumps (`break L`/`continue L`) resolve against an active loop-label stack; trailing
  `do ... while(cond; L)` labels are prebound before the body. Duplicate active labels, inactive/
  undefined targets, and `break`/`continue` with no loop are diagnosed.
- `goto` collects function-wide labels (`PendingGotos`), allows forward jumps, rejects
  undefined/duplicate targets, and keeps functions isolated. A jump-bypasses-initialization pass
  records the initialized-symbol set at each label and each goto and rejects a goto that would skip
  a live initialization.
- `switch` checks the controlling type with B, tracks nested switch contexts (isolated), and
  `case`/`default` use B's `integer()` evaluator with duplicate-case/multiple-default checks.
- `main` is validated: `int`-like return, and either no parameters or `(int, char**)`-compatible.

## 5. C-owned parser repairs (C06–C10)

- `statement.cc`/`compoundStatement.cc` — bare `return`, standalone empty statements, empty bodies.
- `iterationStatement.cc` — for-header clause delimiters consumed once; all omitted-clause
  combinations and all five previously dropped label positions preserved; loop suffix requires a
  separator plus an identifier; empty/numeric/multiple suffixes rejected; one type-name collision
  policy across loop kinds; labels kept out of runtime expression clauses.
- `labelStatement.cc`/`selectionStatement.cc`/`jumpStatement.cc` — labeled statements own their
  statement (including unbraced loop bodies, labeled `do` bodies, and dangling-else); general
  switch body statement; goto/case/default label ownership.
- `expressions.cc` — unparenthesized comma in the middle conditional operand.
- `declaration.cc` — `struct X`/`class X` used where `X` names a non-struct type is diagnosed.
- `function.cc` — variadic marker and duplicate-specifier diagnostics (shared with B).

## 6. C01–C24 coverage

| Package | Where |
| --- | --- |
| C01 AST access/source spans | `ASTSourceSpan`, node `getSourceSpan()`, C-owned AST headers |
| C02 source locations/diagnostics | `driver.cc` span rendering, previous-declaration notes |
| C03 driver/build wiring | `defs.hh`, `args.cc`, `driver.cc`, `semantic/CMakeLists.txt` |
| C04 traversal/context | `SemanticAnalyzer` + context stacks |
| C05 recovery/error boundaries | `Failed` gating, explicit diagnostics, global-error fallback |
| C06 basic statement grammar | `statement.cc`, `compoundStatement.cc` |
| C07 for-header parsing | `iterationStatement.cc` |
| C08 loop suffix grammar | `iterationStatement.cc` |
| C09 labeled statement ownership | `labelStatement.cc`, `selectionStatement.cc` |
| C10 general switch grammar | `selectionStatement.cc` |
| C11 function boundaries/returns | `analyzeFunction`, `analyzeConstructor`, `analyzeDestructor` |
| C12 condition checking | `checkCondition` calls |
| C13 for scope/empty clauses | `analyzeStatement` (for branch) |
| C14 ordinary break/continue | depth counters in `analyzeStatement` |
| C15 named loop jumps | `LoopLabels`, trailing-do prebind, `noteLoopLabel` |
| C16 label namespaces/collisions | `FunctionLabels`, `LoopLabels`, `noteLoopLabel` |
| C17 goto resolution | `PendingGotos`, `JumpTargets`, `JumpSources` |
| C18 switch contexts | `Switches` stack |
| C19 case values/defaults | B's `integer`, duplicate/multiple-default checks |
| C20 jump lifetimes/cleanup | `Initialized`, `trimInitialized`, bypass pass |
| C21 warnings/dataflow | not implemented (see gaps) |
| C22 language policy integration | documented policy choices below |
| C23 audit harness/ownership | `semantic_audit/` runner + fixtures (do not commit audit fixtures) |
| C24 pipeline integration checks | parse hook + teammate A `run.py` + audit classification |

## 7. Known gaps (policy-conditional, documented, not silently accepted)

- Warnings (C21): missing return, unreachable code, uninitialized/unused, lifetime warnings are
  not emitted; the language policy treats them as warnings, not errors.
- VLA/`new`/`delete` are unsupported by policy and diagnosed rather than accepted.
- Loop-label namespace decisions: active duplicate loop labels are errors; sibling reuse after exit
  is allowed; ordinary goto labels and loop suffixes are separate namespaces.
- Labels before declarations and at block ends are accepted. Empty brace initialization is accepted.
- Long-form type spellings such as `long long` remain a parser decision (not enabled); C11-style
  for-clause storage restrictions are not applied.
- Execution-only jump tests (outer-for continue, outer-do continue, named break through a switch)
  are deferred until code generation exists.

## 8. Reproduce

```bash
cmake --build build --parallel 2
./custom_test_hook parse                     # 57 PASS
python3 semantic_audit/teammate_a/run.py     # {"checks":17,"passed":17}
python3 semantic_audit/run.py                # lex/parse snapshot, no crashes
for f in semantic_audit/programs/*.cpp semantic_audit/extended/programs/*.cpp; do
  ./build/baka --semantic "$f" >/dev/null 2>&1; done   # 0 crashes/timeouts
```
````

## `src/semantic/README.md`

Current repository document; inspect the verified-source caveats in sections 6–7.

````markdown
# Semantic analysis

Semantic analysis connects the parser AST to the type and symbol services before
lowering. The implemented pass order is:

```
declaration registration -> resolve/rename -> type annotation -> read-only checks
```

This directory contains both the existing rule implementations and the orchestration
that invokes them. The remaining acceptance and regression tasks are tracked in [TODO.md](TODO.md). A rule
existing in a helper does not establish that every AST context invokes it correctly;
end-to-end regression programs remain part of the acceptance criteria.

## Services and ownership

| Files | Responsibility |
| --- | --- |
| `types.hh`, `types.cc` | Stable `TypeId` values, canonical derived types, nominal aggregate identity, completeness, member lookup, LP64 layout |
| `symbols.hh`, `symbols.cc` | Stable `SymbolId`/`ScopeId` values, lexical scopes, declaration compatibility, diagnostics |
| `declarations.hh`, `declarations.cc` | Rules for type formation, object storage, aliases, aggregate members and enumerators |
| `ast_declarations.hh`, `ast_declarations.cc` | Interpret parser specifiers/declarators using the current scoped symbol table |
| `aggregates.hh`, `aggregates.cc` | Register aggregate definitions, signatures, inheritance and layout |
| `expressions.hh`, `expressions.cc` | Expression types, constants, conversion validation, calls and initializer rules |
| `conversions.cc` | Materialize checked implicit conversions and compound computations on AST edges |
| `semantic_analyzer.hh`, `semantic_analyzer.cc` | Public phase API, saved contexts, service construction and orchestration |
| `registration.cc` | Top-level registration, callable/member signatures, declaration/body collection |
| `resolution.cc` | Source-order local registration, binding/renaming, statement and expression work collection |
| `annotation.cc` | Expression annotations and cached results, initializer typing and finalized array metadata |
| `checks.cc` | Const validation of queued results, storage policy, returns, switches and jumps |

Zero IDs represent an invalid result. IDs belong to one analyzer's arenas and must
not be reused with another analyzer. Source pointers identify existing parser nodes;
they are not owning pointers. The AST and arenas must outlive consumers of their
semantic metadata.

## Public API and phase state

`SemanticAnalyzer::analyze(ProgramNode&)` invokes these methods in order:

| Method | Resulting `SemanticPhase` | Main output |
| --- | --- | --- |
| `registerDeclarations(ProgramNode&)` | `Registered` | Top-level declarations, signatures and body work |
| `resolveAndRename()` | `Resolved` | Scoped bindings, unique names and contextual expression/control work |
| `annotateTypes()` | `Annotated` | AST type/value metadata and cached `ExprResult` values |
| `check()` | `Checked` | Read-only validation diagnostics |

An analyzer begins in `Parsed`. Each method requires its preceding phase; its
boolean result reports whether diagnostics are empty. `analyze()` stops after
invalid top-level registration, but continues resolution, annotation and read-only
checks after recoverable local errors to diagnose independent work. Error-valued
expressions suppress cascades, and AST conversion materialization is skipped when
errors exist. A `Checked` phase alone is therefore not proof of success: require
empty `diagnostics()` before lowering. `phase()` supports intermediate inspection.
`types()` and `symbols()` expose const service views. The three work-count getters
support structural tests without exposing mutable queues. Use one analyzer for one
AST; this API does not promise a reset/reuse operation.

## AST metadata

`ASTNode::semanticInfo()` holds an `ASTSemanticInfo` record. `Type`, `Symbol`,
`Scope` and `Target` are analyzer-owned arena IDs or control-target identities;
zero means unavailable. `Resolved`, `Annotated` and `Error` describe processing
state. `LValue`, `ConstantExpression`, `HasConstant`, `NullPointerConstant` and
`Constant` carry integral-expression facts. `HasFloatingConstant` and
`FloatingConstant` retain supported floating constant results. `CallSignature`,
`MemberOwner`, one-based `MemberIndex` and `Construction` retain selected call/member
facts for lowering. The parser initializes this record empty.

`IdentifierNode::GetName()` preserves source spelling. `resolve()` stores candidate
symbol IDs and an owned unique name of the form `sourceName.SymbolId`, available
through `getBindings()` and `getResolvedName()`. Overload candidates can require
multiple bindings before call selection. The symbol ID, rather than the generated
spelling, is the binding identity.

## Pass contracts

### 1. Declaration registration

Walk the program's top-level declarations once and collect ordered work items.
Collect aggregate members, function signatures, bodies and initializers as their
owners become known. Preserve original source order and lexical ownership even
when later processing groups items by kind.

Registration is an index of work, not permission to use every collected declaration
from every source location. A later declaration must not become visible earlier
merely because it is already in a queue. Preserve declaration-point information or
activate each binding in source order. Function bodies can recurse through their
own registered signature. All top-level function headers are collected before
body resolution, so forward function calls are supported. Global object references
retain a source-order visibility horizon. Forward goto labels are deliberately function-wide.

Do not repeatedly search the program root to find functions or aggregate bodies.
Queue their pointers when discovering the declaration. Declaration-time type
dependencies are handled in the live environment as described below; registration
is not a general fixed-point solver for arbitrary forward type dependencies.

### 2. Resolve and rename (mutating)

Create and enter lexical scopes, activate declarations in order, and bind identifier
uses through the **current scoped symbol table**. Resolve type names through that
same environment, including typedefs, shadowing and aggregate-kind modifiers.

Attach the resulting binding identity to each relevant AST node. A stable
`SymbolId` is the semantic identity even when source spelling is unchanged. The
resolver stores a separate owned unique name while retaining original spelling
for diagnostics. Never use textual rewriting as a substitute for
symbol identity.

Record resolved type-name identities on type AST nodes, and enqueue expressions,
initializers and statements with their owning scope/function/class context. Register
an implicit `this` binding only in instance member contexts. Function-local goto
labels and active named-loop targets have different lifetime and lookup rules.

### 3. Type annotation (mutating)

Declaration registration/resolution has already built signature and object types
from scoped specifiers and nested declarators. This pass uses those types to annotate
expressions with their `TypeId`, value category, constant-evaluation result and
selected symbol/call target where applicable. Annotate declarations with the final
object/function type, recording a previously inferred array shape where applicable.

The annotated form must preserve `f(a,b)` versus `f((a,b))`. Type construction must
also preserve `int *a[3]` versus `int (*a)[3]`. Qualifiers must survive typedef
resolution and member access. Array/function decay is contextual; retain the
undecayed information needed by address-of and `sizeof`.

`ExpressionChecker` caches expression results and writes AST annotations during
annotation. Original AST `Type` records the intrinsic undecayed type and `LValue` records
the corresponding value category. A separate cached `ExprResult` records the
contextual value conversion/decay. This distinction preserves array identity for
`sizeof` and address-of even when a value use converts the same expression to a
pointer. After typing succeeds, `conversions.cc` rewrites value-use edges with
`ImplicitCastNode` (see below), so the final AST explicitly contains those
contextual conversions. Resolved identifier uses consume their saved bindings rather than
performing a later name lookup that could select a different declaration.
Intrinsic expression/call/type errors and initializer shape/conversion errors are
diagnosed here: they are prerequisites to producing valid annotations. This phase
finalizes and records previously inferred array shape and updates declaration
metadata. Initializer services also support deferred inference when called directly;
the orchestrated pipeline establishes shape earlier for dependent declarations.

Array bounds and enum values are declaration-time dependencies and may already
have been evaluated before this phase, using their original live environment.

### 4. Read-only checks

Consume resolved bindings, AST annotations and queued `ExprResult` values to
validate conditions, returns, defaults, storage requirements, switches and jumps. Diagnostics and
checker-local bookkeeping may change; AST nodes, symbol definitions and type
identities must not change during this phase.

Checks must not silently resolve a missing name, infer an array bound, install a
member, or annotate an expression. Those are earlier-pass responsibilities.
`checkReadOnly()` is a const method returning diagnostics; `check()` appends them
and advances phase state. The final pass does not call `checkValue()`, resolve
types or re-evaluate expressions. Calls to const conversion/layout queries are
permitted. Error metadata prevents a failed annotation becoming an ordinary `int`.

## Work-item context

The queues are `TopLevel`, `Bodies`, `Expressions`, `Flows` and `Switches`.
Registration fills declaration/body work; source-order resolution fills expression,
label/jump and switch work while each body's lexical structure is available.

Each expression/body context retains `scope`, class owner, function result type,
static/const-member flags and a global visibility limit. `select()` restores the
context through `Symbols::selectScope()` and the visibility horizon. Expression
work also records its use (condition, initializer, default, return, switch or case),
expected type, associated symbol and cached result. Flow work stores concrete
control targets and initialized declarations. Switch work indexes the expression
queue so checks use the already annotated condition and cases.

Local names are bound as declarations are inserted in source order. Their saved
binding IDs freeze that decision before later declarations enter the same scope.
Named-loop lookup uses the active enclosing loop stack, not a global label map.

Scope arenas persist after leaving a scope, so attached `ScopeId` and `SymbolId`
values remain meaningful. A saved scope alone would not enforce declaration-point
visibility; the frozen bindings and global visibility horizon provide that rule.

## Validation

The focused suite is in `custom_tests/semantic`. Build and run it with:

```sh
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./custom_test_hook semantic
./custom_test_hook parse
```

The build directory must first have been configured with `cmake -S src -B build`.
`pass_tests.cc` links the actual lexer, parser and semantic library. CTest registers
the same ten structural scenarios exercised by the Python runner:

| Scenarios | Contract checked |
| --- | --- |
| `registration`, `registration_error` | Top-level/body work collected once; function bindings attached; bodies deferred; duplicate definitions fail registration |
| `resolution`, `resolution_error` | Shadowed locals receive distinct owned names; expression types remain deferred; use-before-declaration fails resolution |
| `annotation`, `annotation_error` | Expression types appear on the AST; work queue exists; invalid floating bitwise operands fail annotation |
| `checks`, `checks_error` | Final checks preserve sampled AST metadata and symbol/type counts; named break has a resolved target; illegal break fails checks |
| `resolution_loop_targets` | Nested labeled/unnamed loop jumps keep distinct target IDs, including across switches; IDs are printed and retained after annotation/checks |
| `resolution_this` | Instance `this` is registered/bound and then annotated as the implicit-object pointer |
| `annotation_scoped_type` | Shadowed typedef identities attach to their original type operands; `sizeof` uses the correct scoped type |

`cases.json` lists 155 end-to-end programs in `cases/`. `run.py` checks each exact
exit status and, for rejected inputs, an error diagnostic from the expected stage.
Crashes and timeouts fail the suite; a rejected input is not accepted merely because
the process exited unsuccessfully. `./custom_test_hook semantic` combines these
programs with eleven phase scenarios, six conversion/metadata scenarios and 32
multiline diagnostic/recovery fixtures in `diagnostics.json`, plus the member-call structural scenario, for **205 checks**. The conversion scenarios inspect the rewritten AST, not just exit status.

The fixtures cover declaration conflicts, types, mutation, calls/defaults,
initializers, returns, members, labels and switches. Positive controls include
recursion, shadowing, compatible typedef repeats, forward goto/function calls,
function pointers, incomplete/self pointers, inheritance, member defaults and named
loop jumps. Dedicated regressions cover declaration-specific defaults, `for`
initializers and early array-shape dependencies, `main(int argc,char **argv)`,
converted duplicate cases, static local
addresses, comma argument nesting and const/type-name dependencies.

The structural final-pass tests compare selected metadata and arena counts; the
const `checkReadOnly()` implementation supplies the broader architectural guard.
These tests are useful evidence, not a proof that every possible syntax form or
standard rule is implemented. [TODO.md](TODO.md) retains the remaining audit scope.

## Declaration-time type formation prerequisites

Explicit array bounds and enum initializers can be needed while registering a declaration's
signature. They form a deliberate prerequisite exception to the broad phase order:
a declaration-time evaluator may resolve and annotate the required expression in
the live declaration environment. This is not the final read-only checker and must
not make unrelated expressions or later declarations visible. Save its annotations
for subsequent phases instead of re-evaluating it in a different scope.

`ASTDeclarations::inferArrayBound()` is another type-formation prerequisite. During
registration/resolution it determines an omitted outer array bound from initializer
**shape**, before later `sizeof` expressions or array-bound dependencies need the
object's completed shape. It handles braced character strings and flat brace-elided
multidimensional arrays; an empty initializer cannot complete an unsized array.
This syntax-driven inference does not annotate general expressions or validate
element conversions. Element typing and scalar/aggregate initializer checks remain
in annotation, including support for scalar braces.

Defaults are different: a default argument belongs to a declaration, not to a
canonical function type. Interning makes unrelated same-signature functions share
a `TypeId`; therefore required argument counts must be keyed by callable/declaration
identity, not solely by that `TypeId`. Calls through a plain function pointer do not
inherit declaration defaults merely because their signature is equal.


## Current language policies and limits

- Target layout is fixed LP64; target selection is not implemented.
- Explicit array bounds must be positive integral constants; VLAs are unsupported.
  Omitted bounds are supported where parameter adjustment or initialization supplies
  their meaning, and final object storage must be complete.
- Globals and local statics require constant initializers. Taking an automatic
  local's address does not make it a valid static-storage initializer.
- Const scalar objects require initialization; aggregate/constructor behavior must
  follow the accepted initialization rules rather than assuming every subobject
  independently requires an explicit source initializer.
- Duplicate active loop labels are rejected. Reusing a label in sibling loops is
  allowed. Goto labels are function-wide; named break/continue target enclosing
  loops, including a trailing do-while label visible in its body.
- Nested function definitions are unsupported. Top-level function forward calls
  are supported; global objects retain declaration-order visibility.
- Parameters require names. `main(void)`, `long long` declarations and `new`/`delete`
  are outside the implemented syntax even though some related tokens/literals exist.
- `main` is an ordinary function during semantic analysis: it has no special
  return-type or parameter-shape restrictions. Normal declaration, call and return
  compatibility checks still apply. Future executable entry-point/ABI requirements
  belong to an explicitly chosen target policy.

Local aggregate definition syntax remains a parser gap: the legacy
`valid_aggregate_shadow` case is excluded rather than being counted as a semantic
success. The arithmetic constant evaluator handles the implemented subset and is
not a complete standard evaluator for every literal format, conversion and target
arithmetic edge case.

The implementation is a C-like/C++-like subset, not a claim of complete C or C++
standard conformance. The comprehensive checklist remains a regression/audit scope;
helpers implementing a rule do not prove every syntax path is covered. Warning
analysis, cleanup lowering and code generation are not implemented by these passes.


## Explicit implicit conversions for lowering

`types/parser/ast/implicitCast.hh` defines `ImplicitCastNode`. The parser never
creates it. During `annotateTypes`, after all work items have been typed,
`ExpressionChecker::materializeConversions()` rewrites actual child/root slots
using `ASTNode::rewriteExpressions()`. This API enumerates **direct expression
slots only**; it does not repeat declaration or statement discovery. Declaration
bounds/enumerator roots remain compile-time expressions; their operands can still
carry conversions. Original work results remain available for final diagnostics.

Every conversion exposes its operand, source/destination `TypeId`, conversion kind,
source span and annotated rvalue result. Kinds distinguish lvalue loads,
array/function decay, integral promotion, numeric conversion, boolean testing,
null-to-pointer, pointer conversion and pointer qualification. Explicit source
casts remain `CastFactorNode`. No redundant same-type numeric cast is emitted.

For example `double d = s + 1;` with `short s` receives a load and promotion on
`s`, an `int` addition, then an `int -> double` conversion on the initializer.
Assignments keep their left operand as an addressable expression. `sizeof(a)`
keeps the array operand intact, while a value use of `a` receives array decay.
Conditions receive boolean conversions; shifts promote operands independently;
comparisons use common arithmetic/pointer types. Fixed call arguments, default
parameter expressions, return expressions and scalar aggregate/array initializer
elements receive checked destination conversions. Ellipsis arguments receive
integer promotions and `float -> double`. The outer argument-list comma container
is preserved, including the distinction between `f(a,b)` and `f((a,b))`.

Compound assignments become `CompoundAssignmentNode(Target, Value)`. Lowering
must evaluate **Target's address once**, bind it for the Value subtree, evaluate
Value and store it. `CompoundTargetNode` refers to that saved address as an lvalue;
an explicit load/promotion chain uses it in the arithmetic expression, followed by
the conversion back to the stored type. Thus `a[i++] += 2.5` does not duplicate
`i++`. The compound assignment itself denotes the target lvalue under the
project's existing expression rules. Prefix/postfix increments remain primitive
mutation nodes and also evaluate their operand address once.

Calls now retain `ASTSemanticInfo::CallSignature`. Selected fields/methods and
constructors retain `MemberOwner` and one-based `MemberIndex`; a constructor call
also sets `Construction`. `Symbol` identifies ordinary selected callees. These
facts are independent of the expression checker's temporary caches.

`--semantic` prints `Semantic::AST` after successful analysis, making
conversion nodes visible. Parser output still shows the original syntax tree.
The new nodes use the parser's shared `ASTNodeArena` allocation convention; they
are not owned by the checker, and their type IDs require its type arena to remain
alive. See [IR_READINESS.md](IR_READINESS.md) for the lowering boundary and gaps.


## Printed control-flow identities

`semanticInfo().Target` is the control identity: loop, switch and goto-label
nodes define it, while `break`, `continue` and `goto` nodes refer to their resolved
destination. A named jump's label identifier also stores the same Target and is
marked Resolved. This is separate from ordinary variable/function SymbolIds.

AST printing includes `Target: N` on those definitions and uses after resolution.
For nested `for(;;;a)` / `for(;;;b)`, `continue a` prints the outer loop's ID;
an unnamed continue prints the inner loop's ID. Zero IDs are omitted so parser-only
AST output remains unchanged. IR maps a continue's loop ID to its update/condition
block and a break's ID to its exit block; the numeric ID does not itself denote
a machine or IR instruction address.


## Diagnostic source locations and recovery

Parser allocations use `AllocNodeAt(tokenIndex, ...)`: capture the introducing
token before parsing children or bodies. There is no last-consumed-token fallback.
Statement/declaration nodes anchor their beginning, identifiers/literals their own
token, and binary/postfix expression nodes their operator. This prevents composite
nodes from accidentally pointing to a closing brace, semicolon, or child statement.
Semantic conversion nodes inherit the source location of the operand/computation.

Condition, switch, case-value, default-conversion and return-conversion errors
prefer the offending expression. Duplicate active loop labels point at the label
identifier (including a do-while suffix). Declaration conflicts retain both the
new declaration and previous declaration source. Diagnostic regression programs
assert message fragments, exact line/column, the displayed source line and previous
notes, not merely an unsuccessful exit status.

Recoverable local name/type errors do not suppress later independent diagnostics.
For example a local redeclaring parameter `a` and a later `foo(d)` passing an
aggregate to a `char` parameter both produce errors. Unresolved expression uses
produce an error result without repeating the resolver's diagnostic; final checks
skip dependent switch/case facts when their types could not be established. Invalid
top-level registration still stops the pipeline because its declaration/body work
may be incomplete. No erroneous compilation prints a successful semantic AST or
becomes eligible for IR generation.


## Instance-member calls versus ordinary function decay

A selected nonstatic method is a bound designator: its call keeps the receiver
AST and `MemberOwner`/`MemberIndex`/`CallSignature`, without `FunctionToPointer`.
Dot receivers remain addressable; arrow receivers load their object pointer.
Implicit calls retain the member identity and use the current instance context.
Parentheses around a selected method preserve this behavior. Static methods and
free functions still permit ordinary function decay. Function-pointer fields are
indirect callees: their expression remains pointer-typed and receives a pointer
load, rather than being rewritten into a method or function designator.

`ExprResult::boundInstanceFunction` prevents an instance method from silently
becoming an ordinary function pointer in initializers, arguments, bare value uses
or address-of expressions. Pointer-to-member types are not implemented, so these
uses receive a diagnostic. Calling an implicit instance method from a static
context also requires an object. `member_call_tests.cc` inspects AST edges for
explicit/implicit, dot/arrow/parenthesized instance calls and function-pointer
fields, while retaining free/static function decay controls.
````

## `src/semantic/IR_READINESS.md`

Current repository document; inspect the verified-source caveats in sections 6–7.

```markdown
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
| Indirect calls and overload choice | Call `CallSignature`; ordinary selected function `Symbol`; callee value load/decay preserved |
| Explicit field/method access | `MemberOwner`, one-based `MemberIndex`, selected symbol when available; retain receiver AST |
| Explicit construction expressions | `Construction`, selected constructor `CallSignature` and owner/member index; implicit construction has no selected constructor index |
| Field offset | `types.layout({MemberOwner}).layout.offsets[MemberIndex-1]`; inherited access also needs the receiver-to-owner base path |
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
```

## `src/semantic/TODO.md`

Current repository document; inspect the verified-source caveats in sections 6–7.

```markdown
# Semantic analysis: grouped implementation and review checklist

Unchecked entries are acceptance work, not a claim that no corresponding code
exists. Check an entry only after reviewing all relevant contexts and running its
positive and negative controls. Checked regression entries below refer to the
155 ordinary fixtures, 32 diagnostic fixtures, eleven phase scenarios and six conversion scenarios plus a member-call scenario documented in README; they do
not imply coverage of every possible variant. [README.md](README.md) defines the pass contracts.

The four public phase methods and the five work queues are implemented. Source
inspection confirms AST metadata, owned resolved names, persistent scopes and a
const final checker. The checklist below deliberately retains broad behavioral
items until regression evidence covers their relevant contexts; it is not a
percentage-complete report.

## Implemented architecture (source inspection)

- [x] Public `registerDeclarations`, `resolveAndRename`, `annotateTypes`, `check` and `analyze` API with `SemanticPhase` transitions.
- [x] Separate `registration.cc`, `resolution.cc`, `annotation.cc` and `checks.cc`, following the parser's split-file organization.
- [x] `TopLevel`, `Bodies`, `Expressions`, `Flows` and `Switches` queues with saved contexts and cached results.
- [x] `ASTSemanticInfo`, identifier binding IDs and owned resolved names; source spelling remains available.
- [x] Persistent scope arenas and context selection; resolved expression names consume saved bindings.
- [x] Const `checkReadOnly()` consumes annotations/work results without registration or expression evaluation.
- [x] Document declaration-time bounds/enumerators evaluation, syntax-only omitted-array shape inference and intrinsic diagnostics during annotation.

## Pipeline acceptance and shared metadata

- [x] Execute declaration registration, mutating resolution/rename, mutating type annotation, then read-only checks as distinct phases.
- [x] Collect ordered declaration/body/initializer work items during discovery; avoid repeated program-root searches.
- [ ] Preserve declaration-point visibility when declarations are indexed ahead of use.
- [x] Resolve type names in their current lexical symbol table and attach the resulting metadata to the AST.
- [x] Attach stable binding identity and resolved type/value-category metadata; preserve source spelling and locations for diagnostics.
- [x] Ensure the final checks consume annotations without mutating the AST, symbol definitions or type arena.
- [x] Document arena lifetimes, error results, recovery behavior and phase prerequisites.

## Registration and name resolution

- [ ] Reject undeclared/out-of-scope identifiers and use before declaration; permit legitimate inner-block shadowing and recursion.
- [ ] Reject duplicate locals/parameters, conflicting object/function declarations, incompatible prototypes and repeated function definitions.
- [ ] Resolve typedefs; accept compatible repeats and reject incompatible repeats or aggregate-name conflicts.
- [ ] Register aggregate members, enum constants and callable signatures without discarding duplicate declarations.
- [ ] Register `this` in instance members and reject it in static methods or outside an instance context.
- [ ] Resolve goto labels function-wide and named-loop targets only while the target loop is active.
- [ ] Reject nested function definitions deliberately.

## Type annotation and declaration rules

- [ ] Build nested pointer/array/function types correctly; reject arrays of void and direct array/function return types.
- [ ] Reject void objects, incomplete by-value objects/members and recursive by-value aggregates; permit pointers to incomplete/self types.
- [ ] Reject object declarators with function bodies.
- [ ] Check aggregate-kind modifiers and unsigned modifiers after typedef resolution without assertions.
- [ ] Check duplicate storage specifiers for definitions and ordinary declarations consistently.
- [ ] Require integral nonnegative valid array bounds; infer supported omitted bounds from initializers.
- [ ] Record variadic signatures, parameter adjustments and declaration-specific default arguments.
- [ ] Propagate const through aliases, aggregate receivers and recursively const members.
- [ ] Evaluate explicit/implicit enumerator values with correct declaration-point visibility and range checks.

## Expression and conversion checks

- [ ] Reject assignment/mutation of constants, non-lvalues, arrays and const-qualified targets.
- [ ] Validate dereference, address-of and indexing operands and result categories.
- [ ] Validate arithmetic, shifts, bitwise, logical and comparison operands, including pointer arithmetic and pointer multiplication rejection.
- [ ] Apply promotions and implicit conversions consistently, including null pointer constants and qualifier preservation.
- [ ] Require scalar non-void conditions.
- [ ] Validate casts; reject array destinations and apply the documented aggregate-cast policy.
- [ ] Validate `sizeof`, including void/functions/incomplete objects and pointers to incomplete types.
- [ ] Resolve member lookup/inheritance, nonexistent members and `.`/`->` receiver requirements.

## Calls, returns and initialization

- [ ] Reject non-callable targets; validate function pointers and selected callable candidates.
- [ ] Check required/excess argument counts, variadics, defaults and argument conversions.
- [x] Keep defaults associated with declarations/symbols: unrelated functions sharing an interned signature must not share their required argument count.
- [ ] Validate default expressions in their declaration environment and check conversion to the parameter type.
- [ ] Preserve the distinction between comma-separated arguments and a parenthesized comma-expression argument.
- [ ] Validate bare/value returns against the containing function's result type.
- [ ] Check scalar/aggregate/string initializers, excess array elements and inferred dimensions.
- [ ] Apply the selected const-initialization and static/global initialization rules in every context, including `for` declarations.
- [ ] Diagnose duplicate member names, enumerators, constructor signatures and destructors.

## Read-only control-flow checks

- [ ] Validate ordinary break/continue against enclosing loops/switches.
- [ ] Validate undefined, expired and duplicate active loop labels according to the documented policy.
- [ ] Bind trailing do-while labels before analyzing/checking their bodies.
- [ ] Diagnose duplicate/undefined goto labels and prohibited jumps over initialization.
- [ ] Require integral switch conditions and integral constant cases.
- [ ] Detect duplicate cases after conversion to the promoted switch type and multiple defaults per switch.
- [ ] Reject case/default outside switches; preserve nested-switch independence.

## Parser/AST prerequisites and diagnostics

- [ ] Parse bare `return;` and standalone empty statements.
- [ ] Preserve omitted array bounds, explicit enum values, variadic markers and union names.
- [ ] Preserve every constructor/destructor declaration for duplicate checking.
- [ ] Preserve labeled-loop header/body structure across `for`, `while` and `do-while` forms.
- [ ] Replace invalid unsigned-type assertions and conflicting enum/struct exceptions with source-located diagnostics.
- [ ] Reject unsupported syntax predictably rather than silently dropping AST information.

## Language policy acceptance checks

- [ ] Verify the documented no-VLA, positive-constant-bound and inferred/adjusted omitted-bound policy in every declaration context.
- [ ] Verify the documented constant global/local-static initialization policy, including automatic-local addresses.
- [ ] Verify const scalar initialization requirements and the accepted aggregate/constructor behavior.
- [ ] Cast/implicit conversion rules and permitted aggregate conversions.
- [ ] Verify forward function calls, source-order global object visibility and local declaration-point lookup.
- [ ] Duplicate active loop labels, loop/goto namespaces and goto bypassing initialization.
- [ ] Verify unsupported `long long` declarations and `main(void)` are diagnosed; keep parameters named.
- [ ] Verify unsupported `new`/`delete` syntax is diagnosed despite keyword recognition.

## Regression coverage

- [x] Add focused custom tests for each of the four phases, including assertions on binding/type metadata.
- [x] Verify final checks do not mutate semantic annotations, symbol definitions or types.
- [x] Assert diagnostics and nonzero status for invalid cases; assert no crashes.
- [x] Retain valid recursion, shadowing, compatible typedef repeats and forward goto.
- [x] Retain valid self/incomplete-type pointers and `sizeof` of their pointers.
- [x] Retain valid function pointers, members, inheritance and default arguments.
- [x] Retain valid labeled break/continue and empty `for` clauses.
- [x] Test comma argument nesting, declaration-order visibility and defaults on unrelated same-signature functions.
- [ ] Reuse confirmed audit examples as regressions when the corresponding rule is accepted.

## Review findings from the implementation before this revision

These describe the reviewed baseline, not a permanent statement about the current
code. Keep the regression after fixing each defect.

- [x] Replace the single interleaved traversal with explicit phase orchestration and recorded work/context (source inspection; structural regression remains in the coverage section).
- [x] Replace transient expression results with persistent AST binding/type annotations (source inspection).
- [x] Remove default-argument required-count leakage through canonical function-type IDs.
- [x] Run initializer and const/static-storage checks for `for` declaration initializers, using the same rules as ordinary declarations.
- [x] Treat `main` like any other function; remove special signature restrictions while retaining ordinary function/return checks.


## Explicitly outside this implementation

- Warning-only analyses such as unused declarations and unreachable code.
- Cleanup/destruction lowering and code generation.
- Full C/C++ standard coverage beyond the documented language subset.


## Remaining concrete limitations

- [ ] Add parser support for local aggregate definitions before enabling the excluded legacy `valid_aggregate_shadow` control.
- [ ] Broaden arithmetic constant evaluation deliberately; current passing fixtures do not establish full standard coverage of all formats and arithmetic edge cases.
- [ ] Expand final-pass immutability tests beyond sampled metadata and arena counts if mutable service interfaces are introduced later.


## Array-shape regression batch

- [x] Infer omitted outer array bounds from initializer shape during registration/resolution before later `sizeof` and bound dependencies.
- [x] Retain controls for braced character strings, scalar braces and flat brace-elided multidimensional inferred arrays.
- [x] Reject an empty initializer for an unsized array rather than leaving an apparently complete object.
- [x] Keep initializer element typing/conversion in annotation; early shape inference is not general expression annotation.


## Implicit conversion AST and IR boundary

- [x] Insert explicit load, decay, promotion, numeric, boolean and pointer conversion nodes during annotation.
- [x] Rewrite owning AST slots for initializers, arguments/defaults, returns, conditions, switches and expression operands.
- [x] Preserve assignment/address targets, unevaluated array operands and argument nesting.
- [x] Represent compound assignment computation with a saved target address and explicit read/store conversions.
- [x] Persist selected call signatures and aggregate member/constructor identities independently of checker caches.
- [x] Add structural tests for conversions, single target evaluation, persistent call/member facts and final-pass immutability.
- [x] Review the IR input contract and record the remaining integration/normalization work in [IR_READINESS.md](IR_READINESS.md).
- [ ] Keep semantic arenas alive in the driver across future IR generation.
- [ ] Normalize aggregate initialization and per-call defaults before lowering those features without semantic reconstruction.
- [ ] Expose cleanup/construction plans before lowering nontrivial object lifetimes and jumps.

- [x] Print resolved control target IDs and mirror named jump bindings onto their label identifiers; test nested `for` targets across switches and later passes.


## Diagnostic locations and independent-error recovery

- [x] Require explicit token anchors for every parser AST allocation; capture statement/declaration starts before descending into children.
- [x] Anchor identifiers/literals at their tokens and binary/postfix expressions at their operators.
- [x] Point switch/case/condition/default/return conversion diagnostics at offending expressions and duplicate loop labels at their label identifiers.
- [x] Retain accurate previous-declaration notes; add multiline tests checking line, column and displayed source text.
- [x] Continue after recoverable local resolution/type errors so later invalid calls and errors in other functions are reported.
- [x] Suppress unresolved-expression cascades and keep conversion rewriting/IR eligibility gated on successful analysis.


## Bound member-function calls

- [x] Keep instance-method callees bound to their receiver; do not decay them to ordinary function pointers.
- [x] Preserve ordinary decay for free/static functions and pointer loads for indirect calls, including function-pointer fields.
- [x] Preserve selected member/signature metadata through parentheses, implicit calls and dot/arrow receivers.
- [x] Reject bound-method value/address conversions without pointer-to-member support; require an object for implicit instance calls in static contexts.
- [x] Add independent structural member-call tests and source acceptance/rejection fixtures.
```

## `semantic_audit/teammate_a/POLICY.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

```markdown
# Teammate A: type and declaration contract

These are explicit implementation choices for the existing C++-like language. They do not claim ISO C/C++ conformance. B/C should consume these choices or coordinate a change here before changing behavior.

## Target and identity (A01/A02)

The target is LP64: 8-bit bytes; signed plain char; char/bool storage 1 byte, short 2, int/float/enum 4, long/double/pointers 8. Alignment equals the primitive storage size. Bool has one value bit. Integer ranks are bool 0, char 1, short 2, int 3, long 4; unsigned counterparts share rank. Float/double use arithmetic ranks 5/6. The floating representation contract is IEEE binary32/binary64; B owns literal rounding and target arithmetic. `sizeof` returns unsigned long. `Types::arithmetic`, `layout`, and `sizeType` expose this information without consulting the host ABI.

Types use canonical IDs for builtin/derived types and nominal IDs for struct/class/union/enum declarations. Aliases retain the underlying ID, including completion state. Const is a qualification, not a new aggregate identity. Const-qualified array aliases qualify their elements. Error is ID zero; services never substitute int for an unavailable semantic result. IDs and arena object references remain stable for the service lifetime.

## Names and declarations (A03–A08)

Types, aliases, objects, enumerators, and functions share an ordinary lexical namespace. Struct/class spellings are interchangeable for the same nominal declaration; enum and union are distinct. Lookup stops at the first scope containing a name. An inner scope may hide an outer name, including a type name. Goto/loop label namespaces belong to C and are separate from this API.

Declarations are processed in source order. Resolve the base specifier once, evaluate/construct each declarator (including its bounds), insert its symbol, then check its initializer before visiting the next declarator. `ASTDeclarations::declarationList` implements this ordering. Function predeclaration must never insert later local objects. Function parameters and the outer function body share one Function scope; nested blocks create new scopes. Parameter signature adjustment and parameter-object type are separate: scalar `const int x` keeps const on the object, but its signature parameter is int.

An object declaration allocates storage unless the caller explicitly asks for a type-only/prototype check. Ordinary file-scope objects are definitions with external linkage; file-scope static objects/functions have internal linkage; automatic and local static objects have no linkage. Local static storage is recorded separately. Compatible function prototypes and typedef redeclarations are allowed, without requiring a local external definition. Repeated local declarations, conflicting kinds/types/linkage, return-type-only overloads, and repeated definitions are errors. Supported source storage syntax is `static`; no extern/tentative-definition extension is inferred. `Declaration` allows C to record explicit definition/linkage state.

`unsigned` applies only to direct char/short/int/long spellings, not aliases, bool, floats, or aggregates. Repeated/invalid storage/type qualifiers are parser diagnostics. Parser recovery may create an error-marked AST placeholder; C must stop after parser errors and skip error-marked nodes.

## Derived types, signatures, and arrays (A09–A12/A20/A21)

Declarator construction applies pointer prefixes, reversed array suffixes, function suffixes, then the parenthesized inner declarator. Functions cannot return arrays/functions, and arrays cannot contain void/functions. Array element completeness is checked even below pointers; an incomplete aggregate pointer itself is legal. Prototypes may mention incomplete by-value aggregate parameters/results; definitions require complete storage types. Function bodies attached to object declarators are invalid.

Function overloading uses adjusted parameter types and the variadic flag. Return types do not distinguish overloads. Array/function parameters adjust to pointers; top-level parameter const does not affect signatures, while pointee const and inner array bounds remain significant. Empty parameter lists mean exactly zero parameters. B owns default arguments, call ranking, and variadic promotions. Instance methods may additionally overload on receiver constness; static/instance methods with otherwise identical signatures conflict.

Only positive, integral constant array bounds are supported. Runtime bounds and variably modified types are not represented in the type model. Bounds are either Constant or Unspecified; a nonconstant bound is rejected before an array type is constructed. B must distinguish nonconstant/floating/out-of-range bounds from valid integral constants before returning `IntegerResult::Value`; `ConstantState::Nonconstant` records a nonconstant expression without creating a runtime-bound type.

An omitted bound is retained as Unspecified. It is legal in array typedefs, parameter syntax before adjustment, and initializers where B infers/completes the bound. B calls `Symbols::completeObject` after insertion and before final storage validation. Uncompleted storage objects are errors. Flexible array members, zero-length arrays, bit-fields, and designated initialization are disabled. Parser acceptance of `[]` does not enable flexible array members: `Declarations::member` rejects their incomplete storage type. C must stop on semantic diagnostics.

Existing exclusions continue: no reference declarators, unnamed parameters, signed keyword, const-pointer spelling, multiple inheritance, access modifiers, namespaces/scope resolution, or enum class. Long-long specifier sequences and the special `(void)` parameter spelling are not added here; use long and `()` in this subset. New/delete remain B's explicitly unsupported syntax unless B deliberately expands the policy.

## Aggregates, lifetime, and layout (A13–A19/A23)

Aggregate states are Forward, Defining, Complete. Failed definitions stay Defining and cannot masquerade as complete types. Anonymous aggregates get distinct nominal IDs. Recursive by-value containment and incomplete by-value members are invalid; self pointers are valid. All source members and special member declarations survive in order. Field/alias/method name collisions and duplicate signatures are rejected. Derived declarations hide base overload sets.

Single public, nonvirtual inheritance requires a complete struct/class base. Cycles, union bases, and multiple bases are invalid. Base layout starts at offset zero; no vtables, virtual dispatch, empty-base optimization, bit-field packing, or tail-padding reuse. Fields are placed in source order with alignment padding; union instance fields overlap at zero; size is rounded to maximum alignment. Empty aggregates occupy one byte. Static members, methods, aliases, constructors, and destructors occupy no instance storage. Addition/multiplication/rounding overflow produces errors. Layout never evaluates an expression. Layout queries return Ready, Incomplete, or Error.

Constructors overload on parameter signatures; destructors have no parameters and cannot be variadic. Constructors/destructors are never static or const methods; a constructor cannot take its own type by value. Without user constructors, default construction is implicit. This reference-free subset uses generated aggregate copy operations, recorded by `implicitCopyConstructor`, rather than C++ reference-form copy constructor syntax. These flags describe generated declarations; B must check their usability/deletion against base/member construction and initialization requirements before accepting a construction or copy. Implicit destruction exists when no destructor is declared; nontrivial construction/destruction propagates from bases and array/aggregate members. Nontrivial union instance members are rejected. Active-union-member management is unsupported.

Construction traverses base first and then fields in declaration order; destruction reverses that sequence. `Types::subobjects` exposes this order. `Symbols::objectsLeaving` exposes automatic objects/parameters in reverse declaration order for ancestor-scope exits; it excludes static/global objects. C combines this with B's initialization records to determine cleanup and illegal lifetime entry. Union subobject lists describe alternatives, not an instruction to initialize/destroy every alternative; B/C must choose the active member.

Member lookup supports dot/arrow, inherited lookup/hiding, const receiver propagation to instance fields, and static member access. `this` requires an instance context and points to the const-qualified receiver in a const method. Overload results are candidate indices; B selects a callable candidate and enforces const receiver call viability. Type names/special members cannot be used as ordinary field values.

## Enums and library symbols (A17/A22)

Enums are nominal, unscoped, signed 32-bit types. Enumerators enter the surrounding ordinary scope after their initializer is evaluated. Values default to zero then increment the previous value; duplicate *values* are allowed, duplicate names are not. Explicit initializers use B's required integral-constant evaluator in the live declaration-time environment. Unavailable evaluation, overflow, and a failed preceding implicit enumerator never produce fabricated valid values.

`Symbols::installTypes` installs supported builtin type spellings. `installLibrary` explicitly installs only `int printf(const char*, ...)` and `int scanf(const char*, ...)`, with builtin symbol identities and protected signatures. B uses those identities for optional format checking. File APIs/FILE are not guessed: additional library declarations require an agreed signature and opaque type policy. Install these once in the global scope before program traversal.
```

## `semantic_audit/TASK_SPLIT.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

```markdown
# Three parallel workstreams

Each person owns **24 work packages**. These group all **254 source checkboxes** from the main and extended checklists, including repeated/refined findings; each source checkbox has one primary owner in [TASK_OWNERSHIP.json](TASK_OWNERSHIP.json). Equal package counts are a planning balance, not a claim of identical hours. Optional features stay conditional on the chosen language policy.

| Person | Workstream | Packages | Primary output |
| --- | --- | ---: | --- |
| A | Types, names, declarations, and struct/class/union metadata | 24 | Resolved symbols/types, signatures, member tables, layout/lifetime queries |
| B | Expressions, calls, constants, and initialization | 24 | Typed expressions, explicit conversions, evaluated constants, initialization records |
| C | Parser/AST integration, statements, and control flow | 24 | Correct statement AST, semantic stage, resolved jump targets, integrated diagnostics |

Start by agreeing interface declarations and the policy questions already listed in the audit. Publish small usable interfaces early; nobody should wait for another person's full implementation. Each owner implements the regression checks for their own rules.

**Interfaces to agree first:**

- A provides `TypeId`, `SymbolId`, `ScopeId`, type constructors/queries, lookup/declaration services, member/special-member metadata, and layout queries.
- B provides expression checking with an explicit expected-type/evaluation-context parameter, conversion checking, typed constant evaluation, and initializer checking. Its result carries type, value category, resolved symbol, conversions, and optional constant value.
- C provides source spans, diagnostic reporting, statement/program traversal, current-function/class context, and loop/switch/label target IDs. These use A's IDs rather than an independent symbol/type system.
- A calls B's evaluator for dimensions/enumerators when needed; B calls A's lookup/layout services. Use forward-declared service interfaces and explicit deferred queries for incomplete layout, so this is not a mandatory whole-pass ordering or mutually owning data structures.
- C opens/closes A's scopes and supplies context to B. Function predeclaration must not accidentally make later local declarations visible early. Enum/bound evaluation must run with the correct declaration-time environment.
- Agree supported feature/version flags, error-result conventions, source-order rules, and whether typed data lives in AST fields or side tables. Unsupported service stubs should produce an explicit unavailable/error result, never a fabricated valid type or constant.

**File ownership to avoid merge conflicts:**

- A owns new type/symbol/declaration/layout modules, `src/base/symbol_table/`, and declaration/type/typedef/struct/class/union/enum parser files.
- B owns new expression/conversion/constant/initializer/call modules and expression/initializer/sizeof/function-parameter parsing in `expressions.cc`, `initializer.cc`, `sizeof.cc`, and `function.cc`.
- C owns new statement/control-flow/driver modules, loop/selection/label/jump/compound-statement parsing, driver/build wiring, and the shared audit runner.
- C coordinates the initial AST accessor/source-span patch. After that, A owns declaration/type/aggregate/enum AST headers (including `declaration.hh`), B owns expression/constant/cast/initializer/function AST headers, and C owns base/program/statement/control-flow AST headers. B requests parameter-list additions in A's `declaration.hh`; C requests source-location hooks in files owned by A/B. One owner lands each shared-header change.
- Cross-stream edits are small coordinated patches through the owning person. C integrates the pipeline; A/B remain responsible for their service behavior and fixes.


## Person A — types, names, and declarations

- [ ] **A01: Type and storage policy.** Record target widths, ranks, alignment, type identity, and the declaration/storage policy; publish assumptions used by the other streams.

- [ ] **A02: Semantic type model.** Implement builtin, pointer, array, function, qualified, enum, and struct/class/union types, plus an error type.

- [ ] **A03: Symbols and lexical scopes.** Implement stable symbol/scope IDs, current-scope versus outward lookup, namespaces, and legal shadowing without unstable hash-entry pointers.

- [ ] **A04: Name binding and declaration points.** Bind value references in source order, including declaration lists, self-initialization, and array-bound declaration-point cases.

- [ ] **A05: Parameter scopes.** Register parameters in the proper scope; reject duplicates and forbidden redeclarations in the outer function body.

- [ ] **A06: Redeclarations and linkage.** Track declarations versus definitions, reject conflicting kinds and repeated definitions, and allow compatible declarations without requiring external definitions locally.

- [ ] **A07: Typedef resolution.** Expand aliases with qualification and stable identity; reject incompatible replacements and preserve completion state.

- [ ] **A08: Type modifiers and specifiers.** Validate unsigned/type-kind/storage specifiers consistently; replace invalid-input assertions with diagnostics.

- [ ] **A09: Nested declarators.** Construct correct pointer/array/function binding, classify function versus object definitions, and reject illegal derived types.

- [ ] **A10: Completeness rules.** Distinguish prototypes from definitions and uses requiring storage; check array element completeness even below pointers.

- [ ] **A11: Function signature compatibility.** Normalize array/function parameters and top-level qualifiers, retain inner bounds/pointee const, and define overload identity and empty-list policy.

- [ ] **A12: Variably modified type restrictions.** Define VLA support and enforce structural scope, storage, linkage, member, and repeated-typedef restrictions; distinguish static pointers from static VLA objects.

- [ ] **A13: Aggregate identity and completion.** Track forward, being-defined, and complete states; reject duplicate definitions and recursive by-value containment, including anonymous type identities.

- [ ] **A14: Member tables and source order.** Register fields, methods, aliases, and special members with duplicate checks; preserve declaration order and provide ordered metadata.

- [ ] **A15: Inheritance.** Validate bases, completeness and cycles; provide inherited lookup, hiding rules, and base-subobject relationships.

- [ ] **A16: Layout service.** Compute size, alignment, padding, union size, base offsets, and field offsets; diagnose overflow and expose queries without evaluating expressions.

- [ ] **A17: Enums.** Retain explicit enumerator initializer syntax, register constants, detect name conflicts, and use B's constant evaluator for values and overflow checks.

- [ ] **A18: Constructors and destructors.** Preserve all declarations; validate signature sets/duplicates and determine generated special members and construction/destruction order.

- [ ] **A19: Static members and unions.** Retain union names; distinguish static/instance storage, union layout metadata, and supported nontrivial union member rules.

- [ ] **A20: Type/declaration parser repairs.** Fix type-state crashes, omitted bounds, and supported type spellings/parameter syntax in owned declaration/type parsers.

- [ ] **A21: Optional aggregate syntax.** Decide support for flexible arrays, bit-fields, and designated initialization; implement structural syntax/type restrictions when enabled, with B handling initializer interpretation.

- [ ] **A22: Library declarations.** Provide supported builtin/library symbols and prevent incompatible redeclarations; expose builtin identities to call checking.

- [ ] **A23: Member and implicit-object lookup.** Provide member lookup for dot/arrow, inherited members, this, receiver qualification, and static-context access; B supplies receiver types.

- [ ] **A24: Types/declarations regression checks.** Turn owned audit cases into diagnostic and valid-control checks, including typedefs, scopes, signatures, incomplete types, enums, inheritance, and crash regressions.



## Person B — expressions, calls, and initialization

- [ ] **B01: Conversion and value policy.** Record arithmetic/boolean/enum/pointer conversion rules and expression value categories, agreeing initialization and cast behavior with A.

- [ ] **B02: Typed expression output.** Implement expression result records, implicit conversion nodes/metadata, error propagation, and constant-result references for downstream checking.

- [ ] **B03: Literals and strings.** Type all literals, preserve numeric widths and decoded string lengths, and implement the chosen string constness rules.

- [ ] **B04: Value conversions and unevaluated contexts.** Apply lvalue reads and array/function decay in the correct contexts; type-check unevaluated operands without ordinary execution.

- [ ] **B05: Arithmetic, logical, and bitwise operators.** Check/promote arithmetic operands, integral-only operators, logical conditions, and short-circuit result types.

- [ ] **B06: Pointer arithmetic.** Validate pointer/integer arithmetic, complete object pointees, pointer differences, and result types.

- [ ] **B07: Comparisons, pointer conversions, and null.** Separate equality from ordering; check qualification, function/object/void pointers, eligible null constants, and enum conversion uses.

- [ ] **B08: Addressing and indexing.** Check address-of, dereference, and subscripting with correct element type and value category.

- [ ] **B09: Assignment and modifiability.** Check writable targets and destination conversions, recursive const fields, string/array restrictions, and valid aggregate copies containing arrays.

- [ ] **B10: Compound assignment and increment.** Check compound operators and prefix/postfix mutation, preserve single left-side evaluation, and compute correct result categories.

- [ ] **B11: Conditional and comma expressions.** Type conditional/comma results and preserve call nesting; fix parsing of an unparenthesized comma in the middle conditional operand.

- [ ] **B12: Casts.** Resolve cast target types through A, validate conversions and categories, and reject illegal array/function/unsupported aggregate casts.

- [ ] **B13: Sizeof expression semantics.** Use A's layout service; suppress decay for ordinary operands and represent evaluated VLA bounds/runtime sizes when supported.

- [ ] **B14: Constant expression evaluator.** Evaluate target-typed integral/address constants, distinguish valid constant expressions from foldability, and diagnose required-context failures with short-circuit handling.

- [ ] **B15: Array bounds and VLA evaluation.** Type/evaluate dimensions, enforce chosen numeric-bound rules, preserve runtime declaration-time extents, and check size overflow with A's structural type rules.

- [ ] **B16: Scalar initialization.** Preserve brace shape, validate scalar/const initialization and narrowing, and reject function initializers.

- [ ] **B17: Aggregate and string initialization.** Check nested shape, counts, member order, brace elision, string terminators, omitted elements/bounds, and any enabled designated initialization.

- [ ] **B18: Global/static initialization.** Validate constant/dynamic initialization policy and produce initialization records, including static zero initialization and runtime guards when supported.

- [ ] **B19: Direct/indirect calls and overload selection.** Check callable targets, argument boundaries, arity and conversions; rank supported overloads using A's signatures and distinguish constructor candidates.

- [ ] **B20: Defaults and variadics.** Retain variadic syntax; check default scope/types/redeclarations, apply trailing-default rules and variadic promotions, and expose minimum arity.

- [ ] **B21: Object expressions and construction calls.** Use A's lookup/special-member metadata to type member receivers, this, const member calls, object copying, and constructor arguments.

- [ ] **B22: Optional allocation.** Implement new/delete expression syntax and type checks only if chosen, or diagnose/document unsupported allocation; use A's layout and destruction metadata.

- [ ] **B23: Builtin call checks.** Apply ordinary signatures to library calls and add chosen literal printf/scanf format checks using builtin identity.

- [ ] **B24: Expressions/initialization regression checks.** Turn owned cases into checks for operators, values, calls/defaults, constants, sizeof, arrays/VLAs, initializers, and valid conversions.



## Person C — parsing, statements, and control flow

- [ ] **C01: AST access contract.** Publish visitor/getter and annotation access; coordinate header changes, preserving source-order and nested expression/label structure.

- [ ] **C02: Source locations and diagnostics.** Add source spans and semantic diagnostics, including previous-declaration notes and stable reporting interfaces.

- [ ] **C03: Driver and build wiring.** Keep the root alive, add the semantic stage/CLI/build integration, and stop on prior-stage errors.

- [ ] **C04: Traversal and semantic context.** Own statement/program dispatch and context stacks; connect A/B services and collect constants, layout references, initialization records, and resolved targets.

- [ ] **C05: Recovery and error boundaries.** Skip invalid nodes, contain malformed grammar recovery, and prevent cascades or invalid typed output from reaching lowering.

- [ ] **C06: Basic statement grammar.** Parse bare return, standalone empty statements, and empty bodies while preserving the distinction between syntax and semantic legality.

- [ ] **C07: For-header parsing.** Fix double delimiter consumption and shifted clauses; preserve all omitted-clause combinations and all five silently lost label cases.

- [ ] **C08: Loop suffix grammar.** Require separator plus identifier, reject empty/multiple/numeric suffixes, unify the type-name policy, and keep labels out of runtime expressions.

- [ ] **C09: Labeled statement ownership.** Attach goto/case/default labels to their statements; fix unbraced loop bodies, labeled do bodies, and dangling-else cases.

- [ ] **C10: General switch grammar.** Accept a general switch body statement, preserving nested label ownership and recovery.

- [ ] **C11: Function boundaries and returns.** Reset context on every function, enforce nested-definition policy, validate return placement/conversion through B, and apply chosen fall-through/main rules.

- [ ] **C12: Condition checking.** Visit if/else-if/while/do/for conditions and ask B to perform the chosen boolean/scalar conversion.

- [ ] **C13: For scope and empty clauses.** Enter/leave A scopes around for clauses/body, handle absent conditions/updates, and enforce the selected declaration-clause policy.

- [ ] **C14: Ordinary break and continue.** Require the right enclosing loop/switch and resolve each unlabeled target correctly.

- [ ] **C15: Named loop jumps.** Prebind trailing do labels, use an active loop stack, reject inactive/cross-function targets, and record correct outer-loop update/test/exit destinations.

- [ ] **C16: Label namespaces and collisions.** Define variable/type/goto/loop name interactions, duplicate active labels, and sibling reuse without confusing ordinary goto labels and loop suffixes.

- [ ] **C17: Goto resolution.** Collect function-wide labels, allow forward jumps, reject undefined/duplicate targets, and isolate function tables.

- [ ] **C18: Switch contexts.** Check controlling types using B, associate case/default nodes with the right switch, and isolate nested switches.

- [ ] **C19: Case values and defaults.** Use B's typed constant evaluator, compare case values after conversion, and reject duplicates/nonconstants/multiple defaults.

- [ ] **C20: Jump lifetimes and cleanup.** Check VM/initialization entry restrictions and record cleanup on return/break/continue/goto using A's lifetime metadata and B's initialization records.

- [ ] **C21: Chosen warnings and dataflow.** Add agreed missing-return/unreachable/uninitialized/unused/lifetime warnings without treating all runtime UB as mandatory compilation errors.

- [ ] **C22: Language policy integration.** Record supported versions/features and exclusions, choose label-before-declaration/block-end behavior, and coordinate policy decisions crossing module boundaries.

- [ ] **C23: Audit harness and ownership tracking.** Maintain shared runner/fixtures and the source-task ownership map; keep diagnostic failures distinct from crashes and policy-dependent cases.

- [ ] **C24: Pipeline integration checks.** Combine A/B/C services, verify no valid controls regress and no semantic errors reach lowering, and defer execution-only jump tests until code generation exists.



## Suggested integration sequence

1. Agree the shared interfaces and language policies, then publish IDs, AST access/source spans, and service declarations early.
2. Work in parallel: A starts scopes/declarators/signatures, B starts literals/operators/conversions against the type API, and C repairs the parser and implements loop/goto/switch contexts against the expression API.
3. Integrate narrow paths incrementally: one declaration plus assignment, one checked call, then one labeled loop. A/B resolve their layout/constant-query interactions while C expands traversal.
4. Complete aggregate initialization, class lifetimes, and remaining policy-dependent rules; run each owner's checks and the shared suite.
5. Add execution assertions when code generation exists. Parser acceptance alone never closes a semantic or runtime-control-flow task.

Detailed requirements remain in [the main checklist](CHECKLIST.md) and [the extended findings](extended/FINDINGS.md). The ownership JSON records every original source checkbox, line, and assigned package; duplicates are grouped into the same implementation work rather than treated as additional features.
```

## `semantic_audit/TEAMMATE_A.md`

**Historical document; superseded assertions are identified in section 6.2. Current source/interfaces above take precedence.**

```markdown
# Teammate A handoff

Type/name/declaration services, owned AST/parser repairs, and regression checks are in place. The compiler's semantic driver and B's expression/evaluation callbacks still need integration; `--parse` remains parser-only.

- [Handoff: package map, integration sequence, B/C contracts](../../context_A.md)
- [Target/language policy and public service contract](teammate_a/POLICY.md)
- [Verification results](teammate_a/verification.json)
- [Regression runner](teammate_a/run.py)

Implementation: `src/semantic/types.hh`, `symbols.hh`, `declarations.hh`, `ast_declarations.hh`, and `aggregates.hh`.
```

## `semantic_audit/CHECKLIST.md` — historical audit excerpt

```markdown
# Semantic analysis implementation checklist

This checklist is based on the source in this repository and small programs written specifically for this audit. It covers baka's documented C++-like feature set, including custom loop labels. It is not a claim that baka implements all of C or C++. Items marked **policy** require a language rule; the analyzer must implement that rule consistently. Everything unchecked below is work to include, not an assertion that a runtime probe exercised every combination.

The driver currently stops after parsing. No semantic pass is wired in. Successful `--parse` means an AST was produced without a reported parser error; it does not mean the source is a valid program.

## 1. Language rules to settle before implementing checks

- [ ] Specify primitive type widths, ranks, signedness, floating types, alignment, pointer size, and the type returned by `sizeof` for the target.
- [ ] Specify permitted implicit arithmetic, boolean, enum, pointer, `void*`, and inheritance conversions. C and C++ differ here.
- [ ] Specify value categories and the result categories of assignment, comma, conditional, casts, and prefix/postfix increment. C-based tests cannot automatically define these for a C++-like language.
- [ ] Specify whether function and constructor overloading are supported; otherwise diagnose conflicting same-name declarations/signatures.
- [ ] Specify array rules: constant bounds only versus VLAs, zero-length arrays, unspecified bounds, and parameter adjustment.
- [ ] Specify file-scope declaration/definition rules: C tentative definitions versus C++-like definitions, and internal linkage via `static`.
- [ ] Specify constant versus dynamic initialization for globals and local statics, including initialization order and guards if dynamic initialization is supported.
- [ ] Specify const initialization, aggregate initialization, brace narrowing, enum conversion, and string literal type rules.
- [ ] Specify ordinary-name/type-name/tag conflicts and shadowing. The current parser prohibits declarator names matching any visible type name, which already constrains this policy.
- [ ] Specify loop-label visibility, reuse, and duplicate active labels. A named break/continue should target an active enclosing loop.
- [ ] Specify jumps across initialization and object lifetimes; decide diagnostic severity for reaching the end of a non-void function.
- [ ] Specify whether nested function definitions, `new`/`delete`, and special member generation are supported. Reject unsupported constructs deliberately.
- [ ] Specify whether executable entry-point validation is required. Library compilation must not require a `main` definition.

## 2. Semantic pass infrastructure and output

- [ ] Add a semantic stage and CLI stop option to the driver stage enum, argument handling, diagnostics, and build targets.
- [ ] Keep the AST root available after the parsing block; run semantics only after successful lexing/parsing.
- [ ] Provide AST child/operator/value accessors or a visitor interface. Most nodes currently expose only `Print()`.
- [ ] Attach source spans to AST nodes, declarations, identifiers, and operators for diagnostics.
- [ ] Introduce semantic types for builtins, qualified types, pointers, arrays, functions, aggregates, enums, and the null-pointer literal.
- [ ] Assign stable identities to types, symbols, scopes, functions, and control-flow targets. Same spelling does not make two distinct structs/enums equal.
- [ ] Introduce symbols with kind, type, declaring scope, source location, storage duration/linkage, definition state, and member ownership.
- [ ] Add current-scope lookup separately from outward lookup. Same-scope duplicate checks must not prohibit legal shadowing.
```

This is historical audit evidence, not an interface definition or a current test result. Other interface/design agreements are quoted in full above from TASK_SPLIT and POLICY.

## `semantic_audit/RESULTS.md` — historical audit excerpt

```markdown
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
```

This is historical audit evidence, not an interface definition or a current test result. Other interface/design agreements are quoted in full above from TASK_SPLIT and POLICY.

## `semantic_audit/extended/FINDINGS.md` — historical audit excerpt

```markdown
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
```

This is historical audit evidence, not an interface definition or a current test result. Other interface/design agreements are quoted in full above from TASK_SPLIT and POLICY.

## `semantic_audit/extended/RESULTS.md` — historical audit excerpt

```markdown
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
```

This is historical audit evidence, not an interface definition or a current test result. Other interface/design agreements are quoted in full above from TASK_SPLIT and POLICY.

## `tests/chapter_18/invalid_types/extra_credit/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
Most of the tests in this directory exercise type checking of union types. A few test the interaction between support for structure types and earlier extra credit features.

These tests are organized into the following subdirectories:
* `bad_union_member_access/`: `.` and `->` operatiosn with bad member names
* `incompatible_union_types/`: uses of incompatible union types in assignments, function calls, conditional expressions, etc.
* `incomplete_unions/`: uses of incomplete union types where complete types are required
* `invalid_union_lvalues/`: uses of non-lvalues of union type where lvalues are required
* `other_features/`: all tests of extra credit features other than union types
* `scalar_required/`: uses of unions where scalar types (or more specific scalar types, e.g. integers) are required
* `union_initializers/`: invalid initializers for union types
* `union_struct_conflicts/`: using the same tag with both the `struct` and `union` specifiers, in contexts where both uses of the tag should specify the same type.
* `union_tag_resolution/`: errors caused by incorrect tag resolution (typically leading to incompatible types in assignment etc)
* `union_type_declarations/`: invalid definitions, e.g. duplicate member names, members w/ incomplete types
```

## `tests/chapter_18/valid/extra_credit/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
Valid extra credit tests are organized into subdirectories as follows:
* `libraries/`: Library/client pairs to test that handling of unions is ABI-compliant (including memory layout, initialization, and calling conventions)
* `member_access/`: Test cases for accessing union members with `.` and `->` operators
* `other_features/`: Tests of interaction between structures and extra-credit features other than union
* `semantic_analysis/`: Test cases focused on identifier resolution and type checking. We're more concerned with validating that the semantic analysis stage accepts these than we are with validating their behavior.
* `size_and_offset/`: Tests validating that we calculate union sizes and member offsets correctly.
* `union_copy/`: copying entire unions from one location to another (including simple assignment and copying through pointers)
```

## `tests/chapter_18/valid/no_structure_parameters/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
The tests in this subdirectory do not pass structures as arguments or return values, although some do pass pointers to structures.
Tests are organized into the following directories:
* `smoke_tests/`: Basic tests of struct type declarations, compound initializers, and member access.
* `parse_and_lex/`: Tests to make sure the parser and lexer don't reject certain edge cases.
* `semantic_analysis/`: Tests focused on identifier resolution and type checking. The validate that we correctly resolve tags, track when types are complete, and can handle valid uses of incomplete structure types.
* `size_and_offset_calculations/`: Tests validating that we calculate structure sizes and member offsets correctly. These use the `&` and `sizeof` operators to determine sizes and offsets without actually reading or writing any structures or members.
* `scalar_member_access/`: Tests for the `.` and `->` operators. Some of these tests combine operators to access nested scalar members, but none require copying an entire structure to another location.
* `struct_copy/`: Tests that involve copying entire structures, including assigning one structure to another and reading/writing entire structures through `.`, `->`, and `[]` operators.
* `libraries/`: Test programs that include multiple translation units. Tests in `libraries/initializers/` validate that structure initializers initialize the correct values at the correct locations in a structure. Other tests validate that global structure variables can be accessed in all translation units, and that pointers to structures can be passed as parameters and return values.

Recommended debugging order:
1. `smoke_tests/`
2. `size_and_offset_calculations/`
3. `scalar_member_access/`
4. `libraries/initializers/`
5. `parse_and_lex/`
6. `struct_copy/`
7. `semantic_analysis/`
8. Remaining tests in `libraries/`
```

## `tests/chapter_19/constant_folding/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

````markdown
To validate that operations in a test program were constant folded,
the test script looks at the assembly code for each `target_*` function
in the program. Constant-folded `target_*` functions shouldn't contain
arithmetic instructions, conditional jumps or sets, type conversion
instructions, or other computations. They should only contain:
* The function prologue and epilogue
* `mov` and `jmp` instructions
* `xor op, op`, which is equivalent to `mov $0, op`

If a compiled `target_*` function includes any other instructions, the test fails.
The test script doesn't look at the assembly for other functions (e.g. `main`).

We avoid constant folding in `main`, where we compare the results of `target_*`
functions to their expected values, so the expected value we're comparing to
will be correct even if constant folding is buggy. For example if the result
of `target_neg()` should be `-3`, we might have:

```
int three = 3;

int main(void) {
    if (target_neg() != -three) {
        return 1;
    }
    // etc.
}
```

Because we can't infer the value of `three` in main, the expression `-three`
won't be constant folded.
````

## `tests/chapter_19/copy_propagation/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
To validate that copies were propagated in a test program, the test script inspects the assembly for the `target` function.

In some test programs, the copy propagation and constant folding passes make it possible to evaluate the return value at compile time. The test script expects these programs to include a `mov` instruction that copies the expected constant into EAX, followed by the function epilogue.

In some programs, copy propagation should replace the arguments to certain function calls with constants. In other programs, copy propagation should propagate the same value two different function arguments. The test script validates these programs by checking which values are copied into the parameter-passing registers before the `call` instruction.

Register coalescing, which we implement in Chapter 20, can make it look like the same value is passed in two different parameter-passing registers, even if that value wasn't propagated to both parameters. The tests are designed to prevent register coalescing in those cases, so they'll still test the intended cases after you complete Chapter 20.

In a few programs, including `redundant_copies.c`, removing a redundant copy makes a whole branch dead, allowing unreachable code elimination to remove that branch. The test script validates that these programs contains no control-flow instructions.

In `pointer_arithmetic.c`, the test script validates that we optimize away all computation instructions (e.g. arithmetic instructions like `imul` and type conversions like `movsx`).

The programs in the `dont_propagate` directories cover cases where copies should _not_ be propagated. The test script doesn't inspect the assembly for these; it just validates that they behave correctly.

To see what validation we perform for each test case, see `test_framework/tacky/copy_prop.py`.
```

## `tests/chapter_19/dead_store_elimination/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
To validate that dead stores were eliminated, the test script inspects the assembly for the `target` function.

In most programs, dead store elimination should remove a `Copy` of the form `var = const`, so the test script just validates that that constant doesn't appear in the program. In a few cases (`simple.c`, `delete_arithmetic_ops.c` and `delete_dead_pt_ii_instructions.c`), dead store elimination combined with other optimizations should eliminate the whole function body except the `Return` instruction. In these cases, the function body should contain only the prologue, a `mov` instruction that moves the expected
constant into EAX, and the epilogue.

The test cases in the `dont_elim` directories cover cases where stores _shouldn't_ be eliminated. The test script doesn't inspect the assembly for these; it just validates that they behave correctly.
```

## `tests/chapter_19/unreachable_code_elimination/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
To validate that dead code in a test program was eliminated, the test script inspects the assembly for the `target` function.

In most of our test programs, dead code elimination should eliminate all control-flow instructions and all but one `ret` instruction. These tests fail if `target` contains any jumps, `call` instructions, or labels, or more than one `ret`.

In a couple of programs (`dead_branch_inside_loop.c` and `dead_after_if_else.c`), dead code elimination should eliminate all function calls but not other control-flow instructions.
These tests fail if there's a `call` instruction in the `target` function.

A few programs (`keep_final_jump.c`, `empty.c`, `infinite_loop.c`, and`remove_jump_keep_label.c`) test edge cases where code _shouldn't_ be eliminated, or where a bug in unreachable code elimination is likely to just crash the compiler. The test script doesn't inspect the assembly for these; it just validates that they behave correctly.
```

## `tests/chapter_19/whole_pipeline/README.md`

Book-corpus testing design, not a claim that baka generates assembly or passes these tests.

```markdown
To validate that all optimization passes were run until the results converge, the test script inspects the assembly for any functions starting with `target`.  Many of the tests in this chapter focus on validating constant-folding logic that we couldn't validate before we'd implemented copy propagation (like constant folding with chararacter types and negative numbers). Others validate copy-propagation logic that we couldn't validate before implementing dead storee limination.

In one test program (`alias_analysis_change.c`) we make sure that stores of specific constants were removed, just like we do for some dead store elimination tests.

Some tests of constant folding validate that we remove all arithmetic instructions, conditional instructions, etc., similar to earlier constant-folding tests.

Some tests of copy propagation make sure that stores to a particular global variable were removed (because copy prop made a store to that variable dead) or uses of a particular global variable were removed (because copy prop replaced all uses of that variable).

In the remaining test programs, we validate that each target function is optimized down to a single `Return` instruction, much like we do for other dead store elimination tests.

To see what validateion we perform for each test program, see `test_framework/tacky/pipeline.py`.
```

## `src/lexer/definitions.txt`

Informal historical planning list; keyword vocabulary is not a support guarantee.

```text
Preprocessor
    - #include, #define, NULL


brackets
    - (, ), [, ], {, }
strings shit
    - '', "", escape sequences
semi-colon
Keyword
 - types - int, float, char, bool, void, long, long long - #define to long, double, short, unsigned
 - if, else, for, while, do, switch, case, break, continue, return, goto,
 - union, struct, enum, typedef, class, using, this, new, delete, static, sizeof, const
 - true, false
operators
 - Unary
    - ~, -, --, ++
 - Binary
    - *, /, %, &, |, ^, <<, >>, &&, ||, ==, !=, <, >, <=, >=, +, !
 - Assignment
    - =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=, ^=
 - Colon
    - :, ::
 - Comma
    - ,
 - Question mark
    - ?
    - ., ->, ...
identifiers
literals


space, \n, ,, ., ', ", :, ?, ;, []
```

## `todo`

```text
// TODO:
// error handling
```

# Appendix I — Complete untracked path inventory (non-private) and provenance

- `context_B.md`
- `context_C.md`
- `custom_tests/semantic/cases.json`
- `custom_tests/semantic/cases/address_constant.cpp`
- `custom_tests/semantic/cases/array_assignment.cpp`
- `custom_tests/semantic/cases/bare_return.cpp`
- `custom_tests/semantic/cases/break_outside_loop.cpp`
- `custom_tests/semantic/cases/case_outside_switch.cpp`
- `custom_tests/semantic/cases/conflicting_prototypes.cpp`
- `custom_tests/semantic/cases/conflicting_typedef.cpp`
- `custom_tests/semantic/cases/const_assignment.cpp`
- `custom_tests/semantic/cases/const_member_write.cpp`
- `custom_tests/semantic/cases/const_object_no_initializer.cpp`
- `custom_tests/semantic/cases/const_typedef_write.cpp`
- `custom_tests/semantic/cases/continue_in_switch.cpp`
- `custom_tests/semantic/cases/continue_outer_label.cpp`
- `custom_tests/semantic/cases/continue_outside_loop.cpp`
- `custom_tests/semantic/cases/default_argument_type.cpp`
- `custom_tests/semantic/cases/deref_nonpointer.cpp`
- `custom_tests/semantic/cases/duplicate_case.cpp`
- `custom_tests/semantic/cases/duplicate_constructor.cpp`
- `custom_tests/semantic/cases/duplicate_default.cpp`
- `custom_tests/semantic/cases/duplicate_destructor.cpp`
- `custom_tests/semantic/cases/duplicate_enumerator.cpp`
- `custom_tests/semantic/cases/duplicate_goto_label.cpp`
- `custom_tests/semantic/cases/duplicate_local.cpp`
- `custom_tests/semantic/cases/duplicate_member.cpp`
- `custom_tests/semantic/cases/duplicate_parameter.cpp`
- `custom_tests/semantic/cases/empty_statement.cpp`
- `custom_tests/semantic/cases/explicit_enum_value.cpp`
- `custom_tests/semantic/cases/float_bitwise.cpp`
- `custom_tests/semantic/cases/float_switch.cpp`
- `custom_tests/semantic/cases/floating_array_bound.cpp`
- `custom_tests/semantic/cases/forward_struct_pointer.cpp`
- `custom_tests/semantic/cases/function_redefinition.cpp`
- `custom_tests/semantic/cases/function_returns_array.cpp`
- `custom_tests/semantic/cases/function_variable_conflict.cpp`
- `custom_tests/semantic/cases/goto_bypasses_initialization.cpp`
- `custom_tests/semantic/cases/inactive_loop_label.cpp`
- `custom_tests/semantic/cases/incomplete_object.cpp`
- `custom_tests/semantic/cases/increment_constant.cpp`
- `custom_tests/semantic/cases/invalid_bound_method_address.cpp`
- `custom_tests/semantic/cases/invalid_bound_method_argument.cpp`
- `custom_tests/semantic/cases/invalid_bound_method_pointer.cpp`
- `custom_tests/semantic/cases/invalid_bound_method_value.cpp`
- `custom_tests/semantic/cases/invalid_comma_two_arguments.cpp`
- `custom_tests/semantic/cases/invalid_converted_case_duplicate.cpp`
- `custom_tests/semantic/cases/invalid_default_leak.cpp`
- `custom_tests/semantic/cases/invalid_default_this.cpp`
- `custom_tests/semantic/cases/invalid_drop_const.cpp`
- `custom_tests/semantic/cases/invalid_duplicate_active_loop.cpp`
- `custom_tests/semantic/cases/invalid_empty_inferred_array.cpp`
- `custom_tests/semantic/cases/invalid_float_ice.cpp`
- `custom_tests/semantic/cases/invalid_for_const_no_init.cpp`
- `custom_tests/semantic/cases/invalid_for_initializer.cpp`
- `custom_tests/semantic/cases/invalid_forward_global.cpp`
- `custom_tests/semantic/cases/invalid_function_pointer_field_arity.cpp`
- `custom_tests/semantic/cases/invalid_global_use_before_list_declaration.cpp`
- `custom_tests/semantic/cases/invalid_implicit_method_static_context.cpp`
- `custom_tests/semantic/cases/invalid_lvalue.cpp`
- `custom_tests/semantic/cases/invalid_main_pointer_multiply.cpp`
- `custom_tests/semantic/cases/invalid_main_void_return_value.cpp`
- `custom_tests/semantic/cases/invalid_member_default_leak.cpp`
- `custom_tests/semantic/cases/invalid_pointer_default_leak.cpp`
- `custom_tests/semantic/cases/invalid_postfix_void_pointer.cpp`
- `custom_tests/semantic/cases/invalid_prefix_void_pointer.cpp`
- `custom_tests/semantic/cases/invalid_repeated_default.cpp`
- `custom_tests/semantic/cases/invalid_scalar_array_initializer.cpp`
- `custom_tests/semantic/cases/invalid_short_circuit_type.cpp`
- `custom_tests/semantic/cases/invalid_static_implicit_member.cpp`
- `custom_tests/semantic/cases/invalid_static_initializer.cpp`
- `custom_tests/semantic/cases/invalid_static_local_address.cpp`
- `custom_tests/semantic/cases/invalid_string_int_array_initializer.cpp`
- `custom_tests/semantic/cases/invalid_this.cpp`
- `custom_tests/semantic/cases/invalid_void_pointer_arithmetic.cpp`
- `custom_tests/semantic/cases/missing_member.cpp`
- `custom_tests/semantic/cases/negative_array_bound.cpp`
- `custom_tests/semantic/cases/nested_function.cpp`
- `custom_tests/semantic/cases/noncallable.cpp`
- `custom_tests/semantic/cases/nonconstant_array_bound.cpp`
- `custom_tests/semantic/cases/nonconstant_case.cpp`
- `custom_tests/semantic/cases/object_with_body.cpp`
- `custom_tests/semantic/cases/out_of_scope.cpp`
- `custom_tests/semantic/cases/oversized_initializer.cpp`
- `custom_tests/semantic/cases/pointer_multiplication.cpp`
- `custom_tests/semantic/cases/recursive_member.cpp`
- `custom_tests/semantic/cases/sizeof_function.cpp`
- `custom_tests/semantic/cases/sizeof_void.cpp`
- `custom_tests/semantic/cases/static_method_this.cpp`
- `custom_tests/semantic/cases/undeclared_variable.cpp`
- `custom_tests/semantic/cases/undefined_goto.cpp`
- `custom_tests/semantic/cases/undefined_loop_label.cpp`
- `custom_tests/semantic/cases/unsigned_alias.cpp`
- `custom_tests/semantic/cases/unsigned_float.cpp`
- `custom_tests/semantic/cases/unspecified_array_bound.cpp`
- `custom_tests/semantic/cases/use_before_declaration.cpp`
- `custom_tests/semantic/cases/valid_add_const.cpp`
- `custom_tests/semantic/cases/valid_array.cpp`
- `custom_tests/semantic/cases/valid_bound_instance_method.cpp`
- `custom_tests/semantic/cases/valid_braced_char_array.cpp`
- `custom_tests/semantic/cases/valid_comma_one_argument.cpp`
- `custom_tests/semantic/cases/valid_const_bound.cpp`
- `custom_tests/semantic/cases/valid_constructor_overload.cpp`
- `custom_tests/semantic/cases/valid_default_argument.cpp`
- `custom_tests/semantic/cases/valid_derived_pointer.cpp`
- `custom_tests/semantic/cases/valid_empty_for.cpp`
- `custom_tests/semantic/cases/valid_float_cast_bound.cpp`
- `custom_tests/semantic/cases/valid_forward_function.cpp`
- `custom_tests/semantic/cases/valid_forward_goto.cpp`
- `custom_tests/semantic/cases/valid_frozen_global_binding.cpp`
- `custom_tests/semantic/cases/valid_function_pointer.cpp`
- `custom_tests/semantic/cases/valid_global_array_address.cpp`
- `custom_tests/semantic/cases/valid_global_const_bound.cpp`
- `custom_tests/semantic/cases/valid_global_float.cpp`
- `custom_tests/semantic/cases/valid_global_float_arithmetic.cpp`
- `custom_tests/semantic/cases/valid_global_function_address.cpp`
- `custom_tests/semantic/cases/valid_global_inferred_size_dependency.cpp`
- `custom_tests/semantic/cases/valid_incomplete_pointer_size.cpp`
- `custom_tests/semantic/cases/valid_inferred_multidimensional.cpp`
- `custom_tests/semantic/cases/valid_inferred_size_dependency.cpp`
- `custom_tests/semantic/cases/valid_inheritance.cpp`
- `custom_tests/semantic/cases/valid_instance_this.cpp`
- `custom_tests/semantic/cases/valid_labeled_loop.cpp`
- `custom_tests/semantic/cases/valid_literal_size.cpp`
- `custom_tests/semantic/cases/valid_loop_reuse.cpp`
- `custom_tests/semantic/cases/valid_main_arguments.cpp`
- `custom_tests/semantic/cases/valid_main_long.cpp`
- `custom_tests/semantic/cases/valid_main_single_float.cpp`
- `custom_tests/semantic/cases/valid_main_single_int_braced_initializer.cpp`
- `custom_tests/semantic/cases/valid_main_single_int_variadic.cpp`
- `custom_tests/semantic/cases/valid_main_void.cpp`
- `custom_tests/semantic/cases/valid_member.cpp`
- `custom_tests/semantic/cases/valid_member_default.cpp`
- `custom_tests/semantic/cases/valid_negative_float_static.cpp`
- `custom_tests/semantic/cases/valid_prototype_default.cpp`
- `custom_tests/semantic/cases/valid_recursion.cpp`
- `custom_tests/semantic/cases/valid_scalar.cpp`
- `custom_tests/semantic/cases/valid_scalar_braces.cpp`
- `custom_tests/semantic/cases/valid_self_pointer.cpp`
- `custom_tests/semantic/cases/valid_separate_defaults.cpp`
- `custom_tests/semantic/cases/valid_shadow.cpp`
- `custom_tests/semantic/cases/valid_short_circuit_constant.cpp`
- `custom_tests/semantic/cases/valid_static_explicit_receiver.cpp`
- `custom_tests/semantic/cases/valid_static_global_address.cpp`
- `custom_tests/semantic/cases/valid_static_method_pointer.cpp`
- `custom_tests/semantic/cases/valid_typedef_repeat.cpp`
- `custom_tests/semantic/cases/valid_zero_argument.cpp`
- `custom_tests/semantic/cases/variadic.cpp`
- `custom_tests/semantic/cases/void_array.cpp`
- `custom_tests/semantic/cases/void_condition.cpp`
- `custom_tests/semantic/cases/void_object.cpp`
- `custom_tests/semantic/cases/void_return_value.cpp`
- `custom_tests/semantic/cases/wrong_argument_count.cpp`
- `custom_tests/semantic/cases/wrong_argument_type.cpp`
- `custom_tests/semantic/cases/wrong_arrow.cpp`
- `custom_tests/semantic/cases/wrong_default_arity.cpp`
- `custom_tests/semantic/cases/wrong_enum_modifier.cpp`
- `custom_tests/semantic/cases/wrong_pointer_return.cpp`
- `custom_tests/semantic/conversion_tests.cc`
- `custom_tests/semantic/diagnostics.json`
- `custom_tests/semantic/diagnostics/bad_call_after_redeclaration.cpp`
- `custom_tests/semantic/diagnostics/bad_call_aggregate.cpp`
- `custom_tests/semantic/diagnostics/break_outside.cpp`
- `custom_tests/semantic/diagnostics/continue_outside.cpp`
- `custom_tests/semantic/diagnostics/duplicate_case.cpp`
- `custom_tests/semantic/diagnostics/duplicate_default.cpp`
- `custom_tests/semantic/diagnostics/duplicate_do_label.cpp`
- `custom_tests/semantic/diagnostics/duplicate_for_label.cpp`
- `custom_tests/semantic/diagnostics/duplicate_function.cpp`
- `custom_tests/semantic/diagnostics/duplicate_goto_label.cpp`
- `custom_tests/semantic/diagnostics/duplicate_local.cpp`
- `custom_tests/semantic/diagnostics/duplicate_member.cpp`
- `custom_tests/semantic/diagnostics/duplicate_while_label.cpp`
- `custom_tests/semantic/diagnostics/excess_array_initializer.cpp`
- `custom_tests/semantic/diagnostics/floating_case.cpp`
- `custom_tests/semantic/diagnostics/floating_case_split.cpp`
- `custom_tests/semantic/diagnostics/floating_switch.cpp`
- `custom_tests/semantic/diagnostics/independent_across_functions.cpp`
- `custom_tests/semantic/diagnostics/independent_after_unresolved_name.cpp`
- `custom_tests/semantic/diagnostics/invalid_default.cpp`
- `custom_tests/semantic/diagnostics/invalid_dereference.cpp`
- `custom_tests/semantic/diagnostics/invalid_scalar_initializer.cpp`
- `custom_tests/semantic/diagnostics/missing_member.cpp`
- `custom_tests/semantic/diagnostics/missing_return_value.cpp`
- `custom_tests/semantic/diagnostics/non_callable.cpp`
- `custom_tests/semantic/diagnostics/pointer_multiply.cpp`
- `custom_tests/semantic/diagnostics/pointer_return.cpp`
- `custom_tests/semantic/diagnostics/sizeof_void.cpp`
- `custom_tests/semantic/diagnostics/undefined_goto.cpp`
- `custom_tests/semantic/diagnostics/void_condition.cpp`
- `custom_tests/semantic/diagnostics/void_object.cpp`
- `custom_tests/semantic/diagnostics/void_return_value.cpp`
- `custom_tests/semantic/member_call_tests.cc`
- `custom_tests/semantic/pass_tests.cc`
- `custom_tests/semantic/run.py`
- `semantic_audit/CHECKLIST.md`
- `semantic_audit/RESULTS.md`
- `semantic_audit/TASK_OWNERSHIP.json`
- `semantic_audit/TASK_SPLIT.md`
- `semantic_audit/TEAMMATE_A.md`
- `semantic_audit/extended/FINDINGS.md`
- `semantic_audit/extended/RESULTS.md`
- `semantic_audit/extended/compare_c.py`
- `semantic_audit/extended/comparison.json`
- `semantic_audit/extended/manifest.json`
- `semantic_audit/extended/programs/aggregate_condition.cpp`
- `semantic_audit/extended/programs/aggregate_function_member.cpp`
- `semantic_audit/extended/programs/array_size_self_scope.cpp`
- `semantic_audit/extended/programs/assign_aggregate_with_array_member.cpp`
- `semantic_audit/extended/programs/bitfield_member.cpp`
- `semantic_audit/extended/programs/break_goto_label_only.cpp`
- `semantic_audit/extended/programs/break_outer_while_from_switch.cpp`
- `semantic_audit/extended/programs/case_comma_not_ice.cpp`
- `semantic_audit/extended/programs/case_unevaluated_call.cpp`
- `semantic_audit/extended/programs/conditional_void_pointer_qualification.cpp`
- `semantic_audit/extended/programs/const_field_whole_assignment.cpp`
- `semantic_audit/extended/programs/const_local_not_c_ice.cpp`
- `semantic_audit/extended/programs/continue_outer_do.cpp`
- `semantic_audit/extended/programs/continue_outer_for_from_switch.cpp`
- `semantic_audit/extended/programs/dangling_else_after_label.cpp`
- `semantic_audit/extended/programs/definition_incomplete_parameter.cpp`
- `semantic_audit/extended/programs/designated_initializer.cpp`
- `semantic_audit/extended/programs/do_empty_label.cpp`
- `semantic_audit/extended/programs/do_label_after_body.cpp`
- `semantic_audit/extended/programs/do_label_after_labeled_body.cpp`
- `semantic_audit/extended/programs/do_missing_separator.cpp`
- `semantic_audit/extended/programs/do_typedef_label.cpp`
- `semantic_audit/extended/programs/duplicate_enum_values_valid.cpp`
- `semantic_audit/extended/programs/empty_aggregate_initializer.cpp`
- `semantic_audit/extended/programs/flexible_array_member.cpp`
- `semantic_audit/extended/programs/for_body_shadow_initializer.cpp`
- `semantic_audit/extended/programs/for_body_typedef_leak.cpp`
- `semantic_audit/extended/programs/for_declaration_empty_empty.cpp`
- `semantic_audit/extended/programs/for_declaration_empty_empty_label.cpp`
- `semantic_audit/extended/programs/for_declaration_empty_present.cpp`
- `semantic_audit/extended/programs/for_declaration_empty_present_label.cpp`
- `semantic_audit/extended/programs/for_declaration_present_empty.cpp`
- `semantic_audit/extended/programs/for_declaration_present_empty_label.cpp`
- `semantic_audit/extended/programs/for_declaration_present_present.cpp`
- `semantic_audit/extended/programs/for_declaration_present_present_label.cpp`
- `semantic_audit/extended/programs/for_empty_empty_empty.cpp`
- `semantic_audit/extended/programs/for_empty_empty_empty_label.cpp`
- `semantic_audit/extended/programs/for_empty_empty_present.cpp`
- `semantic_audit/extended/programs/for_empty_empty_present_label.cpp`
- `semantic_audit/extended/programs/for_empty_label.cpp`
- `semantic_audit/extended/programs/for_empty_present_empty.cpp`
- `semantic_audit/extended/programs/for_empty_present_empty_label.cpp`
- `semantic_audit/extended/programs/for_empty_present_present.cpp`
- `semantic_audit/extended/programs/for_empty_present_present_label.cpp`
- `semantic_audit/extended/programs/for_expression_empty_empty.cpp`
- `semantic_audit/extended/programs/for_expression_empty_empty_label.cpp`
- `semantic_audit/extended/programs/for_expression_empty_present.cpp`
- `semantic_audit/extended/programs/for_expression_empty_present_label.cpp`
- `semantic_audit/extended/programs/for_expression_present_empty.cpp`
- `semantic_audit/extended/programs/for_expression_present_empty_label.cpp`
- `semantic_audit/extended/programs/for_expression_present_present.cpp`
- `semantic_audit/extended/programs/for_expression_present_present_label.cpp`
- `semantic_audit/extended/programs/for_function_declaration.cpp`
- `semantic_audit/extended/programs/for_static_object.cpp`
- `semantic_audit/extended/programs/for_typedef_declaration.cpp`
- `semantic_audit/extended/programs/for_typedef_label.cpp`
- `semantic_audit/extended/programs/function_empty_list_argument.cpp`
- `semantic_audit/extended/programs/function_initializer.cpp`
- `semantic_audit/extended/programs/function_pointer_relational.cpp`
- `semantic_audit/extended/programs/goto_into_vla_scope.cpp`
- `semantic_audit/extended/programs/goto_loop_label_only.cpp`
- `semantic_audit/extended/programs/label_at_block_end.cpp`
- `semantic_audit/extended/programs/label_before_declaration.cpp`
- `semantic_audit/extended/programs/label_value_same_name.cpp`
- `semantic_audit/extended/programs/loop_goto_same_name.cpp`
- `semantic_audit/extended/programs/loop_label_duplicate_nested.cpp`
- `semantic_audit/extended/programs/loop_label_exited_sibling.cpp`
- `semantic_audit/extended/programs/loop_label_forward_sibling.cpp`
- `semantic_audit/extended/programs/loop_label_nested_function.cpp`
- `semantic_audit/extended/programs/loop_label_other_function.cpp`
- `semantic_audit/extended/programs/loop_label_reuse_siblings.cpp`
- `semantic_audit/extended/programs/loop_variable_same_name.cpp`
- `semantic_audit/extended/programs/main_bad_parameter.cpp`
- `semantic_audit/extended/programs/main_float_return.cpp`
- `semantic_audit/extended/programs/nested_const_field_assignment.cpp`
- `semantic_audit/extended/programs/parameter_array_pointer_compatibility.cpp`
- `semantic_audit/extended/programs/parameter_inner_bound_conflict.cpp`
- `semantic_audit/extended/programs/parameter_pointee_const_conflict.cpp`
- `semantic_audit/extended/programs/parameter_top_const_compatibility.cpp`
- `semantic_audit/extended/programs/pointer_array_incomplete_element.cpp`
- `semantic_audit/extended/programs/pointer_equality_null.cpp`
- `semantic_audit/extended/programs/pointer_relational_null.cpp`
- `semantic_audit/extended/programs/prototype_incomplete_parameter.cpp`
- `semantic_audit/extended/programs/prototype_incomplete_return.cpp`
- `semantic_audit/extended/programs/short_circuit_typecheck.cpp`
- `semantic_audit/extended/programs/sizeof_typechecks_identifier.cpp`
- `semantic_audit/extended/programs/sizeof_vla_bound_side_effect.cpp`
- `semantic_audit/extended/programs/sizeof_vla_frozen_extent.cpp`
- `semantic_audit/extended/programs/static_pointer_to_vla.cpp`
- `semantic_audit/extended/programs/switch_enters_vla_scope.cpp`
- `semantic_audit/extended/programs/ternary_middle_comma.cpp`
- `semantic_audit/extended/programs/typedef_vla_redefinition.cpp`
- `semantic_audit/extended/programs/unbraced_case_loop_body.cpp`
- `semantic_audit/extended/programs/unbraced_goto_label_loop_body.cpp`
- `semantic_audit/extended/programs/unbraced_switch.cpp`
- `semantic_audit/extended/programs/vla_initializer.cpp`
- `semantic_audit/extended/programs/vla_static_storage.cpp`
- `semantic_audit/extended/programs/vla_struct_member.cpp`
- `semantic_audit/extended/programs/vm_pointer_file_scope.cpp`
- `semantic_audit/extended/programs/void_return_void_expression.cpp`
- `semantic_audit/extended/programs/while_const_condition_no_semicolon.cpp`
- `semantic_audit/extended/programs/while_empty_label.cpp`
- `semantic_audit/extended/programs/while_missing_separator.cpp`
- `semantic_audit/extended/programs/while_multiple_labels.cpp`
- `semantic_audit/extended/programs/while_numeric_label.cpp`
- `semantic_audit/extended/programs/while_typedef_label.cpp`
- `semantic_audit/extended/programs/zero_null_expression.cpp`
- `semantic_audit/extended/programs/zero_variable_not_null_constant.cpp`
- `semantic_audit/extended/results.json`
- `semantic_audit/programs/address_constant.cpp`
- `semantic_audit/programs/array_assignment.cpp`
- `semantic_audit/programs/bad_cast_aggregate.cpp`
- `semantic_audit/programs/bad_cast_array.cpp`
- `semantic_audit/programs/bare_return.cpp`
- `semantic_audit/programs/break_outside_loop.cpp`
- `semantic_audit/programs/call_one_comma_argument.cpp`
- `semantic_audit/programs/call_two_arguments.cpp`
- `semantic_audit/programs/case_outside_switch.cpp`
- `semantic_audit/programs/checked_default_order.cpp`
- `semantic_audit/programs/checked_destructor_name.cpp`
- `semantic_audit/programs/checked_duplicate_const.cpp`
- `semantic_audit/programs/checked_duplicate_static.cpp`
- `semantic_audit/programs/checked_incomplete_base.cpp`
- `semantic_audit/programs/checked_loop_typename.cpp`
- `semantic_audit/programs/checked_struct_redefinition.cpp`
- `semantic_audit/programs/checked_unknown_type.cpp`
- `semantic_audit/programs/comma_call_three.cpp`
- `semantic_audit/programs/comma_call_two.cpp`
- `semantic_audit/programs/conflicting_alias_to_base.cpp`
- `semantic_audit/programs/conflicting_prototypes.cpp`
- `semantic_audit/programs/conflicting_typedef.cpp`
- `semantic_audit/programs/const_assignment.cpp`
- `semantic_audit/programs/const_member_write.cpp`
- `semantic_audit/programs/const_object_no_initializer.cpp`
- `semantic_audit/programs/const_typedef_write.cpp`
- `semantic_audit/programs/continue_in_switch.cpp`
- `semantic_audit/programs/continue_outer_label.cpp`
- `semantic_audit/programs/continue_outside_loop.cpp`
- `semantic_audit/programs/default_argument_type.cpp`
- `semantic_audit/programs/delete_expression.cpp`
- `semantic_audit/programs/deref_nonpointer.cpp`
- `semantic_audit/programs/duplicate_active_label_continue.cpp`
- `semantic_audit/programs/duplicate_active_loop_label.cpp`
- `semantic_audit/programs/duplicate_case.cpp`
- `semantic_audit/programs/duplicate_constructor.cpp`
- `semantic_audit/programs/duplicate_default.cpp`
- `semantic_audit/programs/duplicate_destructor.cpp`
- `semantic_audit/programs/duplicate_enumerator.cpp`
- `semantic_audit/programs/duplicate_function_static.cpp`
- `semantic_audit/programs/duplicate_goto_label.cpp`
- `semantic_audit/programs/duplicate_local.cpp`
- `semantic_audit/programs/duplicate_member.cpp`
- `semantic_audit/programs/duplicate_parameter.cpp`
- `semantic_audit/programs/empty_statement.cpp`
- `semantic_audit/programs/explicit_enum_value.cpp`
- `semantic_audit/programs/float_bitwise.cpp`
- `semantic_audit/programs/float_switch.cpp`
- `semantic_audit/programs/floating_array_bound.cpp`
- `semantic_audit/programs/forward_struct_pointer.cpp`
- `semantic_audit/programs/function_redefinition.cpp`
- `semantic_audit/programs/function_returns_array.cpp`
- `semantic_audit/programs/function_variable_conflict.cpp`
- `semantic_audit/programs/goto_bypasses_initialization.cpp`
- `semantic_audit/programs/inactive_loop_label.cpp`
- `semantic_audit/programs/incomplete_object.cpp`
- `semantic_audit/programs/increment_constant.cpp`
- `semantic_audit/programs/invalid_lvalue.cpp`
- `semantic_audit/programs/invalid_static_initializer.cpp`
- `semantic_audit/programs/invalid_this.cpp`
- `semantic_audit/programs/local_static_initialization.cpp`
- `semantic_audit/programs/long_long_declaration.cpp`
- `semantic_audit/programs/missing_member.cpp`
- `semantic_audit/programs/negative_array_bound.cpp`
- `semantic_audit/programs/nested_function.cpp`
- `semantic_audit/programs/new_expression.cpp`
- `semantic_audit/programs/noncallable.cpp`
- `semantic_audit/programs/nonconstant_array_bound.cpp`
- `semantic_audit/programs/nonconstant_case.cpp`
- `semantic_audit/programs/object_with_body.cpp`
- `semantic_audit/programs/out_of_scope.cpp`
- `semantic_audit/programs/oversized_initializer.cpp`
- `semantic_audit/programs/pointer_multiplication.cpp`
- `semantic_audit/programs/recursive_member.cpp`
- `semantic_audit/programs/sizeof_function.cpp`
- `semantic_audit/programs/sizeof_void.cpp`
- `semantic_audit/programs/static_method_this.cpp`
- `semantic_audit/programs/undeclared_variable.cpp`
- `semantic_audit/programs/undefined_goto.cpp`
- `semantic_audit/programs/undefined_loop_label.cpp`
- `semantic_audit/programs/unnamed_parameter.cpp`
- `semantic_audit/programs/unsigned_alias.cpp`
- `semantic_audit/programs/unsigned_float.cpp`
- `semantic_audit/programs/unspecified_array_bound.cpp`
- `semantic_audit/programs/use_before_declaration.cpp`
- `semantic_audit/programs/valid_array.cpp`
- `semantic_audit/programs/valid_default_argument.cpp`
- `semantic_audit/programs/valid_empty_for.cpp`
- `semantic_audit/programs/valid_forward_goto.cpp`
- `semantic_audit/programs/valid_function_pointer.cpp`
- `semantic_audit/programs/valid_incomplete_pointer_size.cpp`
- `semantic_audit/programs/valid_inheritance.cpp`
- `semantic_audit/programs/valid_labeled_loop.cpp`
- `semantic_audit/programs/valid_member.cpp`
- `semantic_audit/programs/valid_recursion.cpp`
- `semantic_audit/programs/valid_scalar.cpp`
- `semantic_audit/programs/valid_self_pointer.cpp`
- `semantic_audit/programs/valid_shadow.cpp`
- `semantic_audit/programs/valid_typedef_repeat.cpp`
- `semantic_audit/programs/variadic.cpp`
- `semantic_audit/programs/void_array.cpp`
- `semantic_audit/programs/void_condition.cpp`
- `semantic_audit/programs/void_object.cpp`
- `semantic_audit/programs/void_parameter_list.cpp`
- `semantic_audit/programs/void_return_value.cpp`
- `semantic_audit/programs/wrong_argument_count.cpp`
- `semantic_audit/programs/wrong_argument_type.cpp`
- `semantic_audit/programs/wrong_arrow.cpp`
- `semantic_audit/programs/wrong_arrow_on_struct.cpp`
- `semantic_audit/programs/wrong_default_arity.cpp`
- `semantic_audit/programs/wrong_enum_modifier.cpp`
- `semantic_audit/programs/wrong_pointer_return.cpp`
- `semantic_audit/programs/wrong_type_kind.cpp`
- `semantic_audit/results.json`
- `semantic_audit/run.py`
- `semantic_audit/teammate_a/POLICY.md`
- `semantic_audit/teammate_a/ast_test.cc`
- `semantic_audit/teammate_a/programs/invalid_anonymous_destructor.cpp`
- `semantic_audit/teammate_a/programs/invalid_unsigned_function.cpp`
- `semantic_audit/teammate_a/programs/valid_aggregate_shadow.cpp`
- `semantic_audit/teammate_a/programs/valid_anonymous_union.cpp`
- `semantic_audit/teammate_a/programs/valid_type_name_shadow.cpp`
- `semantic_audit/teammate_a/programs/valid_union_forward.cpp`
- `semantic_audit/teammate_a/run.py`
- `semantic_audit/teammate_a/services_test.cc`
- `semantic_audit/teammate_a/verification.json`
- `src/semantic/CMakeLists.txt`
- `src/semantic/IR_READINESS.md`
- `src/semantic/README.md`
- `src/semantic/TODO.md`
- `src/semantic/aggregates.cc`
- `src/semantic/annotation.cc`
- `src/semantic/ast_declarations.cc`
- `src/semantic/checks.cc`
- `src/semantic/conversions.cc`
- `src/semantic/declarations.cc`
- `src/semantic/expressions.cc`
- `src/semantic/expressions.hh`
- `src/semantic/registration.cc`
- `src/semantic/resolution.cc`
- `src/semantic/semantic_analyzer.cc`
- `src/semantic/semantic_analyzer.hh`
- `src/semantic/symbols.cc`
- `src/semantic/types.cc`
- `src/types/parser/ast/implicitCast.hh`

Private/local assistant paths reported by git status remain represented by the collapsed `.codex/` entry in section 1.1; no contents were inspected. The inventory above describes the snapshot before this report existed.

## Verification addendum

Explicit command: `clang-tidy src/semantic/conversions.cc -p /tmp/baka-project-context-z90b9935/build --checks='clang-analyzer-*' --quiet`. It completed with **exit 1**. The Clang frontend rejects `static_cast<ASTNode*>(node)` in `CompoundStatementNode::Print` at `src/types/parser/ast/statement.hh:59`, because aggregate alternatives are incomplete at that point. This prevents a successful explicit-check lint run; the GCC build and tests passed. Captured output follows:

```text
Error while processing /home/saturn/CLionProjects/bakabaka/src/semantic/conversions.cc.
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:59:31: error: static_cast from 'baka::types::ClassDeclarationNode *' to 'ASTNode *', which are not related by inheritance, is not allowed [clang-diagnostic-error]
   59 |                     if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2890:7: note: in instantiation of function template specialization 'baka::types::CompoundStatementNode::Print(size_t)::(lambda)::operator()<baka::types::ClassDeclarationNode>' requested here
 2890 |       std::declval<_Fn>()(std::declval<_Args>()...)
      |       ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2901:29: note: while substituting deduced template arguments into function template '_S_test' [with _Fn = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = (no value)]
 2901 |       using type = decltype(_S_test<_Functor, _ArgTypes...>(0));
      |                             ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2907:14: note: in instantiation of template class 'std::__result_of_impl<false, false, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::ClassDeclarationNode *const &>' requested here
 2907 |     : public __result_of_impl<
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/bits/invoke.h:91:24: note: in instantiation of template class 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::ClassDeclarationNode *const &>' requested here
   91 |     constexpr typename __invoke_result<_Callable, _Args...>::type
      |                        ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1071:11: note: while substituting deduced template arguments into function template '__invoke' [with _Callable = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = <baka::types::ClassDeclarationNode *const &>]
 1071 |           return std::__invoke(std::forward<_Visitor>(__visitor),
      |                  ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1926:5: note: in instantiation of member function 'std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<void> (*)((lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28) &&, const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &)>, std::integer_sequence<unsigned long, 7>>::__visit_invoke' requested here
 1926 |                   _GLIBCXX_VISIT_CASE(7)
      |                   ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1911:4: note: expanded from macro '_GLIBCXX_VISIT_CASE'
 1911 |           __visit_invoke(std::forward<_Visitor>(__visitor),             \
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1982:18: note: in instantiation of function template specialization 'std::__do_visit<std::__detail::__variant::__deduce_visit_result<void>, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
 1982 |             return std::__do_visit<_Tag>(
      |                         ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:17:11: note: 'ClassDeclarationNode' is incomplete
   17 |     class ClassDeclarationNode;
      |           ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:59:31: error: static_cast from 'baka::types::ClassDefinitionNode *' to 'ASTNode *', which are not related by inheritance, is not allowed [clang-diagnostic-error]
   59 |                     if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2890:7: note: in instantiation of function template specialization 'baka::types::CompoundStatementNode::Print(size_t)::(lambda)::operator()<baka::types::ClassDefinitionNode>' requested here
 2890 |       std::declval<_Fn>()(std::declval<_Args>()...)
      |       ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2901:29: note: while substituting deduced template arguments into function template '_S_test' [with _Fn = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = (no value)]
 2901 |       using type = decltype(_S_test<_Functor, _ArgTypes...>(0));
      |                             ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2907:14: note: in instantiation of template class 'std::__result_of_impl<false, false, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::ClassDefinitionNode *const &>' requested here
 2907 |     : public __result_of_impl<
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/bits/invoke.h:91:24: note: in instantiation of template class 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::ClassDefinitionNode *const &>' requested here
   91 |     constexpr typename __invoke_result<_Callable, _Args...>::type
      |                        ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1071:11: note: while substituting deduced template arguments into function template '__invoke' [with _Callable = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = <baka::types::ClassDefinitionNode *const &>]
 1071 |           return std::__invoke(std::forward<_Visitor>(__visitor),
      |                  ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1925:5: note: in instantiation of member function 'std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<void> (*)((lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28) &&, const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &)>, std::integer_sequence<unsigned long, 6>>::__visit_invoke' requested here
 1925 |                   _GLIBCXX_VISIT_CASE(6)
      |                   ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1911:4: note: expanded from macro '_GLIBCXX_VISIT_CASE'
 1911 |           __visit_invoke(std::forward<_Visitor>(__visitor),             \
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1982:18: note: in instantiation of function template specialization 'std::__do_visit<std::__detail::__variant::__deduce_visit_result<void>, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
 1982 |             return std::__do_visit<_Tag>(
      |                         ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:16:11: note: 'ClassDefinitionNode' is incomplete
   16 |     class ClassDefinitionNode;
      |           ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:59:31: error: static_cast from 'baka::types::StructDeclarationNode *' to 'ASTNode *', which are not related by inheritance, is not allowed [clang-diagnostic-error]
   59 |                     if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2890:7: note: in instantiation of function template specialization 'baka::types::CompoundStatementNode::Print(size_t)::(lambda)::operator()<baka::types::StructDeclarationNode>' requested here
 2890 |       std::declval<_Fn>()(std::declval<_Args>()...)
      |       ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2901:29: note: while substituting deduced template arguments into function template '_S_test' [with _Fn = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = (no value)]
 2901 |       using type = decltype(_S_test<_Functor, _ArgTypes...>(0));
      |                             ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2907:14: note: in instantiation of template class 'std::__result_of_impl<false, false, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDeclarationNode *const &>' requested here
 2907 |     : public __result_of_impl<
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/bits/invoke.h:91:24: note: in instantiation of template class 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDeclarationNode *const &>' requested here
   91 |     constexpr typename __invoke_result<_Callable, _Args...>::type
      |                        ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1071:11: note: while substituting deduced template arguments into function template '__invoke' [with _Callable = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = <baka::types::StructDeclarationNode *const &>]
 1071 |           return std::__invoke(std::forward<_Visitor>(__visitor),
      |                  ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1924:5: note: in instantiation of member function 'std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<void> (*)((lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28) &&, const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &)>, std::integer_sequence<unsigned long, 5>>::__visit_invoke' requested here
 1924 |                   _GLIBCXX_VISIT_CASE(5)
      |                   ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1911:4: note: expanded from macro '_GLIBCXX_VISIT_CASE'
 1911 |           __visit_invoke(std::forward<_Visitor>(__visitor),             \
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1982:18: note: in instantiation of function template specialization 'std::__do_visit<std::__detail::__variant::__deduce_visit_result<void>, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
 1982 |             return std::__do_visit<_Tag>(
      |                         ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:15:11: note: 'StructDeclarationNode' is incomplete
   15 |     class StructDeclarationNode;
      |           ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:59:31: error: static_cast from 'baka::types::StructDefinitionNode *' to 'ASTNode *', which are not related by inheritance, is not allowed [clang-diagnostic-error]
   59 |                     if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2890:7: note: in instantiation of function template specialization 'baka::types::CompoundStatementNode::Print(size_t)::(lambda)::operator()<baka::types::StructDefinitionNode>' requested here
 2890 |       std::declval<_Fn>()(std::declval<_Args>()...)
      |       ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2901:29: note: while substituting deduced template arguments into function template '_S_test' [with _Fn = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = (no value)]
 2901 |       using type = decltype(_S_test<_Functor, _ArgTypes...>(0));
      |                             ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2907:14: note: in instantiation of template class 'std::__result_of_impl<false, false, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDefinitionNode *const &>' requested here
 2907 |     : public __result_of_impl<
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:3402:14: note: in instantiation of template class 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDefinitionNode *const &>' requested here
 3402 |     : public __invoke_result<_Functor, _ArgTypes...>
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:3413:5: note: in instantiation of template class 'std::invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDefinitionNode *const &>' requested here
 3413 |     using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;
      |     ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1146:2: note: in instantiation of template type alias 'invoke_result_t' requested here
 1146 |         invoke_result_t<_Visitor, __get_t<_Idxs, _Variant>>...
      |         ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1972:6: note: in instantiation of function template specialization 'std::__detail::__variant::__check_visitor_results<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &, 0UL, 1UL, 2UL, 3UL, 4UL, 5UL, 6UL, 7UL, 8UL, 9UL>' requested here
 1972 |             __check_visitor_results<_Visitor, _Vp>(
      |             ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:14:11: note: 'StructDefinitionNode' is incomplete
   14 |     class StructDefinitionNode;
      |           ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:59:31: error: static_cast from 'baka::types::UnionNode *' to 'ASTNode *', which are not related by inheritance, is not allowed [clang-diagnostic-error]
   59 |                     if (node) static_cast<ASTNode*>(node)->Print(Tabs + 1);
      |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2890:7: note: in instantiation of function template specialization 'baka::types::CompoundStatementNode::Print(size_t)::(lambda)::operator()<baka::types::UnionNode>' requested here
 2890 |       std::declval<_Fn>()(std::declval<_Args>()...)
      |       ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2901:29: note: while substituting deduced template arguments into function template '_S_test' [with _Fn = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = (no value)]
 2901 |       using type = decltype(_S_test<_Functor, _ArgTypes...>(0));
      |                             ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:2907:14: note: in instantiation of template class 'std::__result_of_impl<false, false, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::UnionNode *const &>' requested here
 2907 |     : public __result_of_impl<
      |              ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/bits/invoke.h:91:24: note: in instantiation of template class 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::UnionNode *const &>' requested here
   91 |     constexpr typename __invoke_result<_Callable, _Args...>::type
      |                        ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1071:11: note: while substituting deduced template arguments into function template '__invoke' [with _Callable = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = <baka::types::UnionNode *const &>]
 1071 |           return std::__invoke(std::forward<_Visitor>(__visitor),
      |                  ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1927:5: note: in instantiation of member function 'std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<void> (*)((lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28) &&, const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &)>, std::integer_sequence<unsigned long, 8>>::__visit_invoke' requested here
 1927 |                   _GLIBCXX_VISIT_CASE(8)
      |                   ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1911:4: note: expanded from macro '_GLIBCXX_VISIT_CASE'
 1911 |           __visit_invoke(std::forward<_Visitor>(__visitor),             \
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1982:18: note: in instantiation of function template specialization 'std::__do_visit<std::__detail::__variant::__deduce_visit_result<void>, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
 1982 |             return std::__do_visit<_Tag>(
      |                         ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:18:11: note: 'UnionNode' is incomplete
   18 |     class UnionNode;
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/type_traits:3413:5: error: no type named 'type' in 'std::invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDefinitionNode *const &>' [clang-diagnostic-error]
 3413 |     using invoke_result_t = typename invoke_result<_Fn, _Args...>::type;
      |     ^~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1146:2: note: in instantiation of template type alias 'invoke_result_t' requested here
 1146 |         invoke_result_t<_Visitor, __get_t<_Idxs, _Variant>>...
      |         ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1972:6: note: in instantiation of function template specialization 'std::__detail::__variant::__check_visitor_results<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &, 0UL, 1UL, 2UL, 3UL, 4UL, 5UL, 6UL, 7UL, 8UL, 9UL>' requested here
 1972 |             __check_visitor_results<_Visitor, _Vp>(
      |             ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1071:11: error: no matching function for call to '__invoke' [clang-diagnostic-error]
 1071 |           return std::__invoke(std::forward<_Visitor>(__visitor),
      |                  ^~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1923:5: note: in instantiation of member function 'std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<void> (*)((lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28) &&, const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &)>, std::integer_sequence<unsigned long, 4>>::__visit_invoke' requested here
 1923 |                   _GLIBCXX_VISIT_CASE(4)
      |                   ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1911:4: note: expanded from macro '_GLIBCXX_VISIT_CASE'
 1911 |           __visit_invoke(std::forward<_Visitor>(__visitor),             \
      |           ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1982:18: note: in instantiation of function template specialization 'std::__do_visit<std::__detail::__variant::__deduce_visit_result<void>, (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
 1982 |             return std::__do_visit<_Tag>(
      |                         ^
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/bits/invoke.h:92:5: note: candidate template ignored: substitution failure [with _Callable = (lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), _Args = <baka::types::StructDefinitionNode *const &>]: no type named 'type' in 'std::__invoke_result<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), baka::types::StructDefinitionNode *const &>'
   91 |     constexpr typename __invoke_result<_Callable, _Args...>::type
      |                                                              ~~~~
   92 |     __invoke(_Callable&& __fn, _Args&&... __args)
      |     ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1971:44: error: constexpr variable '__visit_rettypes_match' must be initialized by a constant expression [clang-diagnostic-error]
 1971 |           constexpr bool __visit_rettypes_match = __detail::__variant::
      |                                                   ^~~~~~~~~~~~~~~~~~~~~
 1972 |             __check_visitor_results<_Visitor, _Vp>(
      |             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 1973 |               make_index_sequence<variant_size_v<remove_reference_t<_Vp>>>());
      |               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:22: note: in instantiation of function template specialization 'std::visit<(lambda at /home/saturn/CLionProjects/bakabaka/src/types/parser/ast/statement.hh:58:28), const std::variant<baka::types::StatementNode *, baka::types::DeclarationList *, baka::types::TypedefNode *, baka::types::FunctionNode *, baka::types::StructDefinitionNode *, baka::types::StructDeclarationNode *, baka::types::ClassDefinitionNode *, baka::types::ClassDeclarationNode *, baka::types::UnionNode *, baka::types::EnumNode *> &>' requested here
   58 |                 std::visit([&](auto* node) {
      |                      ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1974:18: error: constexpr if condition is not a constant expression [clang-diagnostic-error]
 1974 |           if constexpr (!__visit_rettypes_match)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1974:19: note: initializer of '__visit_rettypes_match' is not a constant expression
 1974 |           if constexpr (!__visit_rettypes_match)
      |                          ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1971:19: note: declared here
 1971 |           constexpr bool __visit_rettypes_match = __detail::__variant::
      |                          ^
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1976:22: error: static assertion expression is not an integral constant expression [clang-diagnostic-error]
 1976 |               static_assert(__visit_rettypes_match,
      |                             ^~~~~~~~~~~~~~~~~~~~~~
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1976:22: note: initializer of '__visit_rettypes_match' is not a constant expression
/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/16/../../../../include/c++/16/variant:1971:19: note: declared here
 1971 |           constexpr bool __visit_rettypes_match = __detail::__variant::
      |                          ^
```

The standard build/custom/CTest/legacy verification results in section 1.4 are fresh. Formatter failures and lint-tool limitations are reported rather than repaired, to respect the read-only scope. No backend code was implemented and no language/AST/diagnostic behavior was changed.
