CFLAGS=-std=c11 -g -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

tinycc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): tinycc.h

test: tinycc
	./test.sh

clean:
	rm -f tinycc *.o *~ tmp*

.PHONY: test clean
