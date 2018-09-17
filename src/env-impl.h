#pragma once
#include "env.h"

#include <memory.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct env {
    unsigned capacity;
    unsigned elements;
    struct sexp* list;
};

struct env* env_alloc() {
    const unsigned caps = 2;
    struct env* const buf = (struct env*) malloc(sizeof(struct env));
    *buf = (struct env) {
        .capacity = caps,
        .elements = 0,
        .list = (struct sexp*) malloc(sizeof(struct sexp) * caps),
    };
    return buf;
}

void env_free(struct env* p) {
    free(p);
}

void env_clear(struct env* p) {
    p->elements = 0;
}

void env_push_front(struct env* p, struct sexp var, struct sexp val) {
    unsigned caps = p->capacity;
    unsigned elms = p->elements;
    if (caps == elms) {
        p->list = (struct sexp*) realloc(p->list, sizeof(struct sexp) * caps * 2);
        p->capacity += caps;
    }
    struct sexp def = cons(var, val);
    memcpy(p->list + elms, &def, sizeof(struct sexp));
    p->elements += 1;
}

struct sexp env_find(struct env* p, struct sexp var) {
    struct sexp* const rend = p->list - 1;
    struct sexp* it = rend + p->elements;
    while (it != rend && strcmp((const char*) fst(*it).p, (const char*) var.p)) {
        it -= 1;
    }
    return it != rend ? *it : NIL();
}

#ifdef __cplusplus
}
#endif
