#include "schedule.h"
#include "logger.h"

int add_schedule(int sched_id, int train_id, const char *date, const char *dep, const char *arr, int ac1, int ac2, int ac3, int sl) {
    FILE *f = fopen("data/schedules.txt", "a");
    if (!f) return 0;
    fprintf(f, "%d|%d|%s|%s|%s|%d|%d|%d|%d|0|0|0|0|1\n", sched_id, train_id, date, dep, arr, ac1, ac2, ac3, sl);
    fclose(f);
    log_message("INFO", "Schedule added successfully");
    return 1;
}

void list_schedules(void) {
    FILE *f = fopen("data/schedules.txt", "r");
    if (!f) { printf("No schedules found.\n"); return; }
    char line[256];
    printf("\n=== Schedule Master List ===\n");
    while (fgets(line, sizeof(line), f)) {
        Schedule s;
        if (sscanf(line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                   &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                   &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                   &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked, &s.is_active) >= 13) {
            printf("Sched ID: %d | Train ID: %d | Date: %s | Dep: %s | Arr: %s\n",
                   s.schedule_id, s.train_id, s.journey_date, s.departure, s.arrival);
        }
    }
    fclose(f);
}

Schedule* find_schedule_by_id(int sched_id) {
    static Schedule s;
    FILE *f = fopen("data/schedules.txt", "r");
    if (!f) return NULL;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                   &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                   &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                   &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked, &s.is_active) >= 13) {
            if (s.schedule_id == sched_id && s.is_active) {
                fclose(f);
                return &s;
            }
        }
    }
    fclose(f);
    return NULL;
}