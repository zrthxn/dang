COMPILER = clang
CFLAGS = -g

.PHONY: all build compiler bootstrap

all: build compiler
	
build:
	$(COMPILER) $(CFLAGS) dang.c -o dang

compiler: build
	./dang dang.dang

bootstrap: build
	@cp dang boot/
