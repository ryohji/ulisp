#include "ulisp.h"
#include "env.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

enum tag {
    SYMBOL,
    PAIR,
    APPLICABLE,
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

struct applicable {
    enum tag tag;
    struct env* env;
    const struct sexp* params;
    const struct sexp* body;
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

const char* name_of(const struct sexp* exp) {
    switch (exp->tag) {
    case SYMBOL:
        return ((const struct symbol*)exp)->p;
    case APPLICABLE:
        return "*applicable*";
    default:
        return "";
    }
}

const struct sexp* make_applicable(struct env* env, const struct sexp* params, const struct sexp* body) {
    struct sexp* applicable = malloc(sizeof(struct applicable));
    memcpy(applicable, &(struct applicable) { .tag = APPLICABLE, .env = env, .params = params, .body = body, }, sizeof(struct applicable));
    return applicable;
}

static const struct applicable* make_sure_applicable(jmp_buf trap, const struct sexp* exp) {
    if (exp->tag != APPLICABLE) {
        longjmp(trap, TRAP_NOTAPPLICABLE);
    } else {
        return (void*)exp;
    }
}

struct env* get_environment(jmp_buf trap, const struct sexp* exp) {
    return make_sure_applicable(trap, exp)->env;
}

const struct sexp* get_body(jmp_buf trap, const struct sexp* exp) {
    return make_sure_applicable(trap, exp)->body;
}

const struct sexp* get_params(jmp_buf trap, const struct sexp* exp) {
    return make_sure_applicable(trap, exp)->params;
}
