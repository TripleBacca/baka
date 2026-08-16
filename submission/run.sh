#!/usr/bin/env bash
set -euo pipefail

EXECUTABLE="${1:-./build/baka}"
TEST_DIR="${2:-test}"

if [[ ! -x "$EXECUTABLE" ]]; then
    echo "error: executable not found: $EXECUTABLE" >&2
    echo "usage: $0 [executable] [test_dir]" >&2
    exit 1
fi

if [[ ! -d "$TEST_DIR" ]]; then
    echo "error: test directory not found: $TEST_DIR" >&2
    echo "usage: $0 [executable] [test_dir]" >&2
    exit 1
fi

for src in "$TEST_DIR"/*; do
    [[ -f "$src" ]] || continue

    echo "=== $(basename "$src") ==="
    if "$EXECUTABLE" "$src"; then
        echo "exit-code: 0"
    else
        echo "exit-code: $?"
    fi
    echo
done
