#include "ulisp.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(a, b) (!strcmp((a), (b)))

static const struct sexp* read_aux(jmp_buf trap, char* token);
static const struct sexp* read_cdr(jmp_buf trap);

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

static const struct sexp* read_aux(jmp_buf trap, char* token) {
    if (STR_EQ("", token)) {
        free(token);
        fprintf(stderr, "Unexpected end of data.");
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        if (STR_EQ("(", token)) {
            free(token);
            token = fgettoken(trap, stdin);
            if (STR_EQ(")", token)) {
                free(token);
                return NIL();
            } else {
                return cons(read_aux(trap, token), read_cdr(trap));
            }
        } else {
            const struct sexp* exp = symbol(token);
            free(token);
            return exp;
        }
    }
}

static const struct sexp* read_cdr(jmp_buf trap) {
    char* token = fgettoken(trap, stdin);
    if (STR_EQ("", token)) {
        free(token);
        fprintf(stderr, "Unexpected end of data.");
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        if (STR_EQ(")", token)) {
            free(token);
            return NIL();
        } else {
            if (STR_EQ(":", token)) {
                free(token);
                const struct sexp* y = read_aux(trap, fgettoken(trap, stdin));
                token = fgettoken(trap, stdin);
                if (!STR_EQ(")", token)) {
                    fprintf(stderr, "Unexpected token %s where expected ')' after %s.", token, text(y));
                    fflush(stderr);
                    free(token);
                    longjmp(trap, TRAP_NOTPAIR);
                } else {
                    free(token);
                    return y;
                }
            } else {
                return cons(read_aux(trap, token), read_cdr(trap));
            }
        }
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
