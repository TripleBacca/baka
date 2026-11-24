# Baka on Compiler Explorer

The compiler dropdown offers four views of the same source:

| Compiler | Output | Default optimization |
| --- | --- | --- |
| Baka (MIPS64 assembly) | MIPS64r2 n64 little-endian GNU assembly | Enabled |
| Baka (typed TAC / IR) | Checked, typed three-address code | Disabled |
| Baka (semantic AST) | AST after name resolution and type checking | Not applicable |
| Baka (parser AST) | Parser AST and verbose frontend output | Not applicable |

Assembly is the default view. Each later stage runs all the earlier stages,
including semantic checks. Add compiler panes with different views to compare
the semantic AST, IR and assembly for one program.

Use `-O0` or `--no-optimize` to disable optimization, and `-O1`, `-O2`, `-O3`
or `--optimize` to enable the compiler's existing block-local IR and MIPS
peephole passes. These optimization levels all enable the same passes; they do
not select different optimization strengths. The last optimization option wins.
The native `--lex`, `--parse`, `--semantic`, `--ir`, `--codegen`, `--verbose`,
and `--build_type debug|release` flags also work in the options box.

For AST and IR views, turn off assembly filters (especially directives, labels
and comments) if they hide parts of the text. Errors appear in Compiler Explorer's
compiler output with source locations and the compiler's failure status. Binary
output and execution are disabled; the existing `tools/mips/run` workflow runs
emitted assembly under QEMU outside this container.

## Start or update the local instance

From the repository root:

```sh
make build
docker compose -f compiler-explorer/docker-compose.yml up -d --build --force-recreate
```

Open <http://localhost:10240> and select C. Baka accepts its supported C++-like
syntax in that editor, without preprocessor directives or system headers.
`CE_BIND` and `CE_CONTAINER` can override the bind address and container name.

The compiler binary, adapter and configuration are mounted from this checkout.
After later compiler builds or configuration changes, recreate the container
so it picks up the new compiler binary and reloads the compiler list:

```sh
docker compose -f compiler-explorer/docker-compose.yml up -d --force-recreate
```

The container's architecture must match `build/baka`. For an ARM64 deployment,
build an ARM64 binary first (the supplied `cross.cmake` configures an
`aarch64-linux-gnu` toolchain).

## Verify the adapter

```sh
bash -n compiler-explorer/baka-ce
python3 compiler-explorer/test_adapter.py --compiler build/baka
```

The checks run the real compiler through GCC-style CE invocations and compare
each view with direct compiler output. They cover optimization settings and
precedence, paths containing spaces, failure diagnostics, version probing and
missing arguments. They require no Docker image.

After starting CE, test the full adapter/configuration/API path with:

```sh
python3 compiler-explorer/test_api.py --url http://localhost:10240
```

This verifies all four compiler registrations, each view at `-O0` and `-O2`,
and semantic failure diagnostics in the semantic, IR and assembly views.

Compiler registrations use CE's documented [compiler groups and options](https://github.com/compiler-explorer/compiler-explorer/blob/main/docs/AddingACompiler.md).
