#!/usr/bin/env python3
"""Exercise the CE invocation contract against a built baka binary."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
ANSI = re.compile(r"\x1b\[[0-9;]*m")


# Compares invoked stages and optimization flags with direct compiler output,
# then checks the adapter's CLI errors and diagnostics.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, default=ROOT.parent / "build/baka")
    args = parser.parse_args()
    compiler = args.compiler.resolve()
    env = dict(os.environ, BAKA=str(compiler))

    def run(command):
        return subprocess.run(command, env=env, text=True, capture_output=True)

    with tempfile.TemporaryDirectory(prefix="baka-ce-") as work:
        work = Path(work)
        source = work / "source with spaces.cpp"
        source.write_text("int main() { int x = 2 + 3; return x * 4; }\n")
        output = work / "output with spaces.s"
        wrapper = str(ROOT / "baka-ce")
        cases = [([], ["--codegen"])]
        for stage in ("--lex", "--parse", "--semantic", "--ir", "--codegen"):
            direct = [stage] + (["--verbose"] if stage == "--parse" else [])
            cases.append(([stage], direct))
        for stage in ("--ir", "--codegen"):
            for flag, native in (("-O0", "--no-optimize"), ("-O2", "--optimize")):
                cases.append(([stage, flag], [stage, native]))
        cases.extend([
            (["--ir", "-O2", "-O0"], ["--ir", "--no-optimize"]),
            (["--ir", "-O0", "--optimize"], ["--ir", "--optimize"]),
            (["--codegen", "--no-optimize"], ["--codegen", "--no-optimize"]),
        ])
        for flags, native in cases:
            expected = run([str(compiler), *native, str(source)])
            assert expected.returncode == 0, expected.stdout + expected.stderr
            actual = run([wrapper, "-g", "-S", "-x", "c", "-I", str(work),
                          "-D", "IGNORED=1", *flags, "-o", str(output), str(source)])
            assert actual.returncode == 0, actual.stderr
            assert actual.stdout == "" and actual.stderr == "", actual
            assert output.read_text() == ANSI.sub("", expected.stdout + expected.stderr), flags
        # With no -o, the adapter remains useful as a command-line tool.
        actual = run([wrapper, "--ir", str(source)])
        assert actual.returncode == 0 and "function" in actual.stdout, actual
        # Failed semantic analysis must retain the exit status and source location,
        # strip terminal colours, and clear an existing assembly output file.
        source.write_text("int main() { return missing; }\n")
        output.write_text("stale assembly")
        actual = run([wrapper, "-S", "-o", str(output), str(source)])
        assert actual.returncode != 0 and actual.stdout == "", actual
        assert "error:" in actual.stderr and f"{source}:1:" in actual.stderr, actual
        assert "\x1b" not in actual.stderr and output.read_text() == "", actual
        version = run([wrapper, "--version"])
        assert version.returncode == 0 and "MIPS64" in version.stdout, version
        for flags in ([], ["-o"], ["-x"], ["--build_type"], [str(work / "missing.cpp")]):
            actual = run([wrapper, *flags])
            assert actual.returncode == 2 and "baka-ce:" in actual.stderr, actual
    print(f"Passed {len(cases)} stage/optimization cases, stdout, diagnostics, version and argument checks")


if __name__ == "__main__":
    main()
