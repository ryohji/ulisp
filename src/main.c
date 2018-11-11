#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

extern bool freadable(FILE* fp);

int main() {
    jmp_buf trap;
    struct env* env = env_create(NULL);
    env_define(env, "t", symbol("True"));
    struct env_exp r = { .env = env, };

    switch (setjmp(trap)) {
    default:
        fprintf(stderr, "\n");
    case TRAP_NONE:
        while (true) {
            if (!freadable(stdin)) {
                printf("> ");
            }
            fflush(stdout);
            r = eval(trap, (struct env_exp){ r.env, read(trap) });
            write(stdout, r.exp);
            printf("\n");
        }
        break;
    case TRAP_NOINPUT:
        return 0;
    }
}
