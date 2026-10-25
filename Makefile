JOBS ?= 4

help:
	@printf "baka : C++ like language compiler\n"
	@printf '%-40s %s\n' \
		'make help' 'show this help' \
		'make build' 'build the compiler' \
		'make test' 'run compiler tests' \
		'make test-semantic' 'run semantic acceptance and rejection tests' \
		'make test-ir' 'generate IR and compare with expected output' \
		'make bless' 'refresh lexer/parser expected output and check semantic/IR tests' \
		'make submit' 'generate assignment submission files' \
		'make clean' 'clean up build artifacts' \
		'make lex file=path/to/file' 'run the compiler through lexing' \
		'make parse file=path/to/file' 'run the compiler through parsing' \
		'make semantic file=path/to/file' 'run semantic analysis and print the annotated AST' \
		'make ir file=path/to/file' 'generate and print typed three-address IR' \
		'make parse-folder dir=path/to/dir' 'parse files recursively and write .out files'

test: build
	./custom_test_hook $(STAGE_ARGS)

submit:
	mkdir -p submissions
	rm -rf submissions/*
	chmod +x gen_submissions
	./gen_submissions

bless: build
	./custom_test_hook bless

build:
	mkdir -p build
	cmake -S src -B build
	cmake --build build --parallel $(JOBS)

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


.PHONY: test build lex clean help parse parse-folder semantic test-semantic ir test-ir bless submit
