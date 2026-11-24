#!/usr/bin/env bash
set -euo pipefail

mkdir -p "$SUBMISSION_ROOT_DIR"/{build,src,test,tools}
for path in src/*; do
    case "$path" in
        src/build|src/cmake-build-*) continue ;;
    esac
    cp -r "$path" "$SUBMISSION_ROOT_DIR/src/"
done
cp -r custom_tests/* "$SUBMISSION_ROOT_DIR/test/"
cp -r tools/mips "$SUBMISSION_ROOT_DIR/tools/"
cp custom_test_hook README.md "$SUBMISSION_ROOT_DIR/"

{
    echo "STAGE_ARGS := lex parse semantic ir ir_opt"
    cat Makefile
} > "$SUBMISSION_ROOT_DIR/Makefile"

find "$SUBMISSION_ROOT_DIR/test" -name '*.expected' -exec sed -i 's#custom_tests/#test/#g' {} +

cat > "$SUBMISSION_ROOT_DIR/run.sh" <<'EOF'
#!/usr/bin/env bash
set -e
make test
ctest --test-dir build -R '^peephole$' --output-on-failure --no-tests=error
EOF
chmod +x "$SUBMISSION_ROOT_DIR/run.sh"

# Build and test the staged files, then clear the compiled artifacts.
trap 'find "$SUBMISSION_ROOT_DIR/build" -mindepth 1 -delete' EXIT
(cd "$SUBMISSION_ROOT_DIR" && ./run.sh)
