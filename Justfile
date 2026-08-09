
test: build
	./test_hook

build:
	cd build && cmake ..
	cd build && make
