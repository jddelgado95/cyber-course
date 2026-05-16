#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <password>\n", argv[0]);
        return 1;
    }

    char *pass = argv[1];

    /* Each check calls a different libc function — visible in ltrace, messy to
       trace through disassembly because results flow through registers and
       pointer arithmetic between calls. */

    if (strlen(pass) != 8) {
        puts("Wrong.");
        return 1;
    }

    char *sep = strchr(pass, '-');
    if (!sep || (sep - pass) != 3) {
        puts("Wrong.");
        return 1;
    }

    if (strncmp(pass, "CTF", 3) != 0) {
        puts("Wrong.");
        return 1;
    }

    if (strcmp(pass + 4, "2024") != 0) {
        puts("Wrong.");
        return 1;
    }

    puts("Access granted!");
    return 0;
}
