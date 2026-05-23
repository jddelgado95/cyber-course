// program-vuln.c — vulnerable version, used in exercises 4 and 5
// Changes from program.c:
//   1. gets(input) replaces fgets — no bounds check
//   2. win() function added as the exploit target
#include <stdio.h>
#include <stdlib.h>

/* gets() was removed from C11 headers but still exists in libc.
   Forward-declare it so the compiler accepts it without an error. */
extern char *gets(char *s);

char greeting[] = "Hello!";

void win() {
    printf("[+] You redirected execution to win()!\n");
}

void say_hello(char *name) {
    char buf[32];
    sprintf(buf, "%s, %s", greeting, name);
    puts(buf);
}

int main() {
    char *input = malloc(64);
    printf("Enter your name: ");
    gets(input);
    say_hello(input);
    free(input);
    return 0;
}
