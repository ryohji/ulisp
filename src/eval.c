#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum TRAPCODE {
  TRAP_NONE,
  TRAP_NOSYM,
  TRAP_ILLARG,
  TRAP_NOTPAIR,
};

static const char* Err_value_not_found = "Value for symbol `%s` not found.";
static const char* Err_illegal_argument = "Illegal argument: %s";
static const char* Err_value_not_pair = "`%s` is not pair.";

static struct sexp find(jmp_buf trap, struct sexp e, struct sexp env);
/* return car(cdr(exp)); throw TRAP_ILLARG if cdr(exp) is not pair. exp should be pair. */
static struct sexp cadr(jmp_buf trap, struct sexp exp);
/* return car(cdr(cdr(exp))); throw TRAP_ILLARG if cdr(exp) or cdr(cdr(exp)) is not pair. exp should be pair. */
static struct sexp caddr(jmp_buf trap, struct sexp exp);
/* helper for cadr, caddr. */
typedef struct sexp (*leaf_iterator)(const struct sexp);
static struct sexp leaf(jmp_buf trap, struct sexp exp, leaf_iterator* fst_or_snd);
static struct sexp ensure_pair(jmp_buf trap, struct sexp exp);
static struct sexp cond(jmp_buf trap, struct sexp env, struct sexp cond_cdr);
static struct sexp APPLICABLE();
static struct sexp closure(jmp_buf trap, struct sexp env, struct sexp exp);
static struct sexp apply(jmp_buf trap, struct sexp env_exp);
static struct sexp map_eval(jmp_buf trap, struct sexp env_exp);
static struct sexp fold_eval(jmp_buf trap, struct sexp env, struct sexp init, struct sexp xs);
static struct sexp zip(jmp_buf trap, struct sexp xs, struct sexp ys);
static struct sexp append_defs(struct sexp env, struct sexp def);

struct sexp eval(jmp_buf trap, const struct sexp env_exp) {
  struct sexp env = fst(env_exp);
  struct sexp exp = snd(env_exp);
  if (atom(exp)) {
    if (nil(exp)) {
      return cons(env, NIL());
    } else {
      return cons(env, find(trap, exp, env));
    }
  } else {
    /* exp is pair */
    struct sexp car = fst(exp);

    if (atom(car)) {
      const char* const pred = car.p;
      if (strcmp("quote", pred) == 0) {
        return cons(env, cadr(trap, exp));
      } else if (strcmp("cons", pred) == 0) {
        struct sexp head = eval(trap, cons(env, cadr(trap, exp)));
        struct sexp tail = eval(trap, cons(fst(head), caddr(trap, exp)));
        return cons(fst(tail), cons(snd(head), snd(tail)));
      } else if (strcmp("atom", pred) == 0) {
        struct sexp r = eval(trap, cons(env, cadr(trap, exp)));
        if (atom(snd(r))) {
          return eval(trap, cons(fst(r), symbol("t")));
        } else {
          return cons(fst(r), NIL());
        }
      } else if (strcmp("car", pred) == 0) {
        struct sexp r = eval(trap, cons(env, cadr(trap, exp)));
        return cons(fst(r), fst(ensure_pair(trap, snd(r))));
      } else if (strcmp("cdr", pred) == 0) {
        struct sexp r = eval(trap, cons(env, cadr(trap, exp)));
        return cons(fst(r), snd(ensure_pair(trap, snd(r))));
      } else if (strcmp("set", pred) == 0) {
        struct sexp var = eval(trap, cons(env, cadr(trap, exp)));
        struct sexp val = eval(trap, cons(fst(var), caddr(trap, exp)));
        struct sexp def = cons(snd(var), snd(val));
        return cons(cons(def, fst(val)), snd(val));
      } else if (strcmp("cond", pred) == 0) {
        cadr(trap, exp); // check at least one branch exist.
        return cond(trap, env, snd(exp));
      } else if (strcmp("lambda", pred) == 0) {
        return closure(trap, env, exp);
      } else {
        return apply(trap, env_exp);
      }
    } else {
      return apply(trap, env_exp);
    }
  }
}

