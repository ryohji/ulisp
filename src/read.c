#include "ulisp.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(a, b) (!strcmp((a), (b)))

static const struct sexp* read_aux(jmp_buf trap, char* token);
static const struct sexp* read_cdr(jmp_buf trap, const struct sexp* x);
static const struct sexp* append_reverse(const struct sexp* xs, const struct sexp* ys);

static char* fgettoken(jmp_buf trap, FILE* fp);
static void fgettok_normal(jmp_buf trap, FILE* fin, FILE* fout, bool trailing);
static void fgettok_escape(jmp_buf trap, FILE* fin, FILE* fout);
static void fgettok_begin(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, false); }
static void fgettok_trail(jmp_buf trap, FILE* fin, FILE* fout) { return fgettok_normal(trap, fin, fout, true); }

const struct sexp* read(jmp_buf trap) {
    char* token = fgettoken(trap, stdin);
    if (STR_EQ("", token)) {
        free(token);
        fprintf(stderr, "No input.");
        fflush(stderr);
        longjmp(trap, TRAP_NOINPUT);
    }
    return read_aux(trap, token);
}

static const struct sexp* read_aux(jmp_buf trap, char* p) {
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

static const struct sexp* read_cdr(jmp_buf trap, const struct sexp* x) {
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
                const struct sexp* y = read_aux(trap, fgettoken(trap, stdin));
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

static const struct sexp* append_reverse(const struct sexp* xs, const struct sexp* ys) {
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
