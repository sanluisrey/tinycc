CFLAGS=-std=c11 -g -static

tinycc: tinycc.c

test: tinycc
			./test.sh

clean:
			rm -f tinycc *.o *~ tmp*
.PHONY: test clean
