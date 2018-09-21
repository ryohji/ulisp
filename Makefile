CFLAGS=-O2

ulisp: src/main.o src/data.o src/text.o src/eval.o src/read.o
	$(CC) -o $@ $^

all: ulisp

src/main.o: src/ulisp.h src/main.c
src/data.o: src/ulisp.h src/data.c
src/text.o: src/ulisp.h src/text.c
src/eval.o: src/ulisp.h src/eval.c
src/read.o: src/ulisp.h src/read.c

.PHONY: clean outdir
clean:
	$(RM) -r ulisp src/*.o src/*~