struct sexp find(jmp_buf trap, struct sexp exp, struct sexp env) {
  if (atom(env)) {
    fprintf(stderr, Err_value_not_found, exp.p);
    fflush(stderr);
    longjmp(trap, TRAP_NOSYM);
  } else {
    struct sexp def = fst(env);
    struct sexp var = fst(def);
    if (strcmp(exp.p, var.p)) {
      return find(trap, exp, snd(env));
    } else {
      return snd(def); // value
    }
  }
}

struct sexp cadr(jmp_buf trap, struct sexp exp) {
  return leaf(trap, exp, (leaf_iterator[]) {snd, fst, 0});
}

struct sexp caddr(jmp_buf trap, struct sexp exp) {
  return leaf(trap, exp, (leaf_iterator[]) {snd, snd, fst, 0});
}

struct sexp leaf(jmp_buf trap, struct sexp exp, leaf_iterator* iter) {
  if (*iter) {
    struct sexp next = (*iter)(exp);
    if (*(iter + 1) && atom(next)) {
      char* p = text(exp);
      fprintf(stderr, Err_illegal_argument, p);
      fflush(stderr);
      free(p);
      longjmp(trap, TRAP_ILLARG);
    } else {
      return leaf(trap, next, iter + 1);
    }
  } else {
    return exp;
  }
}

struct sexp ensure_pair(jmp_buf trap, struct sexp exp) {
  if (atom(exp)) {
    fprintf(stderr, Err_value_not_pair, text(exp));
    fflush(stderr);
    longjmp(trap, TRAP_NOTPAIR);
  } else {
    return exp;
  }
}

struct sexp cond(jmp_buf trap, struct sexp env, struct sexp cond_cdr) {
  if (atom(cond_cdr)) {
    if (nil(cond_cdr)) {
      return cons(env, cond_cdr);
    } else {
      fprintf(stderr, Err_illegal_argument, text(cond_cdr));
      fflush(stderr);
      longjmp(trap, TRAP_ILLARG);
    }
  } else {
    struct sexp branch = fst(ensure_pair(trap, cond_cdr));
    struct sexp pred = eval(trap, cons(env, fst(ensure_pair(trap, branch))));
    if (nil(snd(pred))) {
      return cond(trap, fst(pred), snd(cond_cdr));
    } else {
      return eval(trap, cons(fst(pred), cadr(trap, branch)));
    }
  }
}

struct sexp APPLICABLE() {
  static struct sexp marker = { .symbol = true, .p = "*applicable*", };
  return marker;
}

struct sexp closure(jmp_buf trap, struct sexp env, struct sexp exp) {
  struct sexp lambda_cdr = snd(exp);
  if (atom(lambda_cdr)) {
    fprintf(stderr, "No closure param exist: %s", text(exp));
    fflush(stderr);
    longjmp(trap, TRAP_ILLARG);
  } else {
    struct sexp param = fst(lambda_cdr);
    struct sexp body = snd(lambda_cdr);
    if (atom(param) && !nil(param)) {
      fprintf(stderr, "Closure parameter should be list: %s", text(exp));
      fflush(stderr);
      longjmp(trap, TRAP_ILLARG);
    } else {
      return cons(env, cons(APPLICABLE(), cons(param, body)));
    }
  }
}

