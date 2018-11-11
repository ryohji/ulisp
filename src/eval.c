#include "ulisp.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STR_EQ(a, b) (strcmp(a, b) == 0)

extern const char* name_of(const struct sexp* exp);

extern FILE* fdup(FILE* stream, const char* mode);

extern const struct sexp* make_applicable(struct env* env, const struct sexp* params, const struct sexp* body);
extern struct env* get_environment(jmp_buf trap, const struct sexp* exp);
extern const struct sexp* get_body(jmp_buf trap, const struct sexp* exp);
extern const struct sexp* get_params(jmp_buf trap, const struct sexp* exp);

struct print_context;

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
static const struct env_exp cond(jmp_buf trap, struct env* env, const struct sexp* cond_cdr, struct print_context* print_context);
static const struct env_exp closure(jmp_buf trap, struct env* env, const struct sexp* exp);
static const struct env_exp apply(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct env_exp map_eval(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value, struct print_context* print_context);
static const struct sexp* zip(jmp_buf trap, const struct sexp* xs, const struct sexp* ys);
static void append_defs(struct env* env, const struct sexp* def);
static const struct env_exp eval_impl(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct env_exp eval_core(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);

struct print_context {
    unsigned call_depth;
    FILE* verbose_eval;
};

static void print_nest(struct print_context* print_context) {
    unsigned depth = print_context->call_depth;
    while (depth--) {
        fprintf(print_context->verbose_eval, "| ");
    }
}

static void ennest(struct print_context* print_context) {
    print_context->call_depth += 1;
}

static void unnest(struct print_context* print_context) {
    print_context->call_depth -= 1;
}

static void print_env(struct env* env, struct print_context* print_context) {
    const struct env_iterator* it = env_it_begin(env);
    const struct env_iterator* const end = env_it_end(env);
    while (it != end) {
        print_nest(print_context);
        fprintf(print_context->verbose_eval, " env.%s=%s\n", env_it_name(it), text(env_it_value(it)));
        it = env_it_next(it);
    }
}

static FILE* file_of_verbose_eval(struct env* env) {
    bool verbose;

    jmp_buf trap;
    FILE* const tmp = stderr;
    char *p;
    size_t n;
    stderr = open_memstream(&p, &n);

    if (setjmp(trap)) {
        verbose = false;
    } else {
        verbose = env_search(env, "*verbose-eval*") != NIL();
    }

    fclose(stderr);
    free(p);
    stderr = tmp;

    if (verbose) {
        return fdup(stdout, "w");
    } else {
        return fopen("/dev/null", "w");
    }
}

const struct env_exp eval(jmp_buf trap, const struct env_exp env_exp) {
    struct print_context print_context = {
        .call_depth = 0,
        .verbose_eval = file_of_verbose_eval(env_exp.env),
    };
    struct env_exp result = eval_impl(trap, env_exp, &print_context);
    fclose(print_context.verbose_eval);
    return result;
}

const struct env_exp eval_impl(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    print_nest(print_context);
    fprintf(print_context->verbose_eval, "EVALUATE: %s\n", text(env_exp.exp));
    ennest(print_context);
    print_env(env_exp.env, print_context);

    struct env_exp result = eval_core(trap, env_exp, print_context);

    unnest(print_context);
    print_nest(print_context);
    fprintf(print_context->verbose_eval, "\\___ %s\n", text(result.exp));
    return result;
}


static const struct env_exp eval_core(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    struct env* env = env_exp.env;
    const struct sexp* exp = env_exp.exp;
    if (atom(exp)) {
        if (nil(exp)) {
            return (struct env_exp){ env, NIL() };
        } else {
            return (struct env_exp){ env, env_search(env, name_of(exp)) };
        }
    } else {
        /* exp is pair */
        const struct sexp* car = fst(exp);
        
        if (atom(car)) {
            if (STR_EQ("quote", name_of(car))) {
                return (struct env_exp){ env, cadr(trap, exp) };
            } else if (STR_EQ("cons", name_of(car))) {
                const struct env_exp head = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                const struct env_exp tail = eval_impl(trap, (struct env_exp){ env, caddr(trap, exp) }, print_context);
                return (struct env_exp){ env, cons(head.exp, tail.exp) };
            } else if (STR_EQ("atom", name_of(car))) {
                const struct env_exp r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                if (atom(r.exp)) {
                    return eval_impl(trap, (struct env_exp){ env, symbol("t") }, print_context);
                } else {
                    return (struct env_exp){ env, NIL() };
                }
            } else if (STR_EQ("car", name_of(car))) {
                const struct env_exp r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                return (struct env_exp){ env, fst(ensure_pair(trap, r.exp)) };
            } else if (STR_EQ("cdr", name_of(car))) {
                const struct env_exp r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                return (struct env_exp){ env, snd(ensure_pair(trap, r.exp)) };
            } else if (STR_EQ("set", name_of(car))) {
                const struct env_exp var = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                const struct env_exp val = eval_impl(trap, (struct env_exp){ env, caddr(trap, exp) }, print_context);
                env_define(env, name_of(var.exp), val.exp);
                return (struct env_exp){ env, val.exp };
            } else if (STR_EQ("cond", name_of(car))) {
                cadr(trap, exp); // check at least one branch exist.
                return cond(trap, env, snd(exp), print_context);
            } else if (STR_EQ("lambda", name_of(car))) {
                return closure(trap, env, exp);
            } else {
                return apply(trap, env_exp, print_context);
            }
        } else {
            return apply(trap, env_exp, print_context);
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

const struct env_exp cond(jmp_buf trap, struct env* env, const struct sexp* cond_cdr, struct print_context* print_context) {
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
        const struct env_exp pred = eval_impl(trap, (struct env_exp){ env, fst(ensure_pair(trap, branch)) }, print_context);
        if (nil(pred.exp)) {
            return cond(trap, env, snd(cond_cdr), print_context);
        } else {
            return eval_impl(trap, (struct env_exp){ env, cadr(trap, branch) }, print_context);
        }
    }
}

const struct env_exp closure(jmp_buf trap, struct env* env, const struct sexp* exp) {
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

const struct env_exp apply(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    const struct env_exp evaluated = map_eval(trap, env_exp, print_context);
    const struct sexp* exp = evaluated.exp;
    if (atom(exp)) {
        fprintf(stderr, Err_illegal_argument, text(env_exp.exp));
        fflush(stderr);
        longjmp(trap, TRAP_ILLARG);
    } else {
        const struct sexp* func = fst(exp);
        const struct sexp* args = snd(exp);
        struct env* closed_env = get_environment(trap, func);
        const struct sexp* pars = get_params(trap, func);
        const struct sexp* body = get_body(trap, func);
        jmp_buf trap2;
        if (setjmp(trap2)) {
            fprintf(stderr, ": %s v.s. %s", text(pars), text(args));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        } else {
            struct env* env = env_create(closed_env);
            append_defs(env, zip(trap, pars, args));
            return (struct env_exp){ env, fold_eval(trap, (struct env_exp){ env, body }, NIL(), print_context) };
        }
    }
}

const struct env_exp map_eval(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    struct env* env = env_exp.env;
    const struct sexp* exp = env_exp.exp;
    if (atom(exp)) {
        return env_exp;
    } else {
        const struct env_exp r_car = eval_impl(trap, (struct env_exp){ env, fst(exp) }, print_context);
        const struct env_exp r_cdr = map_eval(trap, (struct env_exp){ env, snd(exp) }, print_context);
        return (struct env_exp){ env, cons(r_car.exp, r_cdr.exp) };
    }
}

const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value, struct print_context* print_context) {
    const struct sexp* xs = env_xs.exp;
    if (atom(xs)) {
        return def_value;
    } else {
        struct env_exp evaled = eval_impl(trap, (struct env_exp){ env_xs.env, fst(xs) }, print_context);
        return fold_eval(trap, (struct env_exp){ env_xs.env, snd(xs) }, evaled.exp, print_context);
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

void append_defs(struct env* env, const struct sexp* def) {
    while (!atom(def)) {
        const struct sexp* pair = fst(def);
        env_define(env, name_of(fst(pair)), snd(pair));
        def = snd(def);
    }
}
