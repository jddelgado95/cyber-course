// program.c — safe version, used in exercises 1, 2, and 3
#include <stdio.h>
#include <stdlib.h>

char greeting[] = "Hello!";

void say_hello(char *name) {
    char buf[32];
    sprintf(buf, "%s, %s", greeting, name);
    puts(buf);
}

int main() {
    char *input = malloc(64);
    fgets(input, 64, stdin);
    say_hello(input);
    free(input);
    return 0;
}
