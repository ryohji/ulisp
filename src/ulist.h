#pragma once

#include <stdbool.h>

struct sexp {
  const bool symbol;
  const void *const p;
};

/**
 * Get special symbol NIL.
 */
struct sexp NIL();

/**
 * Test whether sexp is nil or not.
 */
bool nil(const struct sexp sexp);

/**
 * Test whether sexp is atom or not (i.e. pair).
 */
bool atom(const struct sexp sexp);

/**
 * Make symbol sexp.
 */
struct sexp symbol(const char* symbol);

/**
 * Make pair of sexps.
 */
struct sexp cons(const struct sexp fst, const struct sexp snd);

/**
 * Return `car` of sexp.
 */
struct sexp fst(const struct sexp sexp);

/**
 * Return `cdr` of sexp.
 */
struct sexp snd(const struct sexp sexp);

/**
 * Search the value of symbol from environment list.
 *
 * Environment list is the list of (key, value) pair.
 * I.e. ((key1.val1).((key2.val2).((key3.val3). ... .nil)))
 */
struct sexp value(const char* symbol, const struct sexp list);
