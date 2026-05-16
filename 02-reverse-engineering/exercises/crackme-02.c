#include <stdio.h>
#include <string.h>

static int check(const char *input) {
    /* key is XOR-ed with 0xaa before storage */
    unsigned char enc[] = {0xda, 0xcb, 0xd9, 0xd9, 0x9b};
    int n = (int)sizeof(enc);
    if ((int)strlen(input) != n) return 0;
    for (int i = 0; i < n; i++)
        if (((unsigned char)input[i] ^ 0xaa) != enc[i]) return 0;
    return 1;
}

int main(void) {
    char buf[64];
    printf("Key: ");
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\n")] = 0;
    puts(check(buf) ? "Correct!" : "Wrong.");
    return 0;
}
