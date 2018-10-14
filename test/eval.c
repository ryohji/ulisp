#include "ulisp.h"

#include "../src/eval.c"
#include "../src/data.c"
#include "../src/text.c"

#include <stdlib.h>

#define ASSERT_EQ(expect, actual) if (strcmp(expect, actual)) { printf("expect: %s\n""actual: %s\n""@%d\n", expect, actual, __LINE__); ng += 1; } else { ok += 1; }
#define NOT_REACHED_HERE() { printf("NOT REACHED HERE.\n@%d\n", __LINE__); ng += 1; }

static const struct sexp* LIST(unsigned numElems, ...);

int main() {
    unsigned ok = 0, ng = 0;
    jmp_buf trap;
    FILE* const fp = stderr;
    const struct sexp* x; // expression to test.
    struct env_exp r; // result, (env: evaluated) pair.
    const struct sexp* env = cons(cons(symbol("t"), symbol("True")), NIL());
    char* p = 0;
    size_t n;

    /* ATOM */
    r =  eval(trap, (struct env_exp){ NIL(), NIL() });
    ASSERT_EQ("(())", text(cons(r.env, r.exp)));

    r = eval(trap, (struct env_exp){ env, symbol("t") });
    ASSERT_EQ("(((t: True)): True)", text(cons(r.env, r.exp)));

    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), symbol("t") }); /* symbol `t` not defined in env. */
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOSYM:
            ASSERT_EQ("Value for symbol `t` not found.", p);
            break;
    }
    fclose(stderr);
    free(p);

    r = eval(trap, (struct env_exp){ NIL(), cons(symbol("quote"), cons(symbol("ulisp"), NIL())) });
    ASSERT_EQ("((): ulisp)", text(cons(r.env, r.exp)));

    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("quote"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (quote)", p);
            break;
    }
    fclose(stderr);
    free(p);

    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("quote"), symbol("ulisp")) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (quote: ulisp)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (atom nil) ; => True. nil is atom. */
    x = cons(symbol("atom"), cons(NIL(), NIL()));
    r = eval(trap, (struct env_exp){ .env = env, .exp = x });
    ASSERT_EQ("(((t: True)): True)", text(cons(r.env, r.exp)));

    /* (atom (quote ulisp)) ; => True. quote generate symbol. */
    x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(symbol("ulisp"), NIL())), NIL()));
    r = eval(trap, (struct env_exp){ env, x });
    ASSERT_EQ("(((t: True)): True)", text(cons(r.env, r.exp)));

    /* (atom (quote (ulisp))) ; => nil. pair/list is not atom. */
    x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(cons(symbol("ulisp"), NIL()), NIL())), NIL()));
    r = eval(trap, (struct env_exp){ env, x });
    ASSERT_EQ("(((t: True)))", text(cons(r.env, r.exp)));

    /* Error thrown if no argument specified. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("atom"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (atom)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* Error TRAP_NOSYM thrown if global envitonment does not hold specified symbol. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ env, cons(symbol("atom"), cons(symbol("ulisp"), NIL())) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOSYM:
            ASSERT_EQ("Value for symbol `ulisp` not found.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* So, if global environment holds specified symbol matching atom, eval `(atom ulisp)` returns the value of `t`. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ cons(cons(symbol("ulisp"), symbol("ULISP")), env), cons(symbol("atom"), cons(symbol("ulisp"), NIL())) });
            ASSERT_EQ("(((ulisp: ULISP) (t: True)): True)", text(cons(r.env, r.exp)));
            /* And if mapped global symbol holds pair, eval `(atom ulips)` returns nil. */
            r = eval(trap, (struct env_exp){ cons(cons(symbol("ulisp"), cons(symbol("ULISP"), NIL())), env), cons(symbol("atom"), cons(symbol("ulisp"), NIL())) });
            ASSERT_EQ("(((ulisp ULISP) (t: True)))", text(cons(r.env, r.exp)));
            break;
        default:
            NOT_REACHED_HERE();
            break;
    }
    fclose(stderr);
    free(p);

    /* (cons) throws TRAP_ILLAREG. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("cons"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (cons)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cons hello) throws TRAP_NOSYM, therefore not defined `hello`. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("cons"), cons(symbol("hello"), NIL())) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOSYM:
            ASSERT_EQ("Value for symbol `hello` not found.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cons hello)w/{(hello: nil)} throws TRAP_ILLARG, therefore cdr part not exist. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ cons(cons(symbol("hello"), NIL()), NIL()), cons(symbol("cons"), cons(symbol("hello"), NIL())) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            /* XXX: this error message is confusing. it means that there is no cdr part exist in `(hello)`. */
            ASSERT_EQ("Illegal argument: (hello)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cons hello world)w/{(hello: nil)} throws TRAP_NOSYM, therefore no definition `world`. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ cons(cons(symbol("hello"), NIL()), NIL()), cons(symbol("cons"), cons(symbol("hello"), cons(symbol("world"), NIL()))) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOSYM:
            ASSERT_EQ("Value for symbol `world` not found.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cons hello (quote world))w/{(hello: nil)} ; => (nil: world) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ cons(cons(symbol("hello"), NIL()), NIL()),
            cons(symbol("cons"), cons(symbol("hello"), cons(cons(symbol("quote"), cons(symbol("world"), NIL())), NIL()))) });
            ASSERT_EQ("(((hello)) (): world)", (p = text(cons(r.env, r.exp))));
            break;
        default:
            NOT_REACHED_HERE();
            break;
    }
    fclose(stderr);
    free(p);

    /* (car (quote (x: 1))) ; => x */
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ NIL(), cons(symbol("car"), cons(cons(symbol("quote"), cons(cons(symbol("x"), symbol("1")), NIL())), NIL())) });
            ASSERT_EQ("((): x)", (p = text(cons(r.env, r.exp))));
            break;
        default:
            NOT_REACHED_HERE();
            break;
    }

    /* (car X) with env {(X: (x: 1)}} ; => x */
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ cons(cons(symbol("X"), cons(symbol("x"), symbol("1"))), NIL()),
            cons(symbol("car"), cons(symbol("X"), NIL())) });
            ASSERT_EQ("(((X x: 1)): x)", (p = text(cons(r.env, r.exp))));
            break;
        default:
            NOT_REACHED_HERE();
            break;
    }

    /* (car) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("car"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (car)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (car nil) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("car"), cons(NIL(), NIL())) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOTPAIR:
            ASSERT_EQ("`()` is not pair.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cdr (quote (x: 1))) ; => x */
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ NIL(),
            cons(symbol("cdr"), cons(cons(symbol("quote"), cons(cons(symbol("x"), symbol("1")), NIL())), NIL())) });
            ASSERT_EQ("((): 1)", (p = text(cons(r.env, r.exp))));
            break;
        default:
            NOT_REACHED_HERE();
            break;
    }

    /* (cdr X) with env {(X: (x: 1)}} ; => x */
    switch (setjmp(trap)) {
        case TRAP_NONE:
            r = eval(trap, (struct env_exp){ cons(cons(symbol("X"), cons(symbol("x"), symbol("1"))), NIL()), cons(symbol("cdr"), cons(symbol("X"), NIL())) });
            ASSERT_EQ("(((X x: 1)): 1)", (p = text(cons(r.env, r.exp))));
            break;
        default:
            NOT_REACHED_HERE();
            return 1;
    }

    /* (cdr) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("cdr"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (cdr)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cdr nil) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("cdr"), cons(NIL(), NIL())) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOTPAIR:
            ASSERT_EQ("`()` is not pair.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (set (quote re) (quote ulisp)) ; => t */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(3, symbol("set"), LIST(2, symbol("quote"), symbol("re")), LIST(2, symbol("quote"), symbol("ulisp")));
        r = eval(trap, (struct env_exp){ NIL(), x });
        /* environment expanded to hold (re: ulisp), and returned evaluated (assigned) value. */
        ASSERT_EQ("(((re: ulisp)): ulisp)", (p = text(cons(r.env, r.exp))));
        free(p);
    }

    /* (cond) throws ILLARG.  */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), LIST(1, symbol("cond")) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Illegal argument: (cond)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cond ()) throws NOTPAIR because () has no predicate. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            x = LIST(2, symbol("cond"), NIL());
            eval(trap, (struct env_exp){ NIL(), x });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_NOTPAIR:
            ASSERT_EQ("`()` is not pair.", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (cond (nil)) ; => nil */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(2, symbol("cond"), LIST(1, NIL()));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("(())", text(cons(r.env, r.exp))); // ((): ())
    }

    /* (cond ('t 'hello)) ; => hello */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(2, symbol("cond"), LIST(2, LIST(2, symbol("quote"), symbol("t")), LIST(2, symbol("quote"), symbol("hello"))));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("((): hello)", text(cons(r.env, r.exp)));
    }

    /* (cond ((set 'x nil)) ('t 'hello)) ; => hello, environment expanded. */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(3, symbol("cond"),
                 LIST(1, LIST(3, symbol("set"), LIST(2, symbol("quote"), symbol("x")), NIL())),
                 LIST(2, symbol("t"), LIST(2, symbol("quote"), symbol("hello"))));
        r = eval(trap, (struct env_exp){ env, x });
        ASSERT_EQ("(((x) (t: True)): hello)", text(cons(r.env, r.exp)));
    }

    /* (lambda) throws ILLARG. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            eval(trap, (struct env_exp){ NIL(), cons(symbol("lambda"), NIL()) });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("No closure param exist: (lambda)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (lambda ATOM) throws ILLARG. */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
        case TRAP_NONE:
            x = cons(symbol("lambda"), cons(symbol("ATOM"), NIL()));
            r = eval(trap, (struct env_exp){ env, x });
            /* $FALL-THROUGH$ */
        default:
            NOT_REACHED_HERE();
            break;
        case TRAP_ILLARG:
            ASSERT_EQ("Closure parameter should be list: (lambda ATOM)", p);
            break;
    }
    fclose(stderr);
    free(p);

    /* (lambda params body) returns closure: (env: (*applicable*: (param: body))). */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        r = eval(trap, (struct env_exp){ NIL(), cons(symbol("lambda"), cons(NIL(), NIL())) });
        ASSERT_EQ("((): *applicable*)", (p = text(cons(r.env, r.exp)))); // ((): (*applicable*: ((): ())))
        free(p);

        r = eval(trap, (struct env_exp){ env, cons(symbol("lambda"), cons(cons(NIL(), LIST(3, symbol("set"), symbol("t"), symbol("False"))), NIL())) });
        ASSERT_EQ("(((t: True)): *applicable*)", (p = text(cons(r.env, r.exp))));
        free(p);
    }

    /* ((lambda ())) ; => nil */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(1, LIST(2, symbol("lambda"), NIL()));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("(())", text(cons(r.env, r.exp))); // ((): ())
    }

    /* ((lambda () (quote ulisp))) ; => ulisp */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(1, LIST(3, symbol("lambda"), NIL(), LIST(2, symbol("quote"), symbol("ulisp"))));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("((): ulisp)", text(cons(r.env, r.exp)));
    }

    /* ((lambda (x) x) (quote ulisp)) ; => ulisp */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(1, LIST(3, symbol("lambda"), NIL(), LIST(2, symbol("quote"), symbol("ulisp"))));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("((): ulisp)", text(cons(r.env, r.exp)));
    }

    /* ((lambda (x) x) (quote ulisp)) ; => ulisp */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(2, LIST(3, symbol("lambda"), LIST(1, symbol("x")), symbol("x")), LIST(2, symbol("quote"), symbol("ulisp")));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("((): ulisp)", text(cons(r.env, r.exp)));
    }

    /* ((lambda (x) x) (quote (1 2 3))) ; => (1 2 3) */
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
    } else {
        x = LIST(2, LIST(3, symbol("lambda"), LIST(1, symbol("x")), symbol("x")), LIST(2, symbol("quote"), LIST(3, symbol("1"), symbol("2"), symbol("3"))));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("(() 1 2 3)", text(cons(r.env, r.exp))); // ((): (1 2 3)) ; => (() 1 2 3)
    }

    /* ((lambda (x y) (cons y (cons x nil))) (quote world) (quote hello)) ; => (hello world) */
    stderr = open_memstream(&p, &n);
    if (setjmp(trap)) {
        NOT_REACHED_HERE();
        puts(p);
    } else {
        x = LIST(3, LIST(3, symbol("lambda"), LIST(2, symbol("x"), symbol("y")), LIST(3, symbol("cons"), symbol("y"), LIST(3, symbol("cons"), symbol("x"), NIL()))),
                 LIST(2, symbol("quote"), symbol("world")), LIST(2, symbol("quote"), symbol("hello")));
        r = eval(trap, (struct env_exp){ NIL(), x });
        ASSERT_EQ("(() hello world)", text(cons(r.env, r.exp))); // ((): (hello world)) ; => (() hello world)
    }
    fclose(stderr);
    free(p);

    stderr = fp;
    printf("total %d run, NG = %d\n", ok + ng, ng);

    return ng;
}

#include <stdarg.h>
const struct sexp* LIST(unsigned numElems, ...) {
    const struct sexp** buf = malloc(sizeof(const struct sexp*) * numElems);
    unsigned i;
    va_list exps;
    va_start(exps, numElems);
    for (i = 0; i < numElems; ++i) {
        const struct sexp* e = va_arg(exps, const struct sexp*);
        memcpy(buf + i, &e, sizeof(e));
    }
    va_end(exps);
    {
        const struct sexp* e = NIL();
        while (numElems--) {
            const struct sexp* p = cons(buf[numElems], e);
            memcpy(&e, &p, sizeof(p));
        }
        return e;
    }
}
