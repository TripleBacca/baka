
test: build
	./test_hook
	./custom_test_hook

build:
	mkdir -p build
	cmake -S src -B build
	cmake --build build --parallel


clean:
	rm -rf build/*
