#include "ulisp.h"

struct sexp eval(const struct sexp exp, const struct sexp env) {
  return NIL();
}

#ifdef UNITTEST_
int main() {
  if (!nil(eval(NIL(), NIL()))) {
    return 1;
  }

  return 0;
}
#endif

