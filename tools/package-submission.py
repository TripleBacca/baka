#!/usr/bin/env python3
"""Package the final Baka assignment from the working tree."""

import argparse
import io
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile


ROOT = Path(__file__).resolve().parent.parent
PACKAGE = "baka-final"
INCLUDE = ("Makefile", "custom_test_hook", "src", "custom_tests", "tools/mips")
MARKDOWN = {".md", ".markdown", ".mdx", ".mdown", ".mkd"}

GUIDE = """Baka final compiler submission
==============================

Baka compiles its C++-like language to little-endian MIPS64 Linux executables.

Build on Linux with GNU Make, CMake, Python 3, and a C++23 compiler.

    make build
    make help

Inspect the compiler stages without Docker:

    ./build/baka --lex examples/hello.cpp
    ./build/baka --parse --verbose examples/hello.cpp
    ./build/baka --semantic examples/hello.cpp
    ./build/baka --ir examples/hello.cpp
    ./build/baka --ir --optimize examples/hello.cpp
    ./build/baka --codegen examples/hello.cpp

For linking and execution, start Docker and build the toolchain image once:

    make mips-image
    make run file=examples/hello.cpp

The demo prints "Hello from Baka". To keep the MIPS executable:

    ./build/baka -o hello.mips examples/hello.cpp
    tools/mips/run-binary hello.mips

The compiler emits a MIPS executable by default. Use --codegen to print assembly
or --assemble to write an object file. Use --no-optimize to inspect raw assembly.
Pass program arguments with `make run file=program.cpp args='5 "two words"'`.
The direct launcher preserves the program's exit status.

Run the tests without Docker:

    ./custom_test_hook lex parse semantic ir ir_opt
    ctest --test-dir build -R '^peephole$' --output-on-failure --no-tests=error

After building the Docker image, run the native MIPS suites:

    ctest --test-dir build -R '^(codegen|mips_driver|mips_runtime)$' --output-on-failure --no-tests=error

Team
    Satindra Ramalingam (24114087)
    Shaurya Goel (24114090)
    Tanmay Arya (24114099)
    Abhinav Shresth (24115008)
"""

EXAMPLE = """int main() {
    printf("Hello from Baka\\n");
    return 0;
}
"""


def submission_files():
    # Git supplies the file list. Read the working-tree bytes, including local edits.
    result = subprocess.run(["git", "ls-files", "-z", "--", *INCLUDE], cwd=ROOT, check=True,
                            stdout=subprocess.PIPE)
    paths = sorted({Path(os.fsdecode(name)) for name in result.stdout.split(b"\0") if name})
    return [path for path in paths if path.suffix.lower() not in MARKDOWN]


def add_text(archive, name, text):
    data = text.encode("utf-8")
    entry = tarfile.TarInfo(f"{PACKAGE}/{name}")
    entry.size = len(data)
    archive.addfile(entry, io.BytesIO(data))


def package(destination):
    files = submission_files()
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=destination.parent) as temporary:
        staged = Path(temporary) / destination.name
        with tarfile.open(staged, mode="w:gz") as archive:
            for path in files:
                source = ROOT / path
                if not source.is_file() or source.is_symlink():
                    raise ValueError(f"expected a regular tracked file: {path}")
                entry = archive.gettarinfo(source, arcname=f"{PACKAGE}/{path.as_posix()}")
                entry.uid = entry.gid = 0
                entry.uname = entry.gname = ""
                with source.open("rb") as contents:
                    archive.addfile(entry, contents)
            add_text(archive, "README.txt", GUIDE)
            add_text(archive, "examples/hello.cpp", EXAMPLE)
        staged.replace(destination)
    return len(files) + 2


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "submissions" / f"{PACKAGE}.tar.gz",
                        help="destination .tar.gz path (default: submissions/baka-final.tar.gz)")
    args = parser.parse_args()
    if not args.output.name.endswith(".tar.gz"):
        parser.error("the output filename must end in .tar.gz")
    try:
        count = package(args.output)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"package-submission: {error}\n")
    print(f"Created {args.output} ({count} files)")


if __name__ == "__main__":
    main()
