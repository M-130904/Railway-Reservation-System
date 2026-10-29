#include "logger.h"

void log_message(const char *level, const char *message) {
    FILE *file = fopen("data/app.log", "a");

    if (file != NULL) {
        time_t now = time(NULL);
        const struct tm *t = localtime(&now);
        char time_str[30];

        (void)strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

        (void)fprintf(file, "[%s] [%s] %s\n", time_str, level, message);
        (void)fclose(file);
    }
}