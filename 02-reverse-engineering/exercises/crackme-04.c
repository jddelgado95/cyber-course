#include <stdio.h>
#include <string.h>

static int validate(const char *s) {
    /* check 1: must be exactly 6 characters */
    if ((int)strlen(s) != 6) return 0;

    /* check 2: must start with 'K' */
    if (s[0] != 'K') return 0;

    /* check 3: sum of all character values must equal 504 */
    int sum = 0;
    for (int i = 0; i < 6; i++) sum += (unsigned char)s[i];
    if (sum != 504) return 0;

    /* check 4: XOR of characters at index 2 and 3 must equal 3 */
    if ((s[2] ^ s[3]) != 3) return 0;

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <serial>\n", argv[0]);
        return 1;
    }
    puts(validate(argv[1]) ? "Serial accepted." : "Invalid serial.");
    return 0;
}
