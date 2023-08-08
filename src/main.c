#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

typedef struct {
    int type;
    long num;
    int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create a new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;

    return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;

    return v;
}

void lval_print(lval v) {
    switch (v.type) {
    /* Print a number */
    case LVAL_NUM:
        printf("%li", v.num);
        break;

    /* Print an error */
    case LVAL_ERR:
        if (v.err == LERR_DIV_ZERO)
            printf("division by zero");
        if (v.err == LERR_BAD_OP)
            printf("Error: Invalid Operator!");
        if (v.err == LERR_BAD_NUM)
            printf("Error: Invalid Number!");
        break;
    }
}

/* Print an lval followed by a newline */
void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char *op, lval y) {
    /* If either value is an error, return it */
    if (x.type == LVAL_ERR) {
        return x;
    }
    if (y.type == LVAL_ERR) {
        return y;
    }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) {
        return lval_num(x.num + y.num);
    }
    if (strcmp(op, "-") == 0) {
        return lval_num(x.num - y.num);
    }
    if (strcmp(op, "*") == 0) {
        return lval_num(x.num * y.num);
    }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error */
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t) {
    /* If tagged as number, return it directly. */
    if (strstr(t->tag, "number")) {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);

        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* The operator is always the second child */
    char *op = t->children[1]->contents;

    /* Store the third child in `x` */
    lval x = eval(t->children[2]);

    /* Iterate the remaining children */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(void) {
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT, " \
            number   : /-?[0-9]/ ; \
            operator : '+' | '-' | '*' | '/' ; \
            expr     : <number> | '(' <operator> <expr>+ ')' ; \
            lispy    : /^/ <operator> <expr>+ /$/ ; \
            ",
              Number, Operator, Expr, Lispy);

    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+c to exit");

    while (1) {

        char *input = readline("lispy> ");

        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
