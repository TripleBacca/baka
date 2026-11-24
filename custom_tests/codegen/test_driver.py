#!/usr/bin/env python3
"""Exercise the object and executable stages with the real MIPS toolchain."""

import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check_mips_elf(path, elf_type):
    data = path.read_bytes()
    assert data[:6] == b"\x7fELF\x02\x01", f"{path}: expected little-endian ELF64"
    assert struct.unpack_from("<HH", data, 16) == (elf_type, 8), f"{path}: wrong ELF type or machine"


def check_tail_recursion(run, compiler, work):
    source = work / "countdown.cpp"
    source.write_text("""int countdown(int n, int total) {
    if (n == 0) return total;
    return countdown(n - 1, total + 1);
}
int main(int argc, char** argv) {
    int n = atoi(argv[1]);
    return countdown(n, 0) == n ? 42 : 1;
}
""")
    qemu = ("docker", "run", "--rm", "--init", "--pull=never", "--ulimit", "core=0:0", "-v",
            f"{work}:/work:ro", "baka-mips64el", "qemu-mips64el", "-s", "262144")
    for mode, name, deep_status in (("--optimize", "tail-opt", 42), ("--no-optimize", "tail-raw", 139)):
        text = run(compiler, "--codegen", mode, source).stdout
        body = re.search(r"^countdown\.(\d+):\n(.*?)^\t\.end\tcountdown\.\1$", text, re.MULTILINE | re.DOTALL)
        assert body, text
        symbol, instructions = body.groups()
        assert ("\tjalr\t" in instructions) == (mode == "--no-optimize"), instructions
        if mode == "--optimize":
            assert f"\tb\t.LB{symbol}.1\n" in instructions, instructions
        assembly = work / f"{name}.s"
        assembly.write_text(text)
        run(compiler, "--link", "-o", work / name, assembly)
        run(*qemu, f"/work/{name}", "17", status=42)
        run(*qemu, f"/work/{name}", "1000000", status=deep_status)


# Tests object and executable generation, launcher behavior, atomic output on
# link failure, and bounded stack usage for direct tail recursion.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", type=Path, default=ROOT / "build/baka")
    compiler = parser.parse_args().compiler.resolve()

    with tempfile.TemporaryDirectory(prefix="baka-driver-") as directory:
        work = Path(directory)

        def run(*command, input_text=None, status=0, env=None):
            result = subprocess.run(command, cwd=work, input=input_text, text=True,
                                    capture_output=True, timeout=30, env=env)
            assert result.returncode == status, (command, result.returncode, result.stdout, result.stderr)
            return result

        source = work / "example source.cpp"
        source.write_text('int main(int argc, char** argv) {\n'
                          '    char ch;\n'
                          '    if ((argc != 2 && argc != 4) || read(0, &ch, 1) != 1) return 1;\n'
                          '    int n = atoi(argv[1]);\n'
                          '    printf("%d %c\\n", n * 2, ch);\n'
                          '    if (argc == 4) printf("<%s> <%s>\\n", argv[2], argv[3]);\n'
                          '    if (argc == 4 && n == 21) {\n'
                          '        printf("%s\\n", argv[0]);\n'
                          '        write(2, "stderr\\n", 7);\n'
                          '    }\n'
                          '    return n == 21 ? 7 : 0;\n'
                          '}\n')
        assembly = work / "example assembly.s"
        assembly.write_text(run(compiler, "--codegen", source).stdout)
        obj = work / "example object.o"
        run(compiler, "--assemble", assembly, "-o", obj)
        check_mips_elf(obj, 1)

        run(compiler, "--assemble", source)
        check_mips_elf(work / "example source.o", 1)

        executable = work / "example program"
        run(compiler, "--link", "-o", executable, obj)
        check_mips_elf(executable, 2)

        output = run(ROOT / "tools/mips/run-binary", executable, "21", input_text="x", status=7)
        assert output.stdout == "42 x\n", output.stdout
        output = run("make", "-s", "-C", ROOT, "run-mips-binary", f"file={executable}", "n=5", input_text="y")
        assert output.stdout == "10 y\n", output.stdout
        output = run("make", "-s", "-C", ROOT, "run-mips-binary", f"file={executable}",
                     'args=5 "two words" --flag', input_text="z")
        assert output.stdout == "10 z\n<two words> <--flag>\n", output.stdout

        # Reuse the compiler supplied by CTest without rebuilding it from the Make recipe.
        (work / "build").mkdir()
        (work / "build/baka").symlink_to(compiler)
        (work / "tools").symlink_to(ROOT / "tools", target_is_directory=True)
        output = run("make", "-s", "-f", ROOT / "Makefile", "-o", "build", "run", f"file={source}",
                     'args=6 "two words" --flag', input_text="w")
        assert output.stdout == "12 w\n<two words> <--flag>\n", output.stdout

        shim = work / "run example"
        run("make", "-s", "-C", ROOT, "shim", f"out={shim}")
        assert os.access(shim, os.X_OK)
        env = os.environ.copy()
        env.pop("BAKA_BINARY", None)
        missing = run(shim, status=2, env=env)
        assert "set BAKA_BINARY" in missing.stderr, missing.stderr

        shim_binary = work / 'example program, with "quotes"'
        shim_binary.hardlink_to(executable)
        env["BAKA_BINARY"] = shim_binary.name
        output = run(shim, "21", "two words", "--flag", input_text="x", status=7, env=env)
        assert output.stdout == f"42 x\n<two words> <--flag>\n{shim_binary.name}\n", output.stdout
        assert output.stderr == "stderr\n", output.stderr

        fault_source = work / "null.c"
        fault_source.write_text("int main() { int* ptr = 0; return *ptr; }\n")
        fault_binary = work / "null.mips"
        run(compiler, "-o", fault_binary, fault_source)
        fault = run(shim, input_text="", status=139, env={**env, "BAKA_BINARY": str(fault_binary)})
        assert "Segmentation fault" in fault.stderr, fault.stderr

        result = run(compiler, "--no-optimize", source, "-o", "direct")
        assert result.stdout == "", result.stdout
        check_mips_elf(work / "direct", 2)
        run(compiler, source)
        check_mips_elf(work / "a.out", 2)

        # A link failure must leave an existing output and the input untouched.
        original_source = source.read_bytes()
        run(compiler, "-o", source, source, status=1)
        assert source.read_bytes() == original_source
        before = executable.read_bytes()
        source.write_text("int missing(); int main() { return missing(); }\n")
        failure = run(compiler, source, "-o", executable, status=1)
        assert "missing" in failure.stderr and "MIPS toolchain exited" in failure.stderr, failure.stderr
        assert executable.read_bytes() == before
        assert not list(work.glob(".baka-link-*"))

        check_tail_recursion(run, compiler, work)

    print("MIPS driver stages, binary execution and tail recursion passed")


if __name__ == "__main__":
    main()
