#!/usr/bin/env python3
"""Generate each TAC fixture's IR and compare it with the expected output."""
import argparse
import difflib
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
    parser.add_argument("cases", nargs="*", help="optional fixture stems")
    args = parser.parse_args()
    sources = sorted(HERE.glob("*.cpp"))
    known = {source.stem for source in sources}
    if set(args.cases) - known:
        parser.error("unknown IR cases: " + ", ".join(sorted(set(args.cases) - known)))
    failures = 0
    checked = 0
    for source in sources:
        if args.cases and source.stem not in args.cases:
            continue
        checked += 1
        try:
            result = run([args.compiler, "--ir", source])
            expected = source.with_suffix(".expected").read_text()
            passed = result.returncode == 0 and not result.stderr and result.stdout == expected
            if not passed:
                if result.returncode:
                    print(f"compiler exited with status {result.returncode}")
                print("\n".join(difflib.unified_diff(expected.splitlines(), result.stdout.splitlines(),
                                                   fromfile="expected", tofile="actual")))
                print(result.stderr)
            print(("PASS " if passed else "FAIL ") + source.name)
            failures += not passed
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"FAIL {source.name}: {error}")
            failures += 1
    print(f"{checked} cases; {failures} failures")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
