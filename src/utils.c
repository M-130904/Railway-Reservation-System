#include "utils.h"

void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void safe_fgets(char *buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
    }
}

void generate_pnr(char *pnr_buffer) {
    int r = 100000 + rand() % 900000;
    sprintf(pnr_buffer, "PNR2026%d", r);
}