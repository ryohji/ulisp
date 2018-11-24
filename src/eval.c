#include "ulisp.h"
#include "env.h"

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

static const struct sexp* find(jmp_buf trap, const char* name, struct env* env);
/* return car(cdr(exp)); throw TRAP_ILLARG if cdr(exp) is not pair. exp should be pair. */
static const struct sexp* cadr(jmp_buf trap, const struct sexp* exp);
/* return car(cdr(cdr(exp))); throw TRAP_ILLARG if cdr(exp) or cdr(cdr(exp)) is not pair. exp should be pair. */
static const struct sexp* caddr(jmp_buf trap, const struct sexp* exp);
/* helper for cadr, caddr. */
typedef const struct sexp* (*leaf_iterator)(const struct sexp*);
static const struct sexp* leaf(jmp_buf trap, const struct sexp* exp, leaf_iterator* fst_or_snd);
static const struct sexp* ensure_pair(jmp_buf trap, const struct sexp* exp);
static const struct sexp* cond(jmp_buf trap, struct env* env, const struct sexp* cond_cdr, struct print_context* print_context);
static const struct sexp* closure(jmp_buf trap, struct env* env, const struct sexp* exp);
static const struct sexp* apply(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct sexp* map_eval(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value, struct print_context* print_context);
static const struct sexp* eval_impl(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);
static const struct env_exp eval_core(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context);

static void bind_params(jmp_buf trap, struct env* env, const struct sexp* params, const struct sexp* values);

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
        verbose = !nil(find(trap, "*verbose-eval*", env));
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
    const struct sexp* result = eval_impl(trap, env_exp, &print_context);
    fclose(print_context.verbose_eval);
    return (struct env_exp) { env_exp.env, result };
}

const struct sexp* eval_impl(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    print_nest(print_context);
    fprintf(print_context->verbose_eval, "EVALUATE: %s\n", text(env_exp.exp));
    ennest(print_context);
    print_env(env_exp.env, print_context);

    struct env_exp result = eval_core(trap, env_exp, print_context);

    unnest(print_context);
    print_nest(print_context);
    fprintf(print_context->verbose_eval, "\\___ %s\n", text(result.exp));
    return result.exp;
}


static const struct env_exp eval_core(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    struct env* env = env_exp.env;
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
                const struct sexp* head = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                const struct sexp* tail = eval_impl(trap, (struct env_exp){ env, caddr(trap, exp) }, print_context);
                return (struct env_exp){ env, cons(head, tail) };
            } else if (STR_EQ("atom", name_of(car))) {
                const struct sexp* r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                if (atom(r)) {
                    return (struct env_exp) { env, eval_impl(trap, (struct env_exp){ env, symbol("t") }, print_context) };
                } else {
                    return (struct env_exp){ env, NIL() };
                }
            } else if (STR_EQ("car", name_of(car))) {
                const struct sexp* r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                return (struct env_exp){ env, fst(ensure_pair(trap, r)) };
            } else if (STR_EQ("cdr", name_of(car))) {
                const struct sexp* r = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                return (struct env_exp){ env, snd(ensure_pair(trap, r)) };
            } else if (STR_EQ("set", name_of(car))) {
                const struct sexp* var = eval_impl(trap, (struct env_exp){ env, cadr(trap, exp) }, print_context);
                const struct sexp* val = eval_impl(trap, (struct env_exp){ env, caddr(trap, exp) }, print_context);
                env_define(env, name_of(var), val);
                return (struct env_exp){ env, val };
            } else if (STR_EQ("cond", name_of(car))) {
                cadr(trap, exp); // check at least one branch exist.
                return (struct env_exp) { env, cond(trap, env, snd(exp), print_context) };
            } else if (STR_EQ("lambda", name_of(car))) {
                return (struct env_exp) { env, closure(trap, env, exp) };
            } else {
                return (struct env_exp) { env, apply(trap, env_exp, print_context) };
            }
        } else {
            return (struct env_exp) { env, apply(trap, env_exp, print_context) };
        }
    }
}

const struct sexp* find(jmp_buf trap, const char* name, struct env* env) {
    if (env == NULL) {
        fprintf(stderr, Err_value_not_found, name);
        fflush(stderr);
        longjmp(trap, TRAP_NOSYM);
    } else {
        const struct env_iterator* it = env_it_begin(env);
        const struct env_iterator* end = env_it_end(env);
        while (it != end && !STR_EQ(name, env_it_name(it))) {
            it = env_it_next(it);
        }
        return it != end ? env_it_value(it) : find(trap, name, env_base(env));
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

const struct sexp* cond(jmp_buf trap, struct env* env, const struct sexp* cond_cdr, struct print_context* print_context) {
    if (atom(cond_cdr)) {
        if (nil(cond_cdr)) {
            return cond_cdr;
        } else {
            fprintf(stderr, Err_illegal_argument, text(cond_cdr));
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        }
    } else {
        const struct sexp* branch = fst(ensure_pair(trap, cond_cdr));
        const struct sexp* pred = eval_impl(trap, (struct env_exp){ env, fst(ensure_pair(trap, branch)) }, print_context);
        if (nil(pred)) {
            return cond(trap, env, snd(cond_cdr), print_context);
        } else {
            return eval_impl(trap, (struct env_exp){ env, cadr(trap, branch) }, print_context);
        }
    }
}

const struct sexp* closure(jmp_buf trap, struct env* env, const struct sexp* exp) {
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
            return make_applicable(env, param, body);
        }
    }
}

const struct sexp* apply(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    const struct sexp* exp = map_eval(trap, env_exp, print_context);
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
            bind_params(trap, env, pars, args);
            return fold_eval(trap, (struct env_exp){ env, body }, NIL(), print_context);
        }
    }
}

const struct sexp* map_eval(jmp_buf trap, const struct env_exp env_exp, struct print_context* print_context) {
    struct env* env = env_exp.env;
    const struct sexp* exp = env_exp.exp;
    if (atom(exp)) {
        return env_exp.exp;
    } else {
        const struct sexp* r_car = eval_impl(trap, (struct env_exp){ env, fst(exp) }, print_context);
        const struct sexp* r_cdr = map_eval(trap, (struct env_exp){ env, snd(exp) }, print_context);
        return cons(r_car, r_cdr);
    }
}

const struct sexp* fold_eval(jmp_buf trap, const struct env_exp env_xs, const struct sexp* def_value, struct print_context* print_context) {
    const struct sexp* xs = env_xs.exp;
    if (atom(xs)) {
        return def_value;
    } else {
        const struct sexp* evaled = eval_impl(trap, (struct env_exp){ env_xs.env, fst(xs) }, print_context);
        return fold_eval(trap, (struct env_exp){ env_xs.env, snd(xs) }, evaled, print_context);
    }
}

void bind_params(jmp_buf trap, struct env* env, const struct sexp* params, const struct sexp* values) {
    if (atom(params) && atom(values)) {
        if (!nil(params) || !nil(values)) {
            bind_params(trap, env, params, values);
        } else {
            // end of definitions.
        }
    } else {
        if (!atom(params) && !atom(values)) {
            env_define(env, name_of(fst(params)), fst(values));
            bind_params(trap, env, snd(params), snd(values));
        } else {
            fprintf(stderr, "List length mismatch.");
            fflush(stderr);
            longjmp(trap, TRAP_ILLARG);
        }
    }
}
