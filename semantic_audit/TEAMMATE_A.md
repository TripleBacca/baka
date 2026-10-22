# Teammate A handoff

Type/name/declaration services, owned AST/parser repairs, and regression checks are in place. The compiler's semantic driver and B's expression/evaluation callbacks still need integration; `--parse` remains parser-only.

- [Handoff: package map, integration sequence, B/C contracts](../../context_A.md)
- [Target/language policy and public service contract](teammate_a/POLICY.md)
- [Verification results](teammate_a/verification.json)
- [Regression runner](teammate_a/run.py)

Implementation: `src/semantic/types.hh`, `symbols.hh`, `declarations.hh`, `ast_declarations.hh`, and `aggregates.hh`.
