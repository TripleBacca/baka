#!/usr/bin/env python3
"""Check each optimizer fixture's `--ir --optimize` output and optimized/unoptimized execution."""
import argparse
import difflib
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def run(command):
    return subprocess.run([str(arg) for arg in command], capture_output=True, text=True, timeout=15)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, default=ROOT / "build/baka")
    parser.add_argument("--update", action="store_true", help="rewrite .expected files from the compiler")
    parser.add_argument("cases", nargs="*", help="optional fixture stems")
    args = parser.parse_args()
    cases = json.loads((HERE / "cases.json").read_text())
    known = {Path(name).stem for name in cases}
    if set(args.cases) - known:
        parser.error("unknown IR optimization cases: " + ", ".join(sorted(set(args.cases) - known)))
    executor = args.compiler.resolve().parent / "ir_execution_tests"
    failures = 0
    for name, case in cases.items():
        source = HERE / name
        if args.cases and source.stem not in args.cases:
            continue
        try:
            result = run([args.compiler, "--ir", "--optimize", source])
            expected_file = source.with_suffix(".expected")
            if args.update and result.returncode == 0:
                expected_file.write_text(result.stdout)
            expected = expected_file.read_text()
            passed = result.returncode == 0 and not result.stderr and result.stdout == expected
            if not passed:
                print("\n".join(difflib.unified_diff(expected.splitlines(), result.stdout.splitlines(),
                                                   fromfile="expected", tofile="actual")))
                print(result.stderr)
            else:
                # The executor runs the program unoptimized and optimized and requires equal results.
                for input_value, expected_value in case["runs"]:
                    result = run([executor, source, input_value, expected_value])
                    if result.returncode:
                        passed = False
                        print(result.stdout + result.stderr)
                        break
            print(("PASS " if passed else "FAIL ") + name)
            failures += not passed
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"FAIL {name}: {error}")
            failures += 1
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