struct sexp apply(jmp_buf trap, struct sexp env_exp) {
  struct sexp evaluated = map_eval(trap, env_exp);
  struct sexp env = fst(evaluated);
  struct sexp exp = snd(evaluated);
  if (atom(exp)) {
    fprintf(stderr, Err_illegal_argument, text(snd(env_exp)));
    fflush(stderr);
    longjmp(trap, TRAP_ILLARG);
  } else {
    struct sexp func = fst(exp);
    struct sexp args = snd(exp);
    if (atom(func) || fst(func).p != APPLICABLE().p) {
      fflush(stderr);
      longjmp(trap, TRAP_ILLARG);
    } else {
      struct sexp pars = cadr(trap, func);
      struct sexp body = snd(snd(func));
      jmp_buf trap2;
      if (setjmp(trap2)) {
        fprintf(stderr, ": %s v.s. %s", text(pars), text(args));
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
      } else {
        struct sexp es = append_defs(env, zip(trap, pars, args));
        return cons(env, fold_eval(trap, es, NIL(), body));
      }
    }
  }
}

struct sexp map_eval(jmp_buf trap, struct sexp env_exp) {
  struct sexp env = fst(env_exp);
  struct sexp exp = snd(env_exp);
  if (atom(exp)) {
    return env_exp;
  } else {
    struct sexp r_car = eval(trap, cons(env, fst(exp)));
    struct sexp r_cdr = map_eval(trap, cons(fst(r_car), snd(exp)));
    return cons(fst(r_cdr), cons(snd(r_car), snd(r_cdr)));
  }
}

struct sexp fold_eval(jmp_buf trap, struct sexp env, struct sexp init, struct sexp xs) {
  if (atom(xs)) {
    return init;
  } else {
    struct sexp car = fst(xs);
    struct sexp cdr = snd(xs);
    struct sexp r = eval(trap, cons(env, car));
    return fold_eval(trap, fst(r), snd(r), cdr);
  }
}

struct sexp zip(jmp_buf trap, struct sexp xs, struct sexp ys) {
  if (atom(xs)) {
    if (!atom(ys)) {
      fprintf(stderr, "List length mismatch.");
      fflush(stderr);
      longjmp(trap, TRAP_ILLARG);
    } else {
      return nil(xs) && nil(ys) ? NIL() : cons(xs, ys);
    }
  } else {
    if (atom(ys)) {
      fprintf(stderr, "List length mismatch.");
      fflush(stderr);
      longjmp(trap, TRAP_ILLARG);
    } else {
      return cons(cons(fst(xs), fst(ys)), zip(trap, snd(xs), snd(ys)));
    }
  }
}

struct sexp append_defs(struct sexp env, struct sexp def) {
  if (atom(def)) {
    return nil(def) ? env : cons(def, env);
  } else {
    return append_defs(cons(fst(def), env), snd(def));
  }
}


#ifdef UNITTEST_
#include <stdlib.h>

#define ASSERT_EQ(expect, actual) if (strcmp(expect, actual)) { printf("expect: %s\n""actual: %s\n""@%d\n", expect, actual, __LINE__); ng += 1; } else { ok += 1; }
#define NOT_REACHED_HERE() { printf("NOT REACHED HERE.\n@%d\n", __LINE__); ng += 1; }

static struct sexp LIST(unsigned numElems, ...);

