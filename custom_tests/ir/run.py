#!/usr/bin/env python3
"""TAC golden tests assert status as well as text; diagnostics cannot bless crashes."""
import argparse
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, default=ROOT / "build" / "baka")
    parser.add_argument("cases", nargs="*", help="optional fixture stems")
    args = parser.parse_args()
    failures = 0
    tests = sorted(p for p in HERE.glob("*.cpp") if p.name != "pass_tests.cc")
    known = {source.stem for source in tests} | {"verifier", "cfg", "contracts", "optimization"}
    if set(args.cases) - known:
        parser.error("unknown IR scenarios: " + ", ".join(sorted(set(args.cases) - known)))
    for source in tests:
        if args.cases and source.stem not in args.cases:
            continue
        result = subprocess.run([str(args.compiler.resolve()), "--ir", str(source)],
                                capture_output=True, text=True, timeout=15)
        expected_path = source.with_suffix(".expected")
        if not expected_path.exists():
            print(f"FAIL {source.name}: missing golden file")
            failures += 1
            continue
        expected = expected_path.read_text()
        if source.stem.startswith("invalid_"):
            passed = result.returncode == 1 and not result.stderr and "(IR)" in result.stdout and "IR::TAC:" not in result.stdout
            passed = passed and all(line in result.stdout for line in expected.splitlines() if line)
        else:
            passed = result.returncode == 0 and result.stdout == expected and not result.stderr
        if not passed:
            import difflib
            print(f"FAIL {source.name}: status={result.returncode}")
            print("\n".join(difflib.unified_diff(expected.splitlines(), result.stdout.splitlines(),
                                               fromfile="expected", tofile="actual")))
            print(result.stderr)
            failures += 1
        else:
            print(f"PASS {source.name}")
    structural = args.compiler.resolve().parent / "ir_pass_tests"
    scenarios = [source.stem for source in tests if not source.stem.startswith("invalid_")]
    scenarios += ["verifier", "cfg", "contracts", "optimization"]
    for scenario in scenarios:
        if args.cases and scenario not in args.cases:
            continue
        try:
            result = subprocess.run([str(structural), scenario], capture_output=True, text=True, timeout=15)
            if result.returncode:
                print(f"FAIL structural/{scenario}: {result.stdout}{result.stderr}")
                failures += 1
            else:
                print(f"PASS structural/{scenario}")
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"FAIL structural/{scenario}: {error}")
            failures += 1
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
