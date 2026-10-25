#!/usr/bin/env bash
set -euo pipefail

mkdir -p "$SUBMISSION_ROOT_DIR/src"
tar --exclude='cmake-build-*' -C src -cf - . | tar -C "$SUBMISSION_ROOT_DIR/src" -xf -

mkdir -p "$SUBMISSION_ROOT_DIR/build" "$SUBMISSION_ROOT_DIR/test/ir" "$SUBMISSION_ROOT_DIR/test/semantic"
cp -r custom_tests/ir/. "$SUBMISSION_ROOT_DIR/test/ir/"
cp -r custom_tests/semantic/. "$SUBMISSION_ROOT_DIR/test/semantic/"

cp custom_test_hook "$SUBMISSION_ROOT_DIR/"

printf 'STAGE_ARGS := semantic ir\n' > "$SUBMISSION_ROOT_DIR/Makefile"
cat Makefile >> "$SUBMISSION_ROOT_DIR/Makefile"


cp README.md ir_context.md "$SUBMISSION_ROOT_DIR/"

find "$SUBMISSION_ROOT_DIR/test" -name '*.expected' -exec sed -i 's#custom_tests/#test/#g' {} +

cat > "$SUBMISSION_ROOT_DIR/run.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
make test
EOF
chmod +x "$SUBMISSION_ROOT_DIR/run.sh"

# Verify the saved expectations without regenerating them, then remove build artifacts.
if make -C "$SUBMISSION_ROOT_DIR" test; then
    rc=0
else
    rc=$?
fi
rm -rf "$SUBMISSION_ROOT_DIR"/build/*
exit $rc
