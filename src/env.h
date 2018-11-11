#pragma once

struct env;

/**
 * Create new environment.
 * 
 * @param base is outer environment that is searched definition if this environment does not have specified definition.
 * Can be null.
 */
struct env* env_create(struct env* base);

/**
 * Introduce new definition into the environment.
 * 
 * This call ties the name and the value in the passed environment.
 */
void env_define(struct env* env, const char* name, const void* value);

/**
 * Search the value tied with the passed name.
 * 
 * If the name does not found and this environment created with another environment, definition recursively searched.
 * @return the value if found. otherwise null.
 */
const void* env_search(struct env* env, const char* name);

/**
 * Peek enclosed environment on creation.
 */
struct env* env_base(struct env* env);


struct env_iterator;

const struct env_iterator* env_it_begin(struct env* env);

const struct env_iterator* env_it_end(struct env* env);

const struct env_iterator* env_it_next(const struct env_iterator* iter);

const char* env_it_name(const struct env_iterator* iter);

const void* env_it_value(const struct env_iterator* iter);
