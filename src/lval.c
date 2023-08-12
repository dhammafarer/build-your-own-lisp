#include "lval.h"
#include <stdio.h>
#include <stdlib.h>

lval *lval_num(long x);
lval *lval_err(char *fmt, ...);
lval *lval_sym(char *s);
lval *lval_sexpr(void);
lval *lval_qexpr(void);

lval *lval_read_num(mpc_ast_t *t);
lval *lval_add(lval *v, lval *x);
lval *lval_join(lval *x, lval *y);

void lval_print(lval *v);
void lval_expr_print(lval *v, char open, char close);

lval *lval_take(lval *v, int i);
lval *lval_pop(lval *v, int i);

char *ltype_name(int t);

lval *builtin_op(lenv *e, lval *a, char *op);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_list(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_def(lenv *e, lval *a);

lval *builtin(lenv *e, lval *a, char *func);

lval *lenv_get(lenv *e, lval *k);

#define LASSERT(args, cond, fmt, ...)                                          \
    if (!(cond)) {                                                             \
        lval *err = lval_err(fmt, ##__VA_ARGS__);                              \
        lval_del(args);                                                        \
        return err;                                                            \
    }

#define LASSERT_EMPTY(args)                                                    \
    LASSERT(args, args->count != 0, "Function called with empty list")

lval *lval_num(long x) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;

    return v;
}

lval *lval_err(char *fmt, ...) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    /* Allocate 512 bytes of space */
    v->err = malloc(512);

    /* printf the eror string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);

    /* Reallocate to number of bytes actually used*/
    v->err = realloc(v->err, strlen(v->err) + 1);

    /* Cleanup va list */
    va_end(va);

    return v;
}

lval *lval_sym(char *s) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);

    return v;
}

lval *lval_fun(lbuiltin func) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;

    return v;
}

lval *lval_sexpr(void) {
    lval *v = malloc(sizeof(lval));

    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;
}

lval *lval_qexpr(void) {
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;
}

void lval_del(lval *v) {
    switch (v->type) {
    /* Do nothing special for number type */
    case LVAL_NUM:
        break;

    /* For Exx or Sym free the string data */
    case LVAL_ERR:
        free(v->err);
        break;
    case LVAL_SYM:
        free(v->sym);
        break;

    case LVAL_FUN:
        break;

    /* If Qexpr or Sexpr then delete all elements inside */
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        for (int i = 0; i < v->count; i++) {
            lval_del(v->cell[i]);
        }
        free(v->cell);
        break;
    }
}

lval *lval_read_num(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("Invalid Number");
}

lval *lval_add(lval *v, lval *x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;

    return v;
}

lval *lval_read(mpc_ast_t *t) {
    /* If Symbol or Number, return conversion to that type */
    if (strstr(t->tag, "number"))
        return lval_read_num(t);
    if (strstr(t->tag, "symbol"))
        return lval_sym(t->contents);

    /* If root (>) or sexpr then create empty list */
    lval *x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = lval_sexpr();
    }
    if (strstr(t->tag, "sexpr"))
        x = lval_sexpr();
    if (strstr(t->tag, "qexpr"))
        x = lval_qexpr();

    /* Fill this list with any valid expressions contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, ")") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "{") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "}") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->tag, "regex") == 0) {
            continue;
        }
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}

lval *lval_copy(lval *v) {
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        /* Copy Functions and Numbers directly */
    case LVAL_FUN:
        x->fun = v->fun;
        break;
    case LVAL_NUM:
        x->num = v->num;
        break;

    /* Copy Strings using malloc and strcpy */
    case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err);
        break;

    case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) + 1);
        strcpy(x->sym, v->sym);
        break;

    /* Copy lists by copying each sub-expression */
    case LVAL_QEXPR:
    case LVAL_SEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(lval *) * v->count);

        for (int i = 0; i < v->count; i++) {
            x->cell[i] = lval_copy(v->cell[i]);
        }
        break;
    }

    return x;
}

void lval_print(lval *v) {
    switch (v->type) {
    case LVAL_NUM:
        printf("%li", v->num);
        break;

    case LVAL_FUN:
        printf("<function>");
        break;

    case LVAL_ERR:
        printf("Error: %s", v->err);
        break;
    case LVAL_SYM:
        printf("%s", v->sym);
        break;
    case LVAL_SEXPR:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_expr_print(v, '{', '}');
        break;
    }
}

void lval_expr_print(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        /* Print value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* Print an lval followed by a newline */
void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

lval *lval_eval_sexpr(lenv *e, lval *v) {
    /* Evaluate children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    /* Empty expression */
    if (v->count == 0) {
        return v;
    }

    /* Single expression */
    if (v->count == 1) {
        return lval_take(v, 0);
    }

    /* Ensure first element is a function after evaluation */
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval_del(v);
        lval_del(f);
        return lval_err("First element is nota function.");
    }

    /* If so, call function to get result */
    lval *result = f->fun(e, v);
    lval_del(f);
    return result;
}

lval *lval_eval(lenv *e, lval *v) {
    /* Evaluate S-Expression */
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        return x;
    }
    if (v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }

    /* All other lval types remain the same */
    return v;
}

lval *lval_pop(lval *v, int i) {
    /* ifind the item at i */
    lval *x = v->cell[i];

    /* Shift memory after the item at i over the top */
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    return x;
}

lval *lval_take(lval *v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);

    return x;
}

