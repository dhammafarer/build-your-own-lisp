#pragma once

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

typedef lval *(*lbuiltin)(lenv *, lval *);

struct lval {
    int type;

    /* Basic */
    long num;
    char *err;
    char *sym;

    /* Function */
    lbuiltin builtin;
    lenv *env;
    lval * formals;
    lval * body;

    /* Expression */
    int count;
    struct lval **cell;
};

struct lenv {
    lenv *par;
    int count;
    char **syms;
    lval **vals;
};

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

void lval_del(lval *v);
void lval_println(lval *v);
lval *lval_read(mpc_ast_t *t);
lval *lval_eval(lenv *e, lval *v);
lenv *lenv_new(void);
void lenv_add_builtins(lenv *e);
void lenv_del(lenv *e);
