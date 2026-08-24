#!/usr/bin/env bash
set -euo pipefail

mkdir -p $SUBMISSION_ROOT_DIR/src
cp -r src/* $SUBMISSION_ROOT_DIR/src

mkdir -p $SUBMISSION_ROOT_DIR/build
cp -r custom_tests $SUBMISSION_ROOT_DIR/test
cp custom_test_hook $SUBMISSION_ROOT_DIR/

touch $SUBMISSION_ROOT_DIR/Makefile
echo "STAGE_ARGS := lex" > $SUBMISSION_ROOT_DIR/Makefile
echo "$(cat Makefile)" >> $SUBMISSION_ROOT_DIR/Makefile


# cp Justfile submission/
cp README.md $SUBMISSION_ROOT_DIR/

find $SUBMISSION_ROOT_DIR/test -name '*.expected' -exec sed -i 's#custom_tests/#test/#g' {} +

touch $SUBMISSION_ROOT_DIR/run.sh
echo "#!/usr/bin/env bash
./custom_test_hook" > $SUBMISSION_ROOT_DIR/run.sh
chmod +x $SUBMISSION_ROOT_DIR/run.sh

# build,bless,test and delete build files
make -C "$SUBMISSION_ROOT_DIR" build
./$SUBMISSION_ROOT_DIR/custom_test_hook bless lex
make -C "$SUBMISSION_ROOT_DIR" test
rc=$?
rm -rf "$SUBMISSION_ROOT_DIR"/build/*
exit $rc
