#include <editline/readline.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lval.h"
#include "mpc.h"

/* Parser Declariations */

mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispy;

int main(int argc, char **argv) {

    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "                                              \
      number  : /-?[0-9]+/ ;                       \
      symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
      string  : /\"(\\\\.|[^\"])*\"/ ;             \
      comment : /;[^\\r\\n]*/ ;                    \
      sexpr   : '(' <expr>* ')' ;                  \
      qexpr   : '{' <expr>* '}' ;                  \
      expr    : <number>  | <symbol> | <string>    \
              | <comment> | <sexpr>  | <qexpr>;    \
      lispy   : /^/ <expr>* /$/ ;                  \
    ",
              Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    lenv *e = lenv_new();
    lenv_add_builtins(e);

    /* Interactive Prompt */
    if (argc == 1) {

        puts("Lispy Version 0.0.0.1.0");
        puts("Press Ctrl+c to Exit\n");

        while (1) {

            char *input = readline("lispy> ");
            add_history(input);

            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Lispy, &r)) {

                lval *x = lval_eval(e, lval_read(r.output));
                lval_println(x);
                lval_del(x);

                mpc_ast_delete(r.output);
            } else {
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }

            free(input);
        }
    }

    /* Supplied with list of files */
    if (argc >= 2) {

        /* loop over each supplied filename (starting from 1) */
        for (int i = 1; i < argc; i++) {

            /* Argument list with a single argument, the filename */
            lval *args = lval_add(lval_sexpr(), lval_str(argv[i]));

            /* Pass to builtin load and get the result */
            lval *x = builtin_load(e, args);

            /* If the result is an error be sure to print it */
            if (x->type == LVAL_ERR) {
                lval_println(x);
            }
            lval_del(x);
        }
    }

    lenv_del(e);

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
