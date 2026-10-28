#include "file_io.h"
#include "logger.h"

void init_data_files(void) {
    FILE *f;

    f = fopen("data/users.txt", "a");
    if (f != NULL) {
        (void)fclose(f);
    }

    f = fopen("data/trains.txt", "a");
    if (f != NULL) {
        (void)fclose(f);
    }

    f = fopen("data/schedules.txt", "a");
    if (f != NULL) {
        (void)fclose(f);
    }

    f = fopen("data/reservations.txt", "a");
    if (f != NULL) {
        (void)fclose(f);
    }

    f = fopen("data/app.log", "a");
    if (f != NULL) {
        (void)fclose(f);
    }

    log_message("INFO", "System data storage files initialized");
}