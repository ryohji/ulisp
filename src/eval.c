#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static const char* Err_value_not_found = "Value for symbol `%s` not found.\n";

static struct sexp find(struct sexp e, struct sexp env);

struct sexp eval(jmp_buf trap, const struct sexp exp, const struct sexp env) {
  if (atom(exp)) {
    if (nil(exp)) {
      return NIL();
    } else {
      struct sexp pair = find(exp, env);
      if (nil(pair)) {
	fprintf(stderr, Err_value_not_found, exp.p);
	longjmp(trap, 1);
      } else {
	return snd(pair);
      }
    }
  } else {
    return NIL();
  }
}

struct sexp find(struct sexp e, struct sexp env) {
  if (atom(env)) {
    return NIL();
  } else {
    struct sexp pair = fst(env);
    struct sexp key = fst(pair);
    if (strcmp(e.p, key.p)) {
      return find(e, snd(env));
    } else {
      return pair;
    }
  }
}

#ifdef UNITTEST_
#include <stdlib.h>

int main() {
  jmp_buf trap;

  if (!nil(eval(trap, NIL(), NIL()))) {
    return 1;
  }

  {
    struct sexp env = cons(cons(symbol("t"), symbol("True")), NIL());
    struct sexp val = eval(trap, symbol("t"), env);
    if (strcmp("True", val.p)) {
      return 2;
    }
  }

  {
    FILE* t = stderr;
    char* p;
    size_t n;
    stderr = open_memstream(&p, &n);

    switch (setjmp(trap)) {
    default:
      while (true) {
	struct sexp env = cons(cons(symbol("f"), symbol("False")), NIL());
	eval(trap, symbol("t"), env); /* symbol `t` not defined in env. */
      }
      break;
    case 1:
      fflush(stderr);
      if (strcmp("Value for symbol `t` not found.\n", p)) {
	return 3;
      }
      break;
    }

    free(p);
    fclose(stderr);
    stderr = t;
  }

  return 0;
}
#endif

