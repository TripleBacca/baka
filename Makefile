help:
	@printf 'baka : C++ like language compiler\n\n'
	@printf '  %-46s  %s\n' \
		'make help' 'show this help file' \
		'make build' 'build the compiler' \
		'make test' 'test the compiler' \
		'make clean' 'clean up build artifacts' \
		'make lex file=path/to/file' 'run the compiler till the lexer stage' \
		'make parse file=path/to/file' 'run the compiler till the parser stage' \
		'make codegen file=path/to/file' 'print MIPS64 assembly' \
		'make assemble file=source.cpp [out=program.o]' 'write a MIPS64 object file (Docker)' \
		'make link file=source.cpp [out=program.mips]' 'write a static MIPS64 executable (Docker)' \
		'make mips-runtime' 'build the Docker toolchain for MIPS64' \
		"make run file=path/to/file args='ARG ...'" 'compile and run under QEMU (Docker)' \
		'make shim out=path/to/launcher' 'create a standalone launcher that reads BAKA_BINARY' \
		'make test-codegen' 'run the MIPS64 execution suite (Docker)'

test: build
	./custom_test_hook $(STAGE_ARGS)

submit:
	./gen_submissions

bless: build
	./custom_test_hook bless

build:
	mkdir -p build
	cmake -S src -B build
	cmake --build build --parallel

lex:
	./build/baka --lex --verbose $(file)

parse:
	./build/baka --parse --verbose $(file)

ir:
	./build/baka --ir $(file)

semantic:
	./build/baka --semantic --verbose $(file)

test-ir: build
	./custom_test_hook ir

test-semantic: build
	./custom_test_hook semantic

test-ir-opt: build
	./custom_test_hook ir_opt

# Needs Docker: builds the mips64el toolchain image on first use.
mips-runtime:
	docker build -t baka-mips64el tools/mips

test-codegen: build mips-runtime
	./custom_test_hook codegen

codegen:
	./build/baka --codegen $(file)

assemble link: private export BAKA_STAGE_FILE = $(value file)
assemble link: private export BAKA_STAGE_OUT = $(value out)
assemble link: build
	@set --; \
		if [ -n "$$BAKA_STAGE_OUT" ]; then set -- -o "$$BAKA_STAGE_OUT"; fi; \
		./build/baka --$@ "$$@" "$$BAKA_STAGE_FILE"

# Treat args as shell words, with n as a shortcut for a single argument.
RUN_ARGS = $(if $(strip $(args)),$(args),$(if $(n),"$(n)"))

run: private export BAKA_RUN_FILE = $(value file)

run: build
	@work=$$(mktemp -d) && \
		trap 'rm -rf -- "$$work"' EXIT && \
		./build/baka -o "$$work/program" "$$BAKA_RUN_FILE" && \
		tools/mips/run-binary "$$work/program" $(RUN_ARGS)

shim: private export BAKA_SHIM_OUT = $(if $(filter undefined,$(origin out)),run-baka-mips,$(value out))
shim:
	@install -m 755 -T -- tools/mips/shim "$$BAKA_SHIM_OUT"

parse-folder: build
	@found=0; failed=0; \
	for f in $$(find $(dir) -type f ! -name '*.out'); do \
		out="$${f}.out"; \
		printf "Parsing %s -> %s\n" "$$f" "$$out"; \
		if ./build/baka --parse --verbose "$$f" > "$$out" 2>&1; then \
			found=$$((found + 1)); \
		else \
			found=$$((found + 1)); failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nDone: %d files parsed, %d with errors\n" "$$found" "$$failed"

clean:
	rm -rf build/*


.PHONY: test submit build lex clean help parse parse-folder semantic test-semantic ir test-ir test-ir-opt mips-runtime test-codegen codegen assemble link run shim
