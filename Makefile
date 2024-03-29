COMPILER = clang
CFLAGS = -g -O0

.PHONY: all build compiler bootstrap

all: build compiler
	
build:
	$(COMPILER) $(CFLAGS) dang.c -o dang

compiler: build
	./dang dang.dang -asm

bootstrap: build
	@cp dang boot
	@cp dang.asm boot
