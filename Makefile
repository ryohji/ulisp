CFLAGS=-O2 -Isrc

ulisp: src/main.o src/data.o src/text.o src/eval.o src/read.o src/freadable.o
	$(CC) -o $@ $^

all: ulisp

test: test/data test/text test/read test/eval
	test/data
	test/text
	test/read
	test/eval

src/main.o: src/ulisp.h src/main.c
src/data.o: src/ulisp.h src/data.c
src/text.o: src/ulisp.h src/text.c
src/eval.o: src/ulisp.h src/eval.c
src/read.o: src/ulisp.h src/read.c

test/data.o: src/ulisp.h src/data.c test/data.c
test/text.o: src/ulisp.h src/text.c src/data.c src/text.c
test/eval.o: src/ulisp.h src/eval.c src/data.c src/text.c src/eval.c
test/read.o: src/ulisp.h src/read.c src/data.c src/text.c src/read.c

.PHONY: clean test
clean:
	$(RM) -r ulisp src/*.o src/*~ test/{data,text,read,eval}
