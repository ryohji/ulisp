#include "ulisp.h"

#include <setjmp.h>
#include <stdbool.h>

struct sexp eval(jmp_buf trap, const struct sexp exp, const struct sexp env) {
  if (atom(exp)) {
    if (!nil(exp)) {
      longjmp(trap, 1);
    }
    return NIL();
  } else {
    return NIL();
  }
}

#ifdef UNITTEST_
int main() {
  jmp_buf trap;

  if (!nil(eval(trap, NIL(), NIL()))) {
    return 1;
  }

  switch (setjmp(trap)) {
  default:
    while(true) {
      eval(trap, symbol("t"), NIL());
    }
    break;
  case 1:
    break;
  }

  return 0;
}
#endif

