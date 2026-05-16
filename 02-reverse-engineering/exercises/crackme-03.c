#include <stdio.h>
#include <string.h>

static void build_token(char *out) {
    /* token is assembled at runtime from two byte arrays — not visible to strings */
    unsigned char a[] = {0x16, 0x01, 0x24, 0x01, 0x16, 0x13, 0x01, 0x1c, 0x51};
    unsigned char b[] = {0x64, 0x32, 0x52, 0x32, 0x64, 0x60, 0x32, 0x78, 0x70};
    for (int i = 0; i < 9; i++)
        out[i] = a[i] ^ b[i];
    out[9] = '\0';
}

int main(void) {
    char token[16];
    char input[64];

    build_token(token);

    printf("Token: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    puts(strcmp(input, token) == 0 ? "Correct!" : "Wrong.");
    return 0;
}
