#!/usr/bin/env bash
set -euo pipefail

rm -rf submission

mkdir -p submission
cp -r src submission/

mkdir -p submission/build
cp -r custom_tests/tests submission/test
cp custom_test_hook submission/

cp Makefile submission/
# cp Justfile submission/

find submission/test -name '*.expected' -exec sed -i 's#custom_tests/tests/#test/#g' {} +
make -C submission build
