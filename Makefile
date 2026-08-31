help:
	@printf "baka : C++ like language compiler\n"
	@printf "make help \t\t\t show this help file\n"
	@printf "make build \t\t\t build the compiler\n"
	@printf "make test \t\t\t test the compiler\n"
	@printf "make clean \t\t\t clean up build artifacts\n"
	@printf "make lex file=path/to/file \t run the compiler till the lexer stage\n"
	@printf "make parse file=path/to/file \t run the compiler till the parser stage\n"
	@printf "make parse-folder dir=path/to/dir \t run parser on all .cpp files in dir recursively, output to .out files\n"

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
	cmake --build build --parallel

lex:
	./build/baka --lex --verbose $(file)

parse:
	./build/baka --parse --verbose $(file)

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


.PHONY: test build lex clean help parse parse-folder
