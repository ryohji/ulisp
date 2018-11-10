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
