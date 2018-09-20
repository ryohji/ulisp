#pragma once

#include <stdbool.h>
#include <setjmp.h>

struct sexp {
  const bool symbol;
  const void *const p;
};

enum TRAPCODE {
  TRAP_NONE,
  TRAP_NOINPUT,
  TRAP_NOSYM,
  TRAP_ILLARG,
  TRAP_NOTPAIR,
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
 * Read expression from stdin.
 * 
 * @param trap is a execution state of host. \
 * TRAP_NOINPUT indicates no effective expression buffered in stdin. \
 * TRAP_ILLARG indicates that input is terminated amongst in a list. \
 * TRAP_NOTPAIR shows that there is 2 or more S-expressions following list construction operator `:`.
 * @return S-expression.
 */
struct sexp read(jmp_buf trap);

/**
 * Evaluate expression on the environment.
 * 
 * @param trap is a execution state of host. \
 * If error condition are met, rewind to trapped state by longjmp. \
 * Error message are flushed to stderr.
 * @param env_exp is a pair of (environment: expression).
 * @return S-expression which is a pair of resulting environment and evaluated value.
 */
struct sexp eval(jmp_buf trap, const struct sexp env_exp);

/**
 * Build text representation of sexp.
 *
 * You must need free returned string.
 */
char* text(const struct sexp sexp);
