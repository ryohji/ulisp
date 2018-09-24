#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fwrite_car(FILE* fp, const struct sexp* exp);
static void fwrite_cdr(FILE* fp, const struct sexp* exp);
static void fwrite_pair(FILE* fp, const struct sexp* exp, const char* prefix);

static const char* name_of(const struct sexp* exp);

char* text(const struct sexp* exp) {
    char* p;
    size_t n;
    FILE* fp = open_memstream(&p, &n);
    fwrite_car(fp, exp);
    fclose(fp);
    return p;
}

static void fwrite_car(FILE* fp, const struct sexp* exp) {
    if (atom(exp)) {
        if (nil(exp)) {
            fprintf(fp, "()");
        } else {
            fprintf(fp, "%s", name_of(exp));
        }
    } else {
        fwrite_pair(fp, exp, "(");
    }
}

static void fwrite_cdr(FILE* fp, const struct sexp* exp) {
    if (atom(exp)) {
        if (nil(exp)) {
            fprintf(fp, ")");
        } else {
            fprintf(fp, ": %s)", name_of(exp));
        }
    } else {
        fwrite_pair(fp, exp, " ");
    }
}

inline static void fwrite_pair(FILE* fp, const struct sexp* exp, const char* prefix) {
    fprintf(fp, "%s", prefix);
    fwrite_car(fp, fst(exp));
    fwrite_cdr(fp, snd(exp));
}

static const char* name_of(const struct sexp* exp) {
    /* assume that there is only 'symbol' */
    return (const void*)((const int*)(exp) + 1);
}
