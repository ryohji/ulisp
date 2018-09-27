#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const struct sexp* APPLICABLE();

static const char* Err_value_not_found = "Value for symbol `%s` not found.";
static const char* Err_illegal_argument = "Illegal argument: %s";
static const char* Err_value_not_pair = "`%s` is not pair.";

static const char* symbolOf(const struct sexp* exp);

static const struct sexp* find(jmp_buf trap, const struct sexp* e, const struct sexp* env);
/* return car(cdr(exp)); throw TRAP_ILLARG if cdr(exp) is not pair. exp should be pair. */
static const struct sexp* cadr(jmp_buf trap, const struct sexp* exp);
/* return car(cdr(cdr(exp))); throw TRAP_ILLARG if cdr(exp) or cdr(cdr(exp)) is not pair. exp should be pair. */
static const struct sexp* caddr(jmp_buf trap, const struct sexp* exp);
/* helper for cadr, caddr. */
typedef const struct sexp* (*leaf_iterator)(const struct sexp*);
static const struct sexp* leaf(jmp_buf trap, const struct sexp* exp, leaf_iterator* fst_or_snd);
static const struct sexp* ensure_pair(jmp_buf trap, const struct sexp* exp);
static const struct sexp* cond(jmp_buf trap, const struct sexp* env, const struct sexp* cond_cdr);
static const struct sexp* closure(jmp_buf trap, const struct sexp* env, const struct sexp* exp);
static const struct sexp* apply(jmp_buf trap, const struct sexp* env_exp);
static const struct sexp* map_eval(jmp_buf trap, const struct sexp* env_exp);
static const struct sexp* fold_eval(jmp_buf trap, const struct sexp* env, const struct sexp* init, const struct sexp* xs);
static const struct sexp* zip(jmp_buf trap, const struct sexp* xs, const struct sexp* ys);
static const struct sexp* append_defs(const struct sexp* env, const struct sexp* def);

