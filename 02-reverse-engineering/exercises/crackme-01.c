#include <stdio.h>
#include <string.h>

int main(void) {
    char input[64];
    printf("Enter password: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    if (strcmp(input, "sup3rs3cr3t") == 0)
        puts("Access granted!");
    else
        puts("Access denied.");
    return 0;
}
