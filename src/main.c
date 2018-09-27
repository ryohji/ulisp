#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static void repl(jmp_buf trap, const struct sexp* env_exp);

int main() {
    jmp_buf trap;
    const struct sexp* env = cons(cons(symbol("t"), symbol("True")), NIL());
loop:
    printf("> ");
    switch (setjmp(trap)) {
    case TRAP_NONE:
        fflush(stdout);
        repl(trap, cons(env, read(trap)));
    case TRAP_NOINPUT:
        return 0;
    }
    goto loop;
}

static void repl(jmp_buf trap, const struct sexp* env_exp) {
    const struct sexp* r = eval(trap, env_exp);
    char* p = text(snd(r));
    printf("%s\n> ", p);
    fflush(stdout);
    free(p);
    repl(trap, cons(fst(r), read(trap)));
}
