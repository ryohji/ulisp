#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* text_cdr(const struct sexp sexp);

char* text(const struct sexp sexp) {
  if (atom(sexp)) {
    if (nil(sexp)) {
      return strdup("()");
    } else {
      return strdup(sexp.p);
    }
  } else {
    char* const car = text(fst(sexp));
    char* const cdr = text_cdr(snd(sexp));
    char* const buf = malloc(strlen(car) + strlen(cdr) + 2);
    sprintf(buf, "(%s%s", car, cdr);
    free(car);
    free(cdr);
    return buf;
  }
}

char* text_cdr(const struct sexp sexp) {
  if (atom(sexp)) {
    if (nil(sexp)) {
      return strdup(")");
    } else {
      char* const buf = malloc(strlen(sexp.p) + 5);
      sprintf(buf, " . %s)", sexp.p);
      return buf;
    }
  } else {
    char* const car = text(fst(sexp));
    char* const cdr = text_cdr(snd(sexp));
    char* const buf = malloc(strlen(car) + strlen(cdr) + 2);
    sprintf(buf, " %s%s", car, cdr);
    free(car);
    free(cdr);
    return buf;
  }
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

  str = text(cons(symbol("hello"), symbol("world")));
  if (strcmp("(hello . world)", str)) return 3;
  free(str);

  str = text(cons(symbol("ulisp"), NIL()));
  if (strcmp("(ulisp)", str)) return 4;
  free(str);

  str = text(cons(cons(symbol("x"), symbol("1")), cons(cons(symbol("y"), symbol("2")), NIL())));
  if (strcmp("((x . 1) (y . 2))", str)) return 5;
  free(str);

  return 0;
}
#endif
