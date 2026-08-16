help:
	@printf "baka : C++ like language compiler\n"
	@printf "make help \t\t\t show this help file\n"
	@printf "make build \t\t\t build the compiler\n"
	@printf "make test \t\t\t test the compiler\n"
	@printf "make lex file=path/to/file \t run the compiler till the lexer stage\n"

test: build
	./custom_test_hook

submit:
	mkdir -p submissions
	rm -rf submissions/*
	chmod +x gen_submissions.sh
	./gen_submissions.sh

bless: build
	./custom_test_hook bless

build:
	mkdir -p build
	cmake -S src -B build
	cmake --build build

lex:
	./build/baka --lex --verbose $(file)

clean:
	rm -rf build/*


.PHONY: test build lex clean help
