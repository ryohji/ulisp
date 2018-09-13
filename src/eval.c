#include "ulisp.h"

#include <setjmp.h>

static jmp_buf CALL_CONTEXT;

#define trap()       setjmp(CALL_CONTEXT)
#define throw(cause) longjmp(CALL_CONTEXT, (cause))

struct sexp eval(const struct sexp exp, const struct sexp env) {
  if (atom(exp)) {
    if (!nil(exp)) {
      throw(1);
    }
    return NIL();
  } else {
    return NIL();
  }
}

#ifdef UNITTEST_
int main() {
  if (!nil(eval(NIL(), NIL()))) {
    return 1;
  }

  switch (trap()) {
  case 0:
    while (true) {
      eval(symbol("t"), NIL());
    }
    break;
  default:
    break;
  }

  return 0;
}
#endif

