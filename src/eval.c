#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TRAP_CODE {
  TRAP_NONE,
  TRAP_NOSYM,
  TRAP_ILLARG,
  TRAP_NOTPAIR,
};

static const char* Err_value_not_found = "Value for symbol `%s` not found.\n";
static const char* Err_illegal_argument = "Illegal argument: %s\n";
static const char* Err_value_not_pair = "`%s` is not pair.\n";

static struct sexp find(struct sexp e, struct sexp env);
/* return car(cdr(exp)); throw TRAP_ILLARG if cdr(exp) is not pair. exp should be pair. */
static struct sexp cadr(jmp_buf trap, struct sexp exp);

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
    /* exp is pair */
    struct sexp car = fst(exp);

    if (atom(car)) {
      const char* const pred = car.p;
      if (strcmp("quote", pred) == 0) {
        return cadr(trap, exp);
      } else if (strcmp("atom", pred) == 0) {
        if (atom(eval(trap, cadr(trap, exp), env))) {
          return eval(trap, symbol("t"), env);
        } else {
          return NIL();
        }
      } else if (strcmp("car", pred) == 0) {
        struct sexp v = eval(trap, cadr(trap, exp), env);
        if (atom(v)) {
          fprintf(stderr, Err_value_not_pair, text(v));
          fflush(stderr);
          longjmp(trap, TRAP_NOTPAIR);
        } else {
          return fst(v);
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

struct sexp cadr(jmp_buf trap, struct sexp exp) {
  struct sexp cdr = snd(exp);
  if (atom(cdr)) {
    char* p = text(exp);
    fprintf(stderr, Err_illegal_argument, p);
    fflush(stderr);
    free(p);
    longjmp(trap, TRAP_ILLARG);
  } else {
    return fst(cdr);
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
    FILE* const t = stderr;
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

    /* ATOM */
    {
      /* All predicate operator returns the value of "t" defined in environment */
      struct sexp v;
      struct sexp x;
      struct sexp env = cons(cons(symbol("t"), symbol("True")), NIL());

      /* (atom nil) ; => True. nil is atom. */
      x = cons(symbol("atom"), cons(NIL(), NIL()));
      v= eval(trap, x, env);
      if (strcmp("True", v.p)) {
        fprintf(t, "nil does not evaluated as True.: %s\n", text(x));
        return 1;
      }

      /* (atom (quote ulisp)) ; => True. quote generate symbol. */
      x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(symbol("ulisp"), NIL())), NIL()));
      v = eval(trap, x, env);
      if (strcmp("True", v.p)) {
        fprintf(t, "symbol does not evaluated as True.: %s\n", text(x));
        return 1;
      }

      /* (atom (quote (ulisp))) ; => nil. pair/list is not atom. */
      x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(cons(symbol("ulisp"), NIL()), NIL())), NIL()));
      v = eval(trap, x, env);
      if (!nil(v)) {
        fprintf(t, "list/pair does not evaluated as nil.: %s\n", text(x));
        return 1;
      }

      /* Error thrown if no argument specified. */
      stderr = open_memstream(&p, &n);
      switch (setjmp(trap)) {
      default:
        eval(trap, cons(symbol("atom"), NIL()), NIL());
        fprintf(t, "NOT REACHED HERE.\n");
        return 1;
      case TRAP_ILLARG:
        if (strcmp("Illegal argument: (atom)\n", p)) {
          fprintf(t, "Error message does not match: %s\n", p);
          return 1;
        }
        break;
      }
      fclose(stderr);
      free(p);

      /* Error TRAP_NOSYM thrown if global envitonment does not hold specified symbol. */
      stderr = open_memstream(&p, &n);
      switch (setjmp(trap)) {
      default:
        eval(trap, cons(symbol("atom"), cons(symbol("ulisp"), NIL())), env);
        fprintf(t, "NOT REACHED HERE.\n");
        return 1;
      case TRAP_NOSYM:
        if (strcmp("Value for symbol `ulisp` not found.\n", p)) {
          fprintf(t, "Error message mismatch: %s\n", p);
          return 1;
        }
        break;
      }
      fclose(stderr);
      free(p);

      /* So, if global environment holds specified symbol matching atom, eval `(atom ulisp)` returns the value of `t`. */
      stderr = open_memstream(&p, &n);
      switch (setjmp(trap)) {
      default:
        v = eval(trap, cons(symbol("atom"), cons(symbol("ulisp"), NIL())), cons(cons(symbol("ulisp"), symbol("ULISP")), env));
        if (strcmp("True", v.p)) {
          fprintf(t, "Global symbol search failed: %s\n", text(v));
          return 1;
        }

        /* And if mapped global symbol holds pair, eval `(atom ulips)` returns nil. */
        v = eval(trap, cons(symbol("atom"), cons(symbol("ulisp"), NIL())), cons(cons(symbol("ulisp"), cons(symbol("ULISP"), NIL())), env));
        if (!nil(v)) {
          fprintf(t, "Global symbol search failed: %s\n", text(v));
          return 1;
        }
        break;
      case TRAP_NOSYM:
        fprintf(t, "NOT REACHED HERE.\n");
        return 1;
      }
      fclose(stderr);
      free(p);
    }

    stderr = t;
  }

  { /* CAR */
    FILE* const t = stderr;
    char* p;
    size_t n;
    struct sexp v;

    /* (car (quote (x: 1))) ; => x */
    switch (setjmp(trap)) {
    case TRAP_NONE:
      v = eval(trap, cons(symbol("car"), cons(cons(symbol("quote"), cons(cons(symbol("x"), symbol("1")), NIL())), NIL())), NIL());
      if (strcmp("x", v.p)) {
        fprintf(t, "Failed to extract car: %s\n", text(v));
        return 1;
      }
      break;
    default:
      fprintf(t, "NOT REACHED HERE.\n");
      return 1;
    }

    /* (car X) with env {(X: (x: 1)}} ; => x */
    switch (setjmp(trap)) {
    case TRAP_NONE:
      v = eval(trap, cons(symbol("car"), cons(symbol("X"), NIL())), cons(cons(symbol("X"), cons(symbol("x"), symbol("1"))), NIL()));
      if (strcmp("x", v.p)) {
        fprintf(t, "Failed to extract car: %s\n", text(v));
        return 1;
      }
      break;
    default:
      fprintf(t, "NOT REACHED HERE.\n");
      return 1;
    }

    /* (car) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
    default:
      eval(trap, cons(symbol("car"), NIL()), NIL());
      fprintf(t, "NOT REACHED HERE.\n");
      return 1;
    case TRAP_ILLARG:
      if (strcmp("Illegal argument: (car)\n", p)) {
        fprintf(t, "Error message mismatch: %s\n", p);
        return 1;
      }
      break;
    }
    fclose(stderr);
    free(p);

    /* (car nil) */
    stderr = open_memstream(&p, &n);
    switch (setjmp(trap)) {
    default:
      eval(trap, cons(symbol("car"), cons(NIL(), NIL())), NIL());
      fprintf(t, "NOT REACHED HERE.\n");
      return 1;
    case TRAP_NOTPAIR:
      if (strcmp("`()` is not pair.\n", p)) {
        puts(p);
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