lval *builtin_head(lenv *e, lval *a) {
    /* Check error conditions */
    LASSERT(a, a->count == 1,
            "Function 'head' passed to many arguments! "
            "Got %i, expected %i.",
            a->count, 1);

    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'head' passed incorrect type for argument 0. "
            "Got %s, expected %s",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

    LASSERT_EMPTY(a);

    /* Take first argument */
    lval *v = lval_take(a, 0);

    /* Delete all elements that are not head and return */
    while (v->count > 1) {
        lval_del(lval_pop(v, 1));
    }

    return v;
}

lval *builtin_tail(lenv *e, lval *a) {
    LASSERT(a, a->count == 1,
            "Function 'tail' passed to many arguments! "
            "Got %i, expected %i.",
            a->count, 1);

    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'tail' passed incorrect type for argument 0. "
            "Got %s, expected %s",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_QEXPR));

    LASSERT_EMPTY(a);

    /* Otherwise take first argument */
    lval *v = lval_take(a, 0);

    /* Delete first element and return */
    lval_del(lval_pop(v, 0));

    return v;
}

lval *builtin_cons(lenv *e, lval *a) {
    LASSERT_EMPTY(a);
    LASSERT(a, a->count == 2,
            "Function 'cons' passed incorrect number of arguments! "
            "Got %i, expected %i.",
            a->count, 2);

    LASSERT(a, a->cell[0]->type == LVAL_NUM,
            "Function 'cons' passed incorrect type for argument 1. "
            "Got %s, expected %s",
            ltype_name(a->cell[0]->type), ltype_name(LVAL_NUM));

    LASSERT(a, a->cell[1]->type == LVAL_QEXPR,
            "Function 'cons' passed incorrect type for argument 1. "
            "Got %s, expected %s",
            ltype_name(a->cell[1]->type), ltype_name(LVAL_QEXPR));

    /* Take the first element from Qexpr */

    lval *x = lval_qexpr();
    lval_add(x, lval_pop(a, 0));
    lval *y = lval_pop(a, 0);
    lval_del(a);

    return lval_join(x, y);
}

lval *builtin_list(lenv *e, lval *a) {
    a->type = LVAL_QEXPR;
    return a;
}

lval *builtin_eval(lenv *e, lval *a) {
    LASSERT(a, a->count == 1, "Function 'eval' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'eval' passed incorrect type!");

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;

    return lval_eval(e, x);
}

lval *builtin_join(lenv *e, lval *a) {
    for (int i = 0; i < a->count; i++) {
        LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
                "Function 'join' passed incorrect type!");
    }

    lval *x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval *lval_join(lval *x, lval *y) {
    /* For each cell in 'y' add it to 'x' */
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    /* Delete empty 'y' and return 'x' */
    lval_del(y);
    return x;
}

lval *builtin_add(lenv *e, lval *a) { return builtin_op(e, a, "+"); }

lval *builtin_sub(lenv *e, lval *a) { return builtin_op(e, a, "-"); }

lval *builtin_mul(lenv *e, lval *a) { return builtin_op(e, a, "*"); }

lval *builtin_div(lenv *e, lval *a) { return builtin_op(e, a, "/"); }

lval *builtin_op(lenv *e, lval *a, char *op) {
    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if (a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Operands must be numbers");
        }
    }

    /* Pop the first element */
    lval *x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* While there are still elemets remaining */
    while (a->count > 0) {
        /* Pop the next element */
        lval *y = lval_pop(a, 0);
        if (strcmp(op, "+") == 0) {
            x->num += y->num;
        }
        if (strcmp(op, "-") == 0) {
            x->num -= y->num;
        }
        if (strcmp(op, "*") == 0) {
            x->num *= y->num;
        }
        if (strcmp(op, "/") == 0) {
            if (y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by zero");
                break;
            }
            x->num /= y->num;
        }
        lval_del(y);
    }
    lval_del(a);

    return x;
}

lenv *lenv_new(void) {
    lenv *e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;

    return e;
}

void lenv_del(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

lval *lenv_get(lenv *e, lval *k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    return lval_err("Unbound Symbol '%s'", k->sym);
}

void lenv_put(lenv *e, lval *k, lval *v) {
    /* Iterate over all items in environment */
    /* This is to see if variable already exists */
    for (int i = 0; i < e->count; i++) {

        /* If variable is found, delete item at that position*/
        /* Replace with variable supplied by user */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* If no existing entry found, allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);
    e->syms = realloc(e->syms, sizeof(char *) * e->count);

    /* Copy contents of lval and symbol string into new location */
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *k = lval_sym(name);
    lval *v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv *e) {
    /* List Functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "def", builtin_def);

    /* Math Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
}

lval *builtin_def(lenv *e, lval *a) {
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
            "Function 'def' passed incorrect type");

    /* First argument is symbol list */
    lval *syms = a->cell[0];

    /* Ensure all elements of first list are symbols */
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, syms->cell[i]->type == LVAL_SYM,
                "Function 'def' cannot define non-symbol");
    }

    /* Check correct number of symbols and values */
    LASSERT(
        a, syms->count == a->count - 1,
        "Function 'def' cannot define incorrect number of values to symbols");

    /* Assign copies of values to symbols */
    for (int i = 0; i < syms->count; i++) {
        lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    lval_del(a);

    return lval_sexpr();
}

char *ltype_name(int t) {
    switch (t) {
    case LVAL_FUN:
        return "Function";
    case LVAL_NUM:
        return "Number";
    case LVAL_SYM:
        return "Symbol";
    case LVAL_ERR:
        return "Error";
    case LVAL_QEXPR:
        return "Qexpr";
    case LVAL_SEXPR:
        return "Sexpr";
    default:
        return "Unknown";
    }
}
