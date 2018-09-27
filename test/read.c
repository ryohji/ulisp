#include "ulisp.h"
#include "../src/read.c"
#include "../src/data.c"
#include "../src/text.c"

static bool assert_eq_impl(const char* expect, const char* actual) {
    if (strcmp(expect, actual)) {
        printf("expect: %s\n" "actual: %s\n" "@%d\n", expect, actual, __LINE__);
        return true;
    } else {
        return false;
    }
}
#define ASSERT_EQ(expect, actual) do { if (assert_eq_impl(expect, actual)) { ng += 1; } else { ok += 1; } } while(false)
#define ASSERT_FAIL(message) do { printf("%s\n@%d\n", message, __LINE__); ng += 1; } while(false)

int main() {
    unsigned ok = 0, ng = 0;
    char* p;
    jmp_buf trap;

    if (setjmp(trap)) {
        ASSERT_FAIL("NOT REACHED HERE");
    } else {
        char hello[] = "((hello)\\\nworld)\\(\\:hello\\\\\\ world\\:\\)";
        FILE* fp = fmemopen(hello, sizeof(hello), "r");
        ASSERT_EQ("(",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("(",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("hello", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ(")",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("world", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ(")",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("(:hello\\ world:)", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("",      (p = fgettoken(trap, fp))); free(p);
        fclose(fp);
    }

    if (setjmp(trap)) {
        ASSERT_FAIL("NOT REACHED HERE");
    } else {
        char sexp[] = "(set (quote reverse-append) (lambda (x y) (cond ((atom x) y) (t (reverse-append (cdr x) (cons (car x) y))))))";
        FILE* fp = stdin;
        stdin = fmemopen(sexp, sizeof(sexp), "r");
        const struct sexp* x = read(trap);
        fclose(stdin);
        ASSERT_EQ(sexp, text(x));
        stdin = fp;
    }

    printf("Total %d run, NG = %d\n", ok + ng, ng);
    return -ng;
}
