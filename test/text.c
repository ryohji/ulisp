#include "ulisp.h"
#include "../src/text.c"
#include "../src/data.c"

#include <stdio.h>
#include <string.h>

#define ASSERT_EQ(expect, actual) if (strcmp(expect, actual))\
 { printf("expect: %s\n""actual: %s\n""@%d\n", expect, actual, __LINE__); ng += 1; } else { ok += 1; }

int main() {
    unsigned ok = 0, ng = 0;
    char* str;

    str = text(NIL());
    ASSERT_EQ("()", str);
    free(str);

    str = text(symbol("hello"));
    ASSERT_EQ("hello", str);
    free(str);

    str = text(cons(symbol("about"), symbol("blank")));
    ASSERT_EQ("(about: blank)", str);
    free(str);

    str = text(cons(symbol("hello"), cons(symbol("world"), NIL())));
    ASSERT_EQ("(hello world)", str);
    free(str);

    str = text(cons(symbol("ulisp"), NIL()));
    ASSERT_EQ("(ulisp)", str);
    free(str);

    str = text(cons(cons(symbol("x"), symbol("1")), cons(cons(symbol("y"), symbol("2")), NIL())));
    ASSERT_EQ("((x: 1) (y: 2))", str);
    free(str);

    printf("total %d run, NG = %d\n", ok + ng, ng);
    return -ng;
}