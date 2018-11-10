#include "env.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>

struct pair {
    const char* name;
    const void* value;
};

struct dict {
    unsigned caps; /* indicates the number of pairs. */
    unsigned size; /* shows the number of pairs used. */
    struct pair pair[0];
};

struct env {
    struct env* outer;
    struct dict* definition;
};

struct env* env_create(struct env* base) {
    struct env* const env = malloc(sizeof(struct env));
    struct dict* const dict = malloc(sizeof(struct dict) + 4 * sizeof(struct pair));
    memcpy(dict, &(struct dict) { .caps = 4, .size = 0 }, sizeof(struct dict));
    memcpy(env, &(struct env) { .outer = base, .definition = dict }, sizeof(struct env));
    return env;
}

void env_define(struct env* env, const char* name, const void* value) {
    struct dict* defs = env->definition;
    if (defs->caps == defs->size) {
        // expand room
        defs = realloc(defs, sizeof(struct dict) + 2 * defs->caps * sizeof(struct pair));
        env->definition = defs;
        defs->caps *= 2;
    }
    memcpy(defs->pair + defs->size, &(struct pair) { .name = name, .value = value }, sizeof(struct pair));
    defs->size += 1;
}

const void* env_search(struct env* env, const char* name) {
    struct dict* defs = env->definition;
    struct pair* const rend = defs->pair - 1;
    struct pair* it = defs->pair + defs->size - 1;
    while (it != rend && strcmp(name, it->name)) {
        --it;
    }
    if (it == rend) {
        return env->outer ? env_search(env->outer, name) : NULL;
    } else {
        return it->value;
    }
}
