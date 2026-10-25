#!/usr/bin/env bash
set -euo pipefail

mkdir -p $SUBMISSION_ROOT_DIR/src
tar --exclude='cmake-build-*' -C src -cf - . | tar -C "$SUBMISSION_ROOT_DIR/src" -xf -

mkdir -p $SUBMISSION_ROOT_DIR/build
mkdir -p $SUBMISSION_ROOT_DIR/test/lex
cp -r custom_tests/lex/* $SUBMISSION_ROOT_DIR/test/lex
cp custom_test_hook $SUBMISSION_ROOT_DIR/

touch $SUBMISSION_ROOT_DIR/Makefile
echo "STAGE_ARGS := lex" > $SUBMISSION_ROOT_DIR/Makefile
echo "$(cat Makefile)" >> $SUBMISSION_ROOT_DIR/Makefile


# cp Justfile submission/
cp README.md $SUBMISSION_ROOT_DIR/

find $SUBMISSION_ROOT_DIR/test -name '*.expected' -exec sed -i 's#custom_tests/#test/#g' {} +

touch $SUBMISSION_ROOT_DIR/run.sh
echo "#!/usr/bin/env bash
./custom_test_hook lex" > $SUBMISSION_ROOT_DIR/run.sh
chmod +x $SUBMISSION_ROOT_DIR/run.sh

# Verify saved expectations without regenerating them.
if make -C "$SUBMISSION_ROOT_DIR" test; then
    rc=0
else
    rc=$?
fi
rm -rf "$SUBMISSION_ROOT_DIR"/build/*
exit $rc
