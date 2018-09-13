#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* text_car(const struct sexp sexp);
static char* text_cdr(const struct sexp sexp);
static char* text_pair(const struct sexp sexp, char prefix);

char* text(const struct sexp sexp) {
  return text_car(sexp);
}

char* text_car(const struct sexp sexp) {
  if (atom(sexp)) {
    const char* const p = nil(sexp) ? "()" : sexp.p;
    return strdup(p);
  } else {
    return text_pair(sexp, '(');
  }
}

char* text_cdr(const struct sexp sexp) {
  if (atom(sexp)) {
    const char* const p = nil(sexp) ? "" : ": ";
    const char* const q = nil(sexp) ? "" : sexp.p;
    char* buf = malloc(strlen(p) + strlen(q) + 2);
    sprintf(buf, "%s%s)", p, q);
    return buf;
  } else {
    return text_pair(sexp, ' ');
  }
}

inline char* text_pair(const struct sexp sexp, char prefix) {
  char* const car = text_car(fst(sexp));
  char* const cdr = text_cdr(snd(sexp));
  char* const buf = malloc(strlen(car) + strlen(cdr) + 2);
  sprintf(buf, "%c%s%s", prefix, car, cdr);
  free(car);
  free(cdr);
  return buf;
}

#ifdef UNITTEST_
/* NEED data.o without UNITTEST_. */

int main() {
  char* str;

  str = text(NIL());
  if (strcmp("()", str)) return 1;
  free(str);

  str = text(symbol("hello"));
  if (strcmp("hello", str)) return 2;
  free(str);

  str = text(cons(symbol("about"), symbol("blank")));
  if (strcmp("(about: blank)", str)) return 3;
  free(str);

  str = text(cons(symbol("hello"), cons(symbol("world"), NIL())));
  if (strcmp("(hello world)", str)) return 4;
  free(str);

  str = text(cons(symbol("ulisp"), NIL()));
  if (strcmp("(ulisp)", str)) return 5;
  free(str);

  str = text(cons(cons(symbol("x"), symbol("1")), cons(cons(symbol("y"), symbol("2")), NIL())));
  if (strcmp("((x: 1) (y: 2))", str)) return 6;
  free(str);

  return 0;
}
#endif
