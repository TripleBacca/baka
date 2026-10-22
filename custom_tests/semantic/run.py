#!/usr/bin/env python3
"""Representative semantic acceptance and diagnostic cases."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", type=Path, default=ROOT / "build/baka")
    args = parser.parse_args()
    cases = json.loads((HERE / "cases.json").read_text())
    failures = 0
    for name, messages in cases.items():
        try:
            result = subprocess.run([str(args.compiler), "--semantic", str(HERE / name)],
                                    capture_output=True, text=True, timeout=10)
            passed = result.returncode == (1 if messages else 0) and not result.stderr
            if messages:
                passed = passed and "(SEMANTIC)" in result.stdout
                passed = passed and all(message in result.stdout for message in messages)
            print(("PASS " if passed else "FAIL ") + name)
            if not passed:
                failures += 1
                print(result.stdout + result.stderr)
        except (OSError, subprocess.TimeoutExpired) as error:
            print(f"FAIL {name}: {error}")
            failures += 1
    print(f"{len(cases)} cases; {failures} failures")
    return bool(failures)


if __name__ == "__main__":
    sys.exit(main())
