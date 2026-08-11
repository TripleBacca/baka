test: build
	./test_hook

build:
	cd build && cmake ..
	cd build && make

clean:
	rm -rf build/*

.PHONY: test build clean
