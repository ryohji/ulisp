#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

extern bool freadable(FILE* fp);

int main() {
    jmp_buf trap;
    const struct sexp* env = cons(cons(symbol("t"), symbol("True")), NIL());
    struct env_exp r = { .env = env, };

    switch (setjmp(trap)) {
    case TRAP_NONE:
        while (true) {
            char* p;
            if (!freadable(stdin)) {
                printf("> ");
                fflush(stdout);
            }
            r = eval(trap, (struct env_exp){ r.env, read(trap) });
            p = text(r.exp);
            puts(p);
            free(p);
        }
        break;
    case TRAP_NOINPUT:
        return 0;
    }
}
