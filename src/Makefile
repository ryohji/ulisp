CFLAGS=-O2

ulisp: main.o data.o text.o eval.o read.o
	$(CC) -o $@ $^

all: ulisp

main.o: ulisp.h main.c
data.o: ulisp.h data.c
text.o: ulisp.h text.c
eval.o: ulisp.h eval.c
read.o: ulisp.h read.c

.PHONY: clean
clean:
	$(RM) ulisp *.o *~
