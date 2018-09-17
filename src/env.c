#include "env.h"
#include "env-impl.h"

#include "ulisp.h"

#ifdef UNITTEST_
#include <stdio.h>
#include <string.h>

static char* dump(struct env* e);

int main() {
    struct env* const e = env_alloc();
    {
        struct sexp v;
        char* p;
        v = env_find(e, symbol("T"));
        if (!nil(v)) {
            printf("Undefined variable found. @%d\n", __LINE__);
            return 1;
        }

        env_push_front(e, symbol("T"), symbol("True"));
        p = dump(e);
        if (strcmp("1/2 (T: True)", p)) {
            printf("Can not pushed variable. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);

        env_push_front(e, symbol("F"), symbol("False"));
        p = dump(e);
        if (strcmp("2/2 (F: False) (T: True)", p)) {
            printf("Can not pushed variable. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);

        env_push_front(e, symbol("X"), symbol("1"));
        env_push_front(e, symbol("Y"), symbol("2"));
        p = dump(e);
        if (strcmp("4/4 (Y: 2) (X: 1) (F: False) (T: True)", p)) {
            printf("Can not pushed variable. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);

        env_push_front(e, symbol("Z"), symbol("4"));
        p = dump(e);
        if (strcmp("5/8 (Z: 4) (Y: 2) (X: 1) (F: False) (T: True)", p)) {
            printf("Can not pushed variable. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);

        v = env_find(e, symbol("T"));
        p = text(v);
        if (strcmp("(T: True)", p)) {
            printf("Defined variable not found. @%d\n", __LINE__);
            return 1;
        }
        free(p);

        env_push_front(e, symbol("T"), symbol("False")); // compromize
        p = text(env_find(e, symbol("T")));
        if (strcmp("(T: False)", p)) {
            printf("Defined variable not found. @%d\n", __LINE__);
            return 1;
        }
        free(p);

        env_clear(e);
        p = dump(e);
        if (strcmp("0/8", p)) {
            printf("Clear failed. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);

        v = env_find(e, symbol("T"));
        p = text(v);
        if (strcmp("()", p)) {
            printf("Failed to find unexist symbol. \"%s\" @%d\n", p, __LINE__);
            return 1;
        }
        free(p);
    }
    env_free(e);
    return 0;
}

static char* dump(struct env* e) {
    char* p;
    size_t n;
    struct sexp* const rend = e->list - 1;
    struct sexp* it = rend + e->elements;
    FILE* fp = open_memstream(&p, &n);
    fprintf(fp, "%d/%d", e->elements, e->capacity);
    while (it != rend) {
        fprintf(fp, " %s", text(*it));
        it -= 1;
    }
    fclose(fp);
    return p;
}

#endif
