#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(a, b) (strcmp(a, b) == 0)

extern const char* name_of(const struct sexp* exp);

extern const struct sexp* make_applicable(const struct sexp* env, const struct sexp* params, const struct sexp* body);
extern const struct sexp* get_environment(jmp_buf trap, const struct sexp* exp);
extern const struct sexp* get_body(jmp_buf trap, const struct sexp* exp);
extern const struct sexp* get_params(jmp_buf trap, const struct sexp* exp);

static const char* Err_value_not_found = "Value for symbol `%s` not found.";
static const char* Err_illegal_argument = "Illegal argument: %s";
static const char* Err_value_not_pair = "`%s` is not pair.";

static const struct sexp* find(jmp_buf trap, const char* name, const struct sexp* env);
/* return car(cdr(exp)); throw TRAP_ILLARG if cdr(exp) is not pair. exp should be pair. */
static const struct sexp* cadr(jmp_buf trap, const struct sexp* exp);
/* return car(cdr(cdr(exp))); throw TRAP_ILLARG if cdr(exp) or cdr(cdr(exp)) is not pair. exp should be pair. */
static const struct sexp* caddr(jmp_buf trap, const struct sexp* exp);
/* helper for cadr, caddr. */
typedef const struct sexp* (*leaf_iterator)(const struct sexp*);
static const struct sexp* leaf(jmp_buf trap, const struct sexp* exp, leaf_iterator* fst_or_snd);
static const struct sexp* ensure_pair(jmp_buf trap, const struct sexp* exp);
static const struct env_exp cond(jmp_buf trap, const struct sexp* env, const struct sexp* cond_cdr);
static const struct env_exp closure(jmp_buf trap, const struct sexp* env, const struct sexp* exp);
static const struct env_exp apply(jmp_buf trap, const struct env_exp env_exp);
static const struct env_exp map_eval(jmp_buf trap, const struct env_exp env_exp);
static const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value);
static const struct sexp* zip(jmp_buf trap, const struct sexp* xs, const struct sexp* ys);
static const struct sexp* append_defs(const struct sexp* env, const struct sexp* def);

const struct env_exp eval(jmp_buf trap, const struct env_exp env_exp) {
    const struct sexp* env = env_exp.env;
    const struct sexp* exp = env_exp.exp;
    if (atom(exp)) {
        if (nil(exp)) {
            return (struct env_exp){ env, NIL() };
        } else {
            return (struct env_exp){ env, find(trap, name_of(exp), env) };
        }
    } else {
        /* exp is pair */
        const struct sexp* car = fst(exp);
        
        if (atom(car)) {
            if (STR_EQ("quote", name_of(car))) {
                return (struct env_exp){ env, cadr(trap, exp) };
            } else if (STR_EQ("cons", name_of(car))) {
                const struct env_exp head = eval(trap, (struct env_exp){ env, cadr(trap, exp) });
                const struct env_exp tail = eval(trap, (struct env_exp){ head.env, caddr(trap, exp) });
                return (struct env_exp){ tail.env, cons(head.exp, tail.exp) };
            } else if (STR_EQ("atom", name_of(car))) {
                const struct env_exp r = eval(trap, (struct env_exp){ env, cadr(trap, exp) });
                if (atom(r.exp)) {
                    return eval(trap, (struct env_exp){ r.env, symbol("t") });
                } else {
                    return (struct env_exp){ r.env, NIL() };
                }
            } else if (STR_EQ("car", name_of(car))) {
                const struct env_exp r = eval(trap, (struct env_exp){ env, cadr(trap, exp) });
                return (struct env_exp){ r.env, fst(ensure_pair(trap, r.exp)) };
            } else if (STR_EQ("cdr", name_of(car))) {
                const struct env_exp r = eval(trap, (struct env_exp){ env, cadr(trap, exp) });
                return (struct env_exp){ r.env, snd(ensure_pair(trap, r.exp)) };
            } else if (STR_EQ("set", name_of(car))) {
                const struct env_exp var = eval(trap, (struct env_exp){ env, cadr(trap, exp) });
                const struct env_exp val = eval(trap, (struct env_exp){ var.env, caddr(trap, exp) });
                const struct sexp* def = cons(var.exp, val.exp);
                return (struct env_exp){ cons(def, val.env), val.exp };
            } else if (STR_EQ("cond", name_of(car))) {
                cadr(trap, exp); // check at least one branch exist.
                return cond(trap, env, snd(exp));
            } else if (STR_EQ("lambda", name_of(car))) {
                return closure(trap, env, exp);
            } else {
                return apply(trap, env_exp);
            }
        } else {
            return apply(trap, env_exp);
        }
    }
}

