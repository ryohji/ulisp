#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* name_of(const struct sexp* exp);

static void fwrite_car(FILE* fp, const struct sexp* exp);
static void fwrite_cdr(FILE* fp, const struct sexp* exp);
static void fwrite_pair(FILE* fp, const struct sexp* exp, const char* prefix);

void write(FILE* fp, const struct sexp* exp) {
    return fwrite_car(fp, exp);
}

char* text(const struct sexp* exp) {
    char* p;
    size_t n;
    FILE* fp = open_memstream(&p, &n);
    write(fp, exp);
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
