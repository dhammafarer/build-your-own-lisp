#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

static char input[2048];

/* Fake readline function */
char *readline(char *prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

/* Fake add_history function */
void add_history(char *unused) {}

#else

#include <editline/readline.h>

#endif

int main(void) {

    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+c to exit");

    while (1) {

        char *input = readline("lispy> ");

        add_history(input);

        printf("%s\n", input);

        free(input);
    }

    return 0;
}