const struct sexp* find(jmp_buf trap, const char* name, const struct sexp* env) {
    if (atom(env)) {
        fprintf(stderr, Err_value_not_found, name);
        fflush(stderr);
        longjmp(trap, TRAP_NOSYM);
    } else {
        const struct sexp* def = fst(env);
        if (STR_EQ(name, name_of(fst(def)))) {
            return snd(def); // value
        } else {
            return find(trap, name, snd(env));
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

const struct env_exp cond(jmp_buf trap, const struct sexp* env, const struct sexp* cond_cdr) {
    if (atom(cond_cdr)) {
        if (nil(cond_cdr)) {
            return (struct env_exp){ env, cond_cdr };
        } else {
            fprintf(stderr, Err_illegal_argument, text(cond_cdr));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        }
    } else {
        const struct sexp* branch = fst(ensure_pair(trap, cond_cdr));
        const struct env_exp pred = eval(trap, (struct env_exp){ env, fst(ensure_pair(trap, branch)) });
        if (nil(pred.exp)) {
            return cond(trap, pred.env, snd(cond_cdr));
        } else {
            return eval(trap, (struct env_exp){ pred.env, cadr(trap, branch) });
        }
    }
}

const struct env_exp closure(jmp_buf trap, const struct sexp* env, const struct sexp* exp) {
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
            return (struct env_exp){ env, make_applicable(env, param, body) };
        }
    }
}

const struct env_exp apply(jmp_buf trap, const struct env_exp env_exp) {
    const struct env_exp evaluated = map_eval(trap, env_exp);
    const struct sexp* env = evaluated.env;
    const struct sexp* exp = evaluated.exp;
    if (atom(exp)) {
        fprintf(stderr, Err_illegal_argument, text(env_exp.exp));
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        const struct sexp* func = fst(exp);
        const struct sexp* args = snd(exp);
        const struct sexp* closed_env = get_environment(trap, func);
        const struct sexp* pars = get_params(trap, func);
        const struct sexp* body = get_body(trap, func);
        jmp_buf trap2;
        if (setjmp(trap2)) {
            fprintf(stderr, ": %s v.s. %s", text(pars), text(args));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        } else {
            const struct sexp* es = append_defs(env, zip(trap, pars, args));
            return (struct env_exp){ env, fold_eval(trap, (struct env_exp){ es, body }, NIL()) };
        }
    }
}

const struct env_exp map_eval(jmp_buf trap, const struct env_exp env_exp) {
    const struct sexp* env = env_exp.env;
    const struct sexp* exp = env_exp.exp;
    if (atom(exp)) {
        return env_exp;
    } else {
        const struct env_exp r_car = eval(trap, (struct env_exp){ env, fst(exp) });
        const struct env_exp r_cdr = map_eval(trap, (struct env_exp){ r_car.env, snd(exp) });
        return (struct env_exp){ r_cdr.env, cons(r_car.exp, r_cdr.exp) };
    }
}

const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value) {
    const struct sexp* xs = env_xs.exp;
    if (atom(xs)) {
        return def_value;
    } else {
        struct env_exp evaled = eval(trap, (struct env_exp){ env_xs.env, fst(xs) });
        return fold_eval(trap, (struct env_exp){ evaled.env, snd(xs) }, evaled.exp);
    }
}

const struct sexp* zip(jmp_buf trap, const struct sexp* xs, const struct sexp* ys) {
    if (atom(xs) && atom(ys)) {
        return nil(xs) && nil(ys) ? NIL() : cons(xs, ys);
    } else {
        if (!atom(xs) && !atom(ys)) {
            return cons(cons(fst(xs), fst(ys)), zip(trap, snd(xs), snd(ys)));
        } else {
            fprintf(stderr, "List length mismatch.");
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
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
