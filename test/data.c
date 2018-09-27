#include "ulisp.h"
#include "../src/data.c"

#include <stdio.h>
#include <string.h>

typedef const struct sexp SEXP;

#define ASSERT_TRUE(x) if (!(x)) { printf("!`" #x "`\n@%d\n", __LINE__); ng += 1; } else { ok += 1; }
int main() {
    unsigned ok = 0, ng = 0;
    { /* property of NIL. */
        ASSERT_TRUE(atom(NIL()));
        ASSERT_TRUE(nil(NIL()));
    }

    { /* check symbol implementation. */
        const struct sexp* p = symbol("hello");
        ASSERT_TRUE(atom(p));
        ASSERT_TRUE((p->tag == SYMBOL));
        ASSERT_TRUE((strcmp("hello", (void*) (p+1)) == 0));
    }

    { /* cons'ed sexp. */
        SEXP* pair = cons(symbol("hello"), symbol("world"));
        SEXP* car = fst(pair);
        SEXP* cdr = snd(pair);

        ASSERT_TRUE(!atom(pair));
        ASSERT_TRUE((pair->tag == PAIR));

        ASSERT_TRUE(atom(car));
        ASSERT_TRUE((car->tag == SYMBOL));
        ASSERT_TRUE((strcmp("hello", (void*) (car+1)) == 0));

        ASSERT_TRUE(atom(cdr));
        ASSERT_TRUE((cdr->tag == SYMBOL));
        ASSERT_TRUE((strcmp("world", (void*) (cdr+1)) == 0));
    }

    printf("total %d run, NG = %d\n", ok + ng, ng);
    return -ng;
}