const struct sexp* eval(jmp_buf trap, const struct sexp* env_exp) {
    const struct sexp* env = fst(env_exp);
    const struct sexp* exp = snd(env_exp);
    if (atom(exp)) {
        if (nil(exp)) {
            return cons(env, NIL());
        } else {
            return cons(env, find(trap, exp, env));
        }
    } else {
        /* exp is pair */
        const struct sexp* car = fst(exp);
        
        if (atom(car)) {
            const char* const pred = symbolOf(car);
            if (strcmp("quote", pred) == 0) {
                return cons(env, cadr(trap, exp));
            } else if (strcmp("cons", pred) == 0) {
                const struct sexp* head = eval(trap, cons(env, cadr(trap, exp)));
                const struct sexp* tail = eval(trap, cons(fst(head), caddr(trap, exp)));
                return cons(fst(tail), cons(snd(head), snd(tail)));
            } else if (strcmp("atom", pred) == 0) {
                const struct sexp* r = eval(trap, cons(env, cadr(trap, exp)));
                if (atom(snd(r))) {
                    return eval(trap, cons(fst(r), symbol("t")));
                } else {
                    return cons(fst(r), NIL());
                }
            } else if (strcmp("car", pred) == 0) {
                const struct sexp* r = eval(trap, cons(env, cadr(trap, exp)));
                return cons(fst(r), fst(ensure_pair(trap, snd(r))));
            } else if (strcmp("cdr", pred) == 0) {
                const struct sexp* r = eval(trap, cons(env, cadr(trap, exp)));
                return cons(fst(r), snd(ensure_pair(trap, snd(r))));
            } else if (strcmp("set", pred) == 0) {
                const struct sexp* var = eval(trap, cons(env, cadr(trap, exp)));
                const struct sexp* val = eval(trap, cons(fst(var), caddr(trap, exp)));
                const struct sexp* def = cons(snd(var), snd(val));
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

static inline const char* symbolOf(const struct sexp* exp) {
    return (const void*)((const int*)exp + 1);
}

const struct sexp* find(jmp_buf trap, const struct sexp* exp, const struct sexp* env) {
    if (atom(env)) {
        fprintf(stderr, Err_value_not_found, symbolOf(exp));
        fflush(stderr);
        longjmp(trap, TRAP_NOSYM);
    } else {
        const struct sexp* def = fst(env);
        const struct sexp* var = fst(def);
        if (strcmp(symbolOf(exp), symbolOf(var))) {
            return find(trap, exp, snd(env));
        } else {
            return snd(def); // value
        }
    }
}

const struct sexp* cadr(jmp_buf trap, const struct sexp* exp) {
    return leaf(trap, exp, (leaf_iterator[]) {snd, fst, 0});
}

const struct sexp* caddr(jmp_buf trap, const struct sexp* exp) {
    return leaf(trap, exp, (leaf_iterator[]) {snd, snd, fst, 0});
}

const struct sexp* leaf(jmp_buf trap, const struct sexp* exp, leaf_iterator* iter) {
    if (*iter) {
        const struct sexp* next = (*iter)(exp);
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

const struct sexp* ensure_pair(jmp_buf trap, const struct sexp* exp) {
    if (atom(exp)) {
        fprintf(stderr, Err_value_not_pair, text(exp));
        fflush(stderr);
        longjmp(trap, TRAP_NOTPAIR);
    } else {
        return exp;
    }
}

const struct sexp* cond(jmp_buf trap, const struct sexp* env, const struct sexp* cond_cdr) {
    if (atom(cond_cdr)) {
        if (nil(cond_cdr)) {
            return cons(env, cond_cdr);
        } else {
            fprintf(stderr, Err_illegal_argument, text(cond_cdr));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        }
    } else {
        const struct sexp* branch = fst(ensure_pair(trap, cond_cdr));
        const struct sexp* pred = eval(trap, cons(env, fst(ensure_pair(trap, branch))));
        if (nil(snd(pred))) {
            return cond(trap, fst(pred), snd(cond_cdr));
        } else {
            return eval(trap, cons(fst(pred), cadr(trap, branch)));
        }
    }
}

const struct sexp* closure(jmp_buf trap, const struct sexp* env, const struct sexp* exp) {
    const struct sexp* lambda_cdr = snd(exp);
    if (atom(lambda_cdr)) {
        fprintf(stderr, "No closure param exist: %s", text(exp));
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        const struct sexp* param = fst(lambda_cdr);
        const struct sexp* body = snd(lambda_cdr);
        if (atom(param) && !nil(param)) {
            fprintf(stderr, "Closure parameter should be list: %s", text(exp));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        } else {
            return cons(env, cons(APPLICABLE(), cons(param, body)));
        }
    }
}

const struct sexp* apply(jmp_buf trap, const struct sexp* env_exp) {
    const struct sexp* evaluated = map_eval(trap, env_exp);
    const struct sexp* env = fst(evaluated);
    const struct sexp* exp = snd(evaluated);
    if (atom(exp)) {
        fprintf(stderr, Err_illegal_argument, text(snd(env_exp)));
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        const struct sexp* func = fst(exp);
        const struct sexp* args = snd(exp);
        if (atom(func) || fst(func) != APPLICABLE()) {
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        } else {
            const struct sexp* pars = cadr(trap, func);
            const struct sexp* body = snd(snd(func));
            jmp_buf trap2;
            if (setjmp(trap2)) {
                fprintf(stderr, ": %s v.s. %s", text(pars), text(args));
                fflush(stderr);
                longjmp(trap, TRAP_ILLARG);
            } else {
                const struct sexp* es = append_defs(env, zip(trap, pars, args));
                return cons(env, fold_eval(trap, es, NIL(), body));
            }
        }
    }
}

const struct sexp* map_eval(jmp_buf trap, const struct sexp* env_exp) {
    const struct sexp* env = fst(env_exp);
    const struct sexp* exp = snd(env_exp);
    if (atom(exp)) {
        return env_exp;
    } else {
        const struct sexp* r_car = eval(trap, cons(env, fst(exp)));
        const struct sexp* r_cdr = map_eval(trap, cons(fst(r_car), snd(exp)));
        return cons(fst(r_cdr), cons(snd(r_car), snd(r_cdr)));
    }
}

const struct sexp* fold_eval(jmp_buf trap, const struct sexp* env, const struct sexp* init, const struct sexp* xs) {
    if (atom(xs)) {
        return init;
    } else {
        const struct sexp* car = fst(xs);
        const struct sexp* cdr = snd(xs);
        const struct sexp* r = eval(trap, cons(env, car));
        return fold_eval(trap, fst(r), snd(r), cdr);
    }
}

const struct sexp* zip(jmp_buf trap, const struct sexp* xs, const struct sexp* ys) {
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

const struct sexp* append_defs(const struct sexp* env, const struct sexp* def) {
    if (atom(def)) {
        return nil(def) ? env : cons(def, env);
    } else {
        return append_defs(cons(fst(def), env), snd(def));
    }
}
