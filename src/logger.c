#include "logger.h"

void log_message(const char *level, const char *message) {
    FILE *file = fopen("data/app.log", "a");
    if (!file) return;

    time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    char time_str[30];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(file, "[%s] [%s] %s\n", time_str, level, message);
    fclose(file);
}