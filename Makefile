COMPILER = clang

.PHONY: all build compiler bootstrap

all: build compiler
	
build:
	$(COMPILER) dang.c -o dang

compiler: build
	./dang dang.dang

bootstrap: build
	@cp dang boot/
