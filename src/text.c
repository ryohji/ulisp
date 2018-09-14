#include "ulisp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fwrite_car(FILE* fp, const struct sexp sexp);
static void fwrite_cdr(FILE* fp, const struct sexp sexp);
static void fwrite_pair(FILE* fp, const struct sexp sexp, const char* prefix);

char* text(const struct sexp sexp) {
  char* p;
  size_t n;
  FILE* fp = open_memstream(&p, &n);
  fwrite_car(fp, sexp);
  fclose(fp);
  return p;
}

void fwrite_car(FILE* fp, const struct sexp sexp) {
  if (atom(sexp)) {
    const char* const q = nil(sexp) ? "()" : sexp.p;
    fprintf(fp, "%s", q);
  } else {
    fwrite_pair(fp, sexp, "(");
  }
}

void fwrite_cdr(FILE* fp, const struct sexp sexp) {
  if (atom(sexp)) {
    const char* const p = nil(sexp) ? "" : ": ";
    const char* const q = nil(sexp) ? "" : sexp.p;
    fprintf(fp, "%s%s)", p, q);
  } else {
    fwrite_pair(fp, sexp, " ");
  }
}

inline void fwrite_pair(FILE* fp, const struct sexp sexp, const char* prefix) {
  fprintf(fp, "%s", prefix);
  fwrite_car(fp, fst(sexp));
  fwrite_cdr(fp, snd(sexp));
}

#ifdef UNITTEST_
/* NEED data.o compiled without UNITTEST_. */

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
