#pragma once
#include "ulisp.h"

struct env;

struct env* env_alloc();

void env_free(struct env*);

void env_clear(struct env*);

void env_push_front(struct env*, struct sexp var, struct sexp val);

struct sexp env_find(struct env*, struct sexp var);
