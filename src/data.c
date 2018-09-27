#include "ulisp.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

enum tag {
    SYMBOL,
    PAIR,
};

struct sexp {
    enum tag tag;
};

struct symbol {
    enum tag tag;
    char p[1];
};

struct pair {
    enum tag tag;
    const struct sexp* fst;
    const struct sexp* snd;
};

const struct sexp* NIL() {
    return NULL;
}

bool nil(const struct sexp* sexp) {
    return !sexp;
}

bool atom(const struct sexp* sexp) {
    return nil(sexp) || sexp->tag != PAIR;
}

const struct sexp* symbol(const char* name) {
    struct symbol* exp = malloc(sizeof(struct symbol) + strlen(name));
    exp->tag = SYMBOL;
    strcpy(exp->p, name);
    return (void*) exp;
}

const struct sexp* cons(const struct sexp* fst, const struct sexp* snd) {
    struct pair* exp = malloc(sizeof(struct pair));
    exp->tag = PAIR;
    exp->fst = fst;
    exp->snd = snd;
    return (void*) exp;
}

const struct sexp* fst(const struct sexp* sexp) {
    const struct pair* pair = (const void*) sexp;
    return pair->fst;
}

const struct sexp* snd(const struct sexp* sexp) {
    const struct pair* pair = (const void*) sexp;
    return pair->snd;
}

const struct sexp* APPLICABLE() {
    struct applicable {
        enum tag tag;
        char p[13];
    };
    static const struct applicable applicable = { SYMBOL, "*applicable*", };
    return (const void*)&applicable;
}

const char* name_of(const struct sexp* exp) {
    switch (exp->tag) {
    case SYMBOL:
        return ((const struct symbol*)exp)->p;
    default:
        return "";
    }
}
