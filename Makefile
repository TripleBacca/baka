
test: build
	./test_hook

build:
	cd build && cmake ..
	cd build && cmake


.PHONY: build test
