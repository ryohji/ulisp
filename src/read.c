#include "ulisp.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(a, b) (!strcmp((a), (b)))

static struct sexp read_aux(jmp_buf trap, char* token);
static struct sexp read_cdr(jmp_buf trap, struct sexp x);
static struct sexp append_reverse(struct sexp xs, struct sexp ys);
static char* fgettoken(jmp_buf trap, FILE* fp);
static void fgettok_normal(jmp_buf trap, FILE* fin, FILE* fout, bool trailing);
static void fgettok_escape(jmp_buf trap, FILE* fin, FILE* fout);
static void fgettok_begin(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, false); }
static void fgettok_trail(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, true); }

struct sexp read(jmp_buf trap) {
    char* token = fgettoken(trap, stdin);
    if (STR_EQ("", token)) {
        free(token);
        fprintf(stderr, "No input.");
        fflush(stderr);
        longjmp(trap, TRAP_NOINPUT);
    }
    return read_aux(trap, token);
}

static struct sexp read_aux(jmp_buf trap, char* p) {
    if (STR_EQ("", p)) {
        free(p);
        fprintf(stderr, "Unexpected end of data.");
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        if (STR_EQ("(", p)) {
            free(p);
            p = fgettoken(trap, stdin);
            if (STR_EQ(")", p)) {
                free(p);
                return NIL();
            } else {
                return read_cdr(trap, cons(read_aux(trap, p), NIL()));
            }
        } else {
            return symbol(p);
        }
    }
}

static struct sexp read_cdr(jmp_buf trap, struct sexp x) {
    char* p = fgettoken(trap, stdin);
    if (STR_EQ("", p)) {
        free(p);
        fprintf(stderr, "Unexpected end of data.");
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        if (STR_EQ(")", p)) {
            free(p);
            return append_reverse(x, NIL());
        } else {
            if (STR_EQ(":", p)) {
                free(p);
                struct sexp y = read_aux(trap, fgettoken(trap, stdin));
                p = fgettoken(trap, stdin);
                if (!STR_EQ(")", p)) {
                    fprintf(stderr, "Unexpected token %s where expected ')' after %s.", p, text(y));
                    fflush(stderr);
                    free(p);
                    longjmp(trap, TRAP_NOTPAIR);
                } else {
                    free(p);
                    return append_reverse(x, y);
                }
            } else {
                return read_cdr(trap, cons(read_aux(trap, p), x));
            }
        }
    }
}

static struct sexp append_reverse(struct sexp xs, struct sexp ys) {
    if (atom(xs)) {
        return ys;
    } else {
        return append_reverse(snd(xs), cons(fst(xs), ys));
    }
}

static char* fgettoken(jmp_buf trap, FILE* fp) {
    char *p;
    size_t n;
    FILE* mem = open_memstream(&p, &n);
    fgettok_begin(trap, fp, mem);
    fclose(mem);
    return p;
}

static void fgettok_normal(jmp_buf trap, FILE* fin, FILE* fout, bool trailing) {
    const int c = fgetc(fin);
    switch (c) {
    default:
        fputc(c, fout);
        return fgettok_trail(trap, fin, fout);
    case ' ':
    case '\t':
    case '\n':
        return trailing ? (void) 0 : fgettok_begin(trap, fin, fout);
    case '\\':
        return fgettok_escape(trap, fin, fout);
    case '(':
    case ':':
    case ')':
        return (void) (trailing ? ungetc(c, fin) : fputc(c, fout));
    case EOF:
        return;
    }
}

static void fgettok_escape(jmp_buf trap, FILE* fin, FILE* fout) {
    const int c = fgetc(fin);
    switch (c) {
    case ' ':
    case '\t':
    case '\\':
    case '(':
    case ':':
    case ')':
        fputc(c, fout);
        // $FALL-THROUGH$
    case '\n':
        return fgettok_trail(trap, fin, fout);
    case EOF:
        return;
    default:
        fprintf(stderr, "Unknown escape character: %c\n", c);
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    }
}

#ifdef UNITTEST_
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
        struct sexp x = read(trap);
        fclose(stdin);
        ASSERT_EQ(sexp, text(x));
        stdin = fp;
    }

    printf("Total %d run, NG = %d\n", ok + ng, ng);
    return -ng;
}
#endif
