#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>

#include "env.h"

struct sexp;

enum TRAPCODE {
  TRAP_NONE,
  TRAP_NOINPUT,
  TRAP_NOSYM,
  TRAP_ILLARG,
  TRAP_NOTPAIR,
  TRAP_NOTAPPLICABLE,
};

/**
 * Get special symbol NIL.
 */
const struct sexp* NIL();

/**
 * Test whether sexp is nil or not.
 */
bool nil(const struct sexp* sexp);

/**
 * Test whether sexp is atom or not (i.e. pair).
 */
bool atom(const struct sexp* sexp);

/**
 * Make symbol sexp.
 */
const struct sexp* symbol(const char* name);

/**
 * Make pair of sexps.
 */
const struct sexp* cons(const struct sexp* fst, const struct sexp* snd);

/**
 * Return `car` of sexp.
 */
const struct sexp* fst(const struct sexp* sexp);

/**
 * Return `cdr` of sexp.
 */
const struct sexp* snd(const struct sexp* sexp);

/**
 * Read expression from stdin.
 * 
 * @param trap is a execution state of host. \
 * TRAP_NOINPUT indicates no effective expression buffered in stdin. \
 * TRAP_ILLARG indicates that input is terminated amongst in a list. \
 * TRAP_NOTPAIR shows that there is 2 or more S-expressions following list construction operator `:`.
 * @return S-expression.
 */
const struct sexp* read(jmp_buf trap);

/**
 * Evaluate expression on the environment.
 * 
 * @param trap is a execution state of host. \
 * If error condition are met, rewind to trapped state by longjmp. \
 * Error message are flushed to stderr.
 * @param sexp is a S-experession.
 * @param env is a environment.
 * @return S-expression which is evaluated on the environment.
 */
const struct sexp* eval(jmp_buf trap, const struct sexp* sexp, struct env* env);

/**
 * Build text representation of sexp.
 *
 * You must need free returned string.
 */
char* text(const struct sexp* sexp);

/**
 * Write sexp into stream represented by fp.
 */
void write(FILE* fp, const struct sexp* sexp);
