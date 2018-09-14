#include "ulisp.h"

#include <memory.h>
#include <stdlib.h>

struct pair { struct sexp fst, snd; };

struct sexp NIL() { return symbol(NULL); }

static const struct pair* asPair(const struct sexp sexp) { return sexp.p; }

bool nil(const struct sexp sexp) { return atom(sexp) && !sexp.p; }

bool atom(const struct sexp sexp) { return sexp.symbol; }

struct sexp symbol(const char *s) {
  return (struct sexp) { true, s };
}

struct sexp cons(const struct sexp fst, const struct sexp snd) {
  struct pair* pair = malloc(sizeof(struct pair));
  memcpy(pair, &(struct pair) { fst, snd }, sizeof(struct pair));
  return (struct sexp) { false, pair };
}

struct sexp fst(const struct sexp sexp) { return asPair(sexp)->fst; }

struct sexp snd(const struct sexp sexp) { return asPair(sexp)->snd; }

#ifdef UNITTEST_
#include <string.h>

int main() {
  { /* property of NIL. */
    if (!atom(NIL()) || NIL().p || !nil(NIL()) || !nil(symbol(NULL))) {
      return 1;
    }
  }

  { /* cons'ed sexp. */
    struct sexp pair = cons(symbol("hello"), symbol("world"));
    struct sexp car = fst(pair);
    struct sexp cdr = snd(pair);
    /* cons'ed sexp is NOT atom. */
    if (atom(pair)) {
      return 2;
    }
    /* 1st is atom and it's value is "hello". */
    if (!atom(car) || strcmp(car.p, "hello")) {
      return 3;
    }
    /* 2nd is atom and it's value is "world". */
    if (!atom(cdr) || strcmp(cdr.p, "world")) {
      return 4;
    }
  }

  return 0;
}
#endif
