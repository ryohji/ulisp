#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>

static const char* Err_value_not_found = "Value for symbol `%s` not found.\n";

struct sexp eval(jmp_buf trap, const struct sexp exp, const struct sexp env) {
  if (atom(exp)) {
    if (!nil(exp)) {
      fprintf(stderr, Err_value_not_found, exp.p);
      longjmp(trap, 1);
    }
    return NIL();
  } else {
    return NIL();
  }
}

#ifdef UNITTEST_
#include <stdlib.h>
#include <string.h>

int main() {
  jmp_buf trap;

  if (!nil(eval(trap, NIL(), NIL()))) {
    return 1;
  }

  {
    FILE* t = stderr;
    char* p;
    size_t n;
    stderr = open_memstream(&p, &n);

    switch (setjmp(trap)) {
    default:
      while(true) {
	eval(trap, symbol("t"), NIL());
      }
      break;
    case 1:
      fflush(stderr);
      if (strcmp("Value for symbol `t` not found.\n", p)) {
	return 2;
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