int main() {
  unsigned ok = 0, ng = 0;
  jmp_buf trap;
  FILE* const fp = stderr;
  struct sexp x; // expression to test.
  struct sexp r; // result, (env: evaluated) pair.
  struct sexp env = cons(cons(symbol("t"), symbol("True")), NIL());
  char* p = 0;
  size_t n;

  /* ATOM */
  r =  eval(trap, cons(NIL(), NIL()));
  ASSERT_EQ("(())", text(r));

  r = eval(trap, cons(env, symbol("t")));
  ASSERT_EQ("(((t: True)): True)", text(r));

  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), symbol("t"))); /* symbol `t` not defined in env. */
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOSYM:
    ASSERT_EQ("Value for symbol `t` not found.", p);
    break;
  }
  fclose(stderr);
  free(p);

  r = eval(trap, cons(NIL(), cons(symbol("quote"), cons(symbol("ulisp"), NIL()))));
  ASSERT_EQ("((): ulisp)", text(r));

  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("quote"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (quote)", p);
    break;
  }
  fclose(stderr);
  free(p);

  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("quote"), symbol("ulisp"))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (quote: ulisp)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (atom nil) ; => True. nil is atom. */
  x = cons(symbol("atom"), cons(NIL(), NIL()));
  r = eval(trap, cons(env, x));
  ASSERT_EQ("(((t: True)): True)", text(r));

  /* (atom (quote ulisp)) ; => True. quote generate symbol. */
  x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(symbol("ulisp"), NIL())), NIL()));
  r = eval(trap, cons(env, x));
  ASSERT_EQ("(((t: True)): True)", text(r));

  /* (atom (quote (ulisp))) ; => nil. pair/list is not atom. */
  x = cons(symbol("atom"), cons(cons(symbol("quote"), cons(cons(symbol("ulisp"), NIL()), NIL())), NIL()));
  r = eval(trap, cons(env, x));
  ASSERT_EQ("(((t: True)))", text(r));

  /* Error thrown if no argument specified. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("atom"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (atom)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* Error TRAP_NOSYM thrown if global envitonment does not hold specified symbol. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(env, cons(symbol("atom"), cons(symbol("ulisp"), NIL()))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOSYM:
    ASSERT_EQ("Value for symbol `ulisp` not found.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* So, if global environment holds specified symbol matching atom, eval `(atom ulisp)` returns the value of `t`. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(cons(cons(symbol("ulisp"), symbol("ULISP")), env), cons(symbol("atom"), cons(symbol("ulisp"), NIL()))));
    ASSERT_EQ("(((ulisp: ULISP) (t: True)): True)", text(r));
    /* And if mapped global symbol holds pair, eval `(atom ulips)` returns nil. */
    r = eval(trap, cons(cons(cons(symbol("ulisp"), cons(symbol("ULISP"), NIL())), env), cons(symbol("atom"), cons(symbol("ulisp"), NIL()))));
    ASSERT_EQ("(((ulisp ULISP) (t: True)))", text(r));
    break;
  default:
    NOT_REACHED_HERE();
    break;
  }
  fclose(stderr);
  free(p);

  /* (cons) throws TRAP_ILLAREG. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("cons"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (cons)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cons hello) throws TRAP_NOSYM, therefore not defined `hello`. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("cons"), cons(symbol("hello"), NIL()))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOSYM:
    ASSERT_EQ("Value for symbol `hello` not found.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cons hello)w/{(hello: nil)} throws TRAP_ILLARG, therefore cdr part not exist. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(cons(cons(symbol("hello"), NIL()), NIL()), cons(symbol("cons"), cons(symbol("hello"), NIL()))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    /* XXX: this error message is confusing. it means that there is no cdr part exist in `(hello)`. */
    ASSERT_EQ("Illegal argument: (hello)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cons hello world)w/{(hello: nil)} throws TRAP_NOSYM, therefore no definition `world`. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(cons(cons(symbol("hello"), NIL()), NIL()), cons(symbol("cons"), cons(symbol("hello"), cons(symbol("world"), NIL())))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOSYM:
    ASSERT_EQ("Value for symbol `world` not found.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cons hello (quote world))w/{(hello: nil)} ; => (nil: world) */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(cons(cons(symbol("hello"), NIL()), NIL()),
      cons(symbol("cons"), cons(symbol("hello"), cons(cons(symbol("quote"), cons(symbol("world"), NIL())), NIL())))));
    ASSERT_EQ("(((hello)) (): world)", (p = text(r)));
    break;
  default:
    NOT_REACHED_HERE();
    break;
  }
  fclose(stderr);
  free(p);

  /* (car (quote (x: 1))) ; => x */
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(NIL(), cons(symbol("car"), cons(cons(symbol("quote"), cons(cons(symbol("x"), symbol("1")), NIL())), NIL()))));
    ASSERT_EQ("((): x)", (p = text(r)));
    break;
  default:
    NOT_REACHED_HERE();
    break;
  }

  /* (car X) with env {(X: (x: 1)}} ; => x */
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(cons(cons(symbol("X"), cons(symbol("x"), symbol("1"))), NIL()),
      cons(symbol("car"), cons(symbol("X"), NIL()))));
    ASSERT_EQ("(((X x: 1)): x)", (p = text(r)));
    break;
  default:
    NOT_REACHED_HERE();
    break;
  }

  /* (car) */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("car"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (car)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (car nil) */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("car"), cons(NIL(), NIL()))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOTPAIR:
    ASSERT_EQ("`()` is not pair.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cdr (quote (x: 1))) ; => x */
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(NIL(),
      cons(symbol("cdr"), cons(cons(symbol("quote"), cons(cons(symbol("x"), symbol("1")), NIL())), NIL()))));
    ASSERT_EQ("((): 1)", (p = text(r)));
    break;
  default:
    NOT_REACHED_HERE();
    break;
  }

  /* (cdr X) with env {(X: (x: 1)}} ; => x */
  switch (setjmp(trap)) {
  case TRAP_NONE:
    r = eval(trap, cons(cons(cons(symbol("X"), cons(symbol("x"), symbol("1"))), NIL()), cons(symbol("cdr"), cons(symbol("X"), NIL()))));
    ASSERT_EQ("(((X x: 1)): 1)", (p = text(r)));
    break;
  default:
    NOT_REACHED_HERE();
    return 1;
  }

  /* (cdr) */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("cdr"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (cdr)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cdr nil) */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("cdr"), cons(NIL(), NIL()))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOTPAIR:
    ASSERT_EQ("`()` is not pair.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (set (quote re) (quote ulisp)) ; => t */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(3, symbol("set"), LIST(2, symbol("quote"), symbol("re")), LIST(2, symbol("quote"), symbol("ulisp")));
    r = eval(trap, cons(NIL(), x));
    /* environment expanded to hold (re: ulisp), and returned evaluated (assigned) value. */
    ASSERT_EQ("(((re: ulisp)): ulisp)", (p = text(r)));
    free(p);
  }

  /* (cond) throws ILLARG.  */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), LIST(1, symbol("cond"))));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Illegal argument: (cond)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cond ()) throws NOTPAIR because () has no predicate. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    x = LIST(2, symbol("cond"), NIL());
    eval(trap, cons(NIL(), x));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_NOTPAIR:
    ASSERT_EQ("`()` is not pair.", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (cond (nil)) ; => nil */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(2, symbol("cond"), LIST(1, NIL()));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("(())", text(r)); // ((): ())
  }

  /* (cond ('t 'hello)) ; => hello */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(2, symbol("cond"), LIST(2, LIST(2, symbol("quote"), symbol("t")), LIST(2, symbol("quote"), symbol("hello"))));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("((): hello)", text(r));
  }

  /* (cond ((set 'x nil)) ('t 'hello)) ; => hello, environment expanded. */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(3, symbol("cond"),
      LIST(1, LIST(3, symbol("set"), LIST(2, symbol("quote"), symbol("x")), NIL())),
      LIST(2, symbol("t"), LIST(2, symbol("quote"), symbol("hello"))));
    r = eval(trap, cons(env, x));
    ASSERT_EQ("(((x) (t: True)): hello)", text(r));
  }

  /* (lambda) throws ILLARG. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    eval(trap, cons(NIL(), cons(symbol("lambda"), NIL())));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("No closure param exist: (lambda)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (lambda ATOM) throws ILLARG. */
  stderr = open_memstream(&p, &n);
  switch (setjmp(trap)) {
  case TRAP_NONE:
    x = cons(symbol("lambda"), cons(symbol("ATOM"), NIL()));
    r = eval(trap, cons(env, x));
    /* $FALL-THROUGH$ */
  default:
    NOT_REACHED_HERE();
    break;
  case TRAP_ILLARG:
    ASSERT_EQ("Closure parameter should be list: (lambda ATOM)", p);
    break;
  }
  fclose(stderr);
  free(p);

  /* (lambda params body) returns closure: (env: (*applicable*: (param: body))). */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    r = eval(trap, cons(NIL(), cons(symbol("lambda"), cons(NIL(), NIL()))));
    ASSERT_EQ("(() *applicable* ())", (p = text(r))); // ((): (*applicable*: ((): ())))
    free(p);

    r = eval(trap, cons(env, cons(symbol("lambda"), cons(cons(NIL(), LIST(3, symbol("set"), symbol("t"), symbol("False"))), NIL()))));
    ASSERT_EQ("(((t: True)) *applicable* (() set t False))", (p = text(r)));
    free(p);
  }

  /* ((lambda ())) ; => nil */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(1, LIST(2, symbol("lambda"), NIL()));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("(())", text(r)); // ((): ())
  }

  /* ((lambda () (quote ulisp))) ; => ulisp */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(1, LIST(3, symbol("lambda"), NIL(), LIST(2, symbol("quote"), symbol("ulisp"))));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("((): ulisp)", text(r));
  }

  /* ((lambda (x) x) (quote ulisp)) ; => ulisp */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(1, LIST(3, symbol("lambda"), NIL(), LIST(2, symbol("quote"), symbol("ulisp"))));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("((): ulisp)", text(r));
  }

  /* ((lambda (x) x) (quote ulisp)) ; => ulisp */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(2, LIST(3, symbol("lambda"), LIST(1, symbol("x")), symbol("x")), LIST(2, symbol("quote"), symbol("ulisp")));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("((): ulisp)", text(r));
  }

  /* ((lambda (x) x) (quote (1 2 3))) ; => (1 2 3) */
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
  } else {
    x = LIST(2, LIST(3, symbol("lambda"), LIST(1, symbol("x")), symbol("x")), LIST(2, symbol("quote"), LIST(3, symbol("1"), symbol("2"), symbol("3"))));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("(() 1 2 3)", text(r)); // ((): (1 2 3)) ; => (() 1 2 3)
  }

  /* ((lambda (x y) (cons y (cons x nil))) (quote world) (quote hello)) ; => (hello world) */
  stderr = open_memstream(&p, &n);
  if (setjmp(trap)) {
    NOT_REACHED_HERE();
    puts(p);
  } else {
    x = LIST(3, LIST(3, symbol("lambda"), LIST(2, symbol("x"), symbol("y")), LIST(3, symbol("cons"), symbol("y"), LIST(3, symbol("cons"), symbol("x"), NIL()))),
     LIST(2, symbol("quote"), symbol("world")), LIST(2, symbol("quote"), symbol("hello")));
    r = eval(trap, cons(NIL(), x));
    ASSERT_EQ("(() hello world)", text(r)); // ((): (hello world)) ; => (() hello world)
  }
  fclose(stderr);
  free(p);

  stderr = fp;
  printf("total %d run, NG = %d\n", ok + ng, ng);

  return ng;
}

#include <stdarg.h>
struct sexp LIST(unsigned numElems, ...) {
  struct sexp* buf = malloc(sizeof(struct sexp) * numElems);
  unsigned i;
  va_list exps;
  va_start(exps, numElems);
  for (i = 0; i < numElems; ++i) {
    struct sexp e = va_arg(exps, struct sexp);
    memcpy(buf + i, &e, sizeof(e));
  }
  va_end(exps);
  {
    struct sexp e = NIL();
    while (numElems--) {
      struct sexp p = cons(buf[numElems], e);
      memcpy(&e, &p, sizeof(p));
    }
    return e;
  }
}

#endif
