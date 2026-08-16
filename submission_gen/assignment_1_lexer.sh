#!/usr/bin/env bash
set -euo pipefail

mkdir -p $SUBMISSION_ROOT_DIR/src
cp -r src/* $SUBMISSION_ROOT_DIR/src

mkdir -p $SUBMISSION_ROOT_DIR/build
cp -r custom_tests $SUBMISSION_ROOT_DIR/test
cp custom_test_hook $SUBMISSION_ROOT_DIR/

cp Makefile $SUBMISSION_ROOT_DIR/
# cp Justfile submission/
cp run.sh $SUBMISSION_ROOT_DIR/
cp README.md $SUBMISSION_ROOT_DIR/

find $SUBMISSION_ROOT_DIR/test -name '*.expected' -exec sed -i 's#custom_tests/#test/#g' {} +
