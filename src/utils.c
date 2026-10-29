#include "utils.h"

void clear_input_buffer(void) {
    int c = getchar();

    while ((c != (int)'\n') && (c != EOF)) {
        c = getchar();
    }
}

void safe_fgets(char *buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);

        if ((len > 0U) && (buffer[len - 1U] == '\n')) {
            buffer[len - 1U] = '\0';
        }
    }
}

void generate_pnr(char *pnr_buffer) {
    int r = 100000 + (rand() % 900000);

    (void)sprintf(pnr_buffer, "PNR2026%d", r);
}