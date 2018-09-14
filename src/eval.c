#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TRAP_CODE {
  TRAP_NONE,
  TRAP_NOSYM,
  TRAP_ILLARG,
};

static const char* Err_value_not_found = "Value for symbol `%s` not found.\n";
static const char* Err_illegal_argument = "Illegal argument: %s\n";

static struct sexp find(struct sexp e, struct sexp env);

struct sexp eval(jmp_buf trap, const struct sexp exp, const struct sexp env) {
  if (atom(exp)) {
    if (nil(exp)) {
      return NIL();
    } else {
      struct sexp pair = find(exp, env);
      if (nil(pair)) {
	fprintf(stderr, Err_value_not_found, exp.p);
	fflush(stderr);
	longjmp(trap, TRAP_NOSYM);
      } else {
	return snd(pair);
      }
    }
  } else {
    struct sexp pred = fst(exp);
    if (atom(pred)) {
      if (strcmp("quote", pred.p) == 0) {
	struct sexp cdr = snd(exp);
	if (atom(cdr)) {
	  char *p;
	  fprintf(stderr, Err_illegal_argument, (p = text(exp))); free(p);
	  fflush(stderr);
	  longjmp(trap, TRAP_ILLARG);
	} else {
	  return fst(cdr);
	}
      }
    }
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

  /* ATOM */
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
    case TRAP_NOSYM:
      if (strcmp("Value for symbol `t` not found.\n", p)) {
	return 3;
      }
      break;
    }

    fclose(stderr);
    free(p);
    stderr = t;
  }

  /* PAIR */
  { /* QUOTE */
    FILE* t = stderr;
    char* p;
    size_t n;

    {
      struct sexp v = eval(trap, cons(symbol("quote"), cons(symbol("ulisp"), NIL())), NIL());
      if (strcmp("ulisp", v.p)) {
	fprintf(t, "Failed to quote: %s\n", text(v));
	return 1;
      }
    }

    stderr = open_memstream(&p, &n);

    switch (setjmp(trap)) {
    default:
      while (true) {
	eval(trap, cons(symbol("quote"), NIL()), NIL());
      }
      break;
    case TRAP_ILLARG:
      if (strcmp("Illegal argument: (quote)\n", p)) {
	fprintf(t, "Trapped error message: %s\n", p);
	return 1;
      }
      break;
    }

    fclose(stderr);
    free(p);
    stderr = open_memstream(&p, &n);

    switch (setjmp(trap)) {
    default:
      while (true) {
	eval(trap, cons(symbol("quote"), symbol("ulisp")), NIL());
      }
      break;
    case TRAP_ILLARG:
      if (strcmp("Illegal argument: (quote: ulisp)\n", p)) {
	fprintf(t, "Trapped error message: %s\n", p);
	return 1;
      }
      break;
    }

    fclose(stderr);
    free(p);
    stderr = t;
  }

  return 0;
}
#endif

