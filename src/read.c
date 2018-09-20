#include "ulisp.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>

static char* fgettoken(jmp_buf trap, FILE* fp);
static void fgettok_normal(jmp_buf trap, FILE* fin, FILE* fout, bool trailing);
static void fgettok_escape(jmp_buf trap, FILE* fin, FILE* fout);
static void fgettok_begin(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, false); }
static void fgettok_trail(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, true); }

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
        return trailing ? (void) 0 : fgettok_begin(trap, fin, fout);
    case '\\':
        return fgettok_escape(trap, fin, fout);
    case '(':
    case ':':
    case ')':
        return (void) (trailing ? ungetc(c, fin) : fputc(c, fout));
    case '\n':
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    char hello[] = "((hello)\\\nworld)\\(\\:hello\\\\\\ world\\:\\)";
    char* p;
    jmp_buf trap;

    FILE* fp = fmemopen(hello, sizeof(hello), "r");
    if (setjmp(trap)) {
        ASSERT_FAIL("NOT REACHED HERE");
    } else {
        ASSERT_EQ("(",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("(",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("hello", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ(")",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("world", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ(")",     (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("(:hello\\ world:)", (p = fgettoken(trap, fp))); free(p);
        ASSERT_EQ("",      (p = fgettoken(trap, fp))); free(p);
    }
    fclose(fp);

    printf("Total %d run, NG = %d\n", ok + ng, ng);
    return -ng;
}
#endif
