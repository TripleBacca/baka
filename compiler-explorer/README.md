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

The semantic and parser views start at their AST; the lexer's token table (and the
parser's driver settings) that the compiler prints first is skipped. Add `--verbose`
to the options to see the full output. For AST and IR views, turn off assembly
filters (especially directives, labels and comments) if they hide parts of the
text. Errors appear in Compiler Explorer's compiler output with source locations
and the compiler's failure status.

## Running programs

Execution works from every view: open an Executor (or tick "Execute the code" in the
output pane) and set arguments and stdin there. The output shows the program's stdout,
stderr and exit code. The program is always built the same way, whatever the view:

1. `baka --codegen` (with the pane's optimization flags) produces MIPS64 assembly.
2. `mips64el-linux-gnuabi64-gcc -march=mips64r2 -mabi=64 -static -no-pie` assembles and
   links it, exactly like baka's own `--link` stage.
3. CE runs the static executable under `qemu-mips64el` (`executionWrapper`).

"Compile to binary" shows the linked executable disassembled with the MIPS objdump.

How it is wired:

| Piece | Setting |
| --- | --- |
| `baka-ce` | Without `-S`, CE wants a binary: the adapter links to the `-o` path (`-c` stops at an object). `MIPS_CC` overrides the cross compiler. |
| `config/c.local.properties` | `supportsBinary` and `supportsExecute` on, `objdumper` set to the MIPS objdump, `executionWrapper=/usr/bin/qemu-mips64el`, and `emulated=true` so CE runs the foreign binary locally instead of looking for a remote MIPS host. |
| `config/compiler-explorer.local.properties` | `trustProxy=loopback, uniquelocal`, so behind a reverse proxy or tunnel on a private network (such as cloudflared on the Docker network) CE honours `X-Forwarded-Proto` and short links come back as `https://`. Otherwise the browser refuses the `http://` link (a different origin) and the Short Link dialog never fills in. |
| `Dockerfile` | Installs `gcc-mips64el-linux-gnuabi64`, `libc6-dev-mips64el-cross`, `qemu-user` and `file` (CE checks the executable with `file` before running it). These packages exist for amd64 and arm64. |

## Start or update the local instance

From the repository root:

```sh
make build
docker compose -f compiler-explorer/docker-compose.yml up -d --build --force-recreate
```

Open <http://localhost:10240>, or `http://<machine-address>:10240` from another
machine (the port listens on all interfaces by default), and select C. Baka accepts its supported C++-like
syntax in that editor, without preprocessor directives or system headers.
`CE_BIND` and `CE_CONTAINER` can override the bind address (default `0.0.0.0`; use
`127.0.0.1` to keep it local) and container name.

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
each view with direct compiler output. Binary mode is checked with a stand-in for
the cross compiler, so the host needs no MIPS toolchain. They cover optimization settings and
precedence, paths containing spaces, failure diagnostics, version probing and
missing arguments. They require no Docker image.

After starting CE, test the full adapter/configuration/API path with:

```sh
python3 compiler-explorer/test_api.py --url http://localhost:10240
```

This verifies all four compiler registrations, each view at `-O0` and `-O2`,
semantic failure diagnostics in the semantic, IR and assembly views, execution from
every view (arguments, stdin, stdout and exit code), a compile error reported instead
of running, and the binary disassembly view.

If `docker compose up` fails with `Unable to enable LOOPBACK FILTERING` or
`Extension tcp revision 0 not supported`, the host's iptables cannot publish the port.
Run the image with host networking instead (it then listens on port 10240 directly):

```sh
docker run -d --name baka-godbolt --network host \
    -v "$PWD/build/baka:/opt/baka/bin/baka:ro" \
    -v "$PWD/compiler-explorer/baka-ce:/opt/baka/bin/baka-ce:ro" \
    -v "$PWD/compiler-explorer/config/c.local.properties:/compiler-explorer/etc/config/c.local.properties:ro" \
    -v "$PWD/compiler-explorer/config/compiler-explorer.local.properties:/compiler-explorer/etc/config/compiler-explorer.local.properties:ro" \
    -v baka-ce-links:/compiler-explorer/lib/storage/data \
    baka-compiler-explorer:latest
```

Compiler registrations use CE's documented [compiler groups and options](https://github.com/compiler-explorer/compiler-explorer/blob/main/docs/AddingACompiler.md).
