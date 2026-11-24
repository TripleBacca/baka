help:
	@printf "baka : C++ like language compiler\n"
	@printf "make help \t\t\t\t show this help file\n"
	@printf "make build \t\t\t\t build the compiler\n"
	@printf "make test \t\t\t\t test the compiler\n"
	@printf "make clean \t\t\t\t clean up build artifacts\n"
	@printf "make lex file=path/to/file \t\t run the compiler till the lexer stage\n"
	@printf "make parse file=path/to/file \t\t run the compiler till the parser stage\n"
	@printf "make parse-folder dir=path/to/dir \t run parser on all files in dir recursively, output to .out files\n"
	@printf "make codegen file=path/to/file \t\t print MIPS64 assembly\n"
	@printf "make run file=path/to/file args='ARG ...' \t compile and run under qemu (Docker)\n"
	@printf "make run-mips-binary file=path/to/binary args='ARG ...' \t run an existing MIPS executable (Docker)\n"
	@printf "make shim out=path/to/launcher \t create a standalone launcher that reads BAKA_BINARY\n"
	@printf "make test-codegen \t\t\t run the MIPS64 execution suite (Docker)\n"

test: build
	./custom_test_hook $(STAGE_ARGS)

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
mips-image:
	docker build -t baka-mips64el tools/mips

test-codegen: build mips-image
	./custom_test_hook codegen

codegen:
	./build/baka --codegen $(file)

# Treat args as shell words, with n as a shortcut for a single argument.
RUN_ARGS = $(if $(strip $(args)),$(args),$(if $(n),"$(n)"))

run run-mips-binary: private export BAKA_RUN_FILE = $(value file)

run: build
	@work=$$(mktemp -d) && \
		trap 'rm -rf -- "$$work"' EXIT && \
		./build/baka -o "$$work/program" "$$BAKA_RUN_FILE" && \
		tools/mips/run-binary "$$work/program" $(RUN_ARGS)

run-mips-binary:
	@tools/mips/run-binary "$$BAKA_RUN_FILE" $(RUN_ARGS)

out ?= run-baka-mips

shim: private export BAKA_SHIM_OUT = $(value out)
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


.PHONY: test build lex clean help parse parse-folder semantic test-semantic ir test-ir test-ir-opt mips-image test-codegen codegen run run-mips-binary shim
