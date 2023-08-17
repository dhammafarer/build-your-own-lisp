#pragma once

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_STR,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

typedef lval *(*lbuiltin)(lenv *, lval *);

struct lval {
    int type;

    /* Basic */
    long num;
    char *err;
    char *sym;
    char *str;

    /* Function */
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

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

lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);
lval *lval_lambda(lval *formals, lval *body);
lval *lval_str(char *s);

lval *lval_read_num(mpc_ast_t *t);
lval *lval_read_str(mpc_ast_t *t);

lval *lval_add(lval *v, lval *x);
lval *lval_join(lval *x, lval *y);

void lval_print(lval *v);
void lval_print_str(lval *v);
void lval_expr_print(lval *v, char open, char close);

lval *lval_take(lval *v, int i);
lval *lval_pop(lval *v, int i);
lval *lval_call(lenv *e, lval *f, lval *a);

char *ltype_name(int t);

int lval_eq(lval *x, lval *y);

lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_lambda(lenv *e, lval *a);
lval *builtin_if(lenv *e, lval *a);
lval *builtin_load(lenv *e, lval *a);
lval *builtin_print(lenv *e, lval *a);
lval *builtin_error(lenv *e, lval *a);

/* Comparison functions */
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);

lval *builtin_ord(lenv *e, lval *a, char *op);
lval *builtin_cmp(lenv *e, lval *a, char *op);

lval *builtin(lenv *e, lval *a, char *func);

lval *lenv_get(lenv *e, lval *k);
lenv *lenv_copy(lenv *e);
void lenv_def(lenv *e, lval *k, lval *v);

void lval_del(lval *v);
void lval_println(lval *v);
lval *lval_read(mpc_ast_t *t);
lval *lval_eval(lenv *e, lval *v);
lenv *lenv_new(void);
void lenv_add_builtins(lenv *e);
void lenv_del(lenv *e);
