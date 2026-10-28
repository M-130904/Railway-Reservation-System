#include "schedule.h"
#include "logger.h"
#include "train.h"

#include <stdio.h>
#include <string.h>

#define MAX_SCHEDULES 200
#define SCHEDULES_FILE "data/schedules.txt"
#define LINE_LENGTH 256

/* ==========================================
 * IN-MEMORY DATA STRUCTURE (RAM STORAGE)
 * ========================================== */
static Schedule g_schedules[MAX_SCHEDULES];
static int g_schedule_count = 0;

/* 1. INITIALIZATION: Load data from file into RAM */
int init_schedules(void)
{
    FILE *f = fopen(SCHEDULES_FILE, "r");
    char line[LINE_LENGTH];

    g_schedule_count = 0;

    if (f != NULL) {
        while ((g_schedule_count < MAX_SCHEDULES) &&
               (fgets(line, sizeof(line), f) != NULL)) {
            Schedule s = {0};

            if (sscanf(line,
                       "%d|%d|%11[^|]|%5[^|]|%5[^|]|"
                       "%d|%d|%d|%d|%d|%d|%d|%d|%d",
                       &s.schedule_id,
                       &s.train_id,
                       s.journey_date,
                       s.departure,
                       s.arrival,
                       &s.ac1_cap,
                       &s.ac2_cap,
                       &s.ac3_cap,
                       &s.sl_cap,
                       &s.ac1_booked,
                       &s.ac2_booked,
                       &s.ac3_booked,
                       &s.sl_booked,
                       &s.is_active) == 14) {
                g_schedules[g_schedule_count] = s;
                g_schedule_count++;
            }
        }

        (void)fclose(f);
    }

    return g_schedule_count;
}

/* 2. PERSISTENCE: Save RAM array back to disk */
int save_schedules(void)
{
    int result = 1;
    FILE *f = fopen(SCHEDULES_FILE, "w");

    if (f == NULL) {
        result = 0;
    } else {
        for (int i = 0; (i < g_schedule_count) && (result != 0); i++) {
            const Schedule *s = &g_schedules[i];

            if (fprintf(f, "%d|%d|%s|%s|%s|%d|%d|%d|%d|"
                        "%d|%d|%d|%d|%d\n",
                        s->schedule_id,
                        s->train_id,
                        s->journey_date,
                        s->departure,
                        s->arrival,
                        s->ac1_cap,
                        s->ac2_cap,
                        s->ac3_cap,
                        s->sl_cap,
                        s->ac1_booked,
                        s->ac2_booked,
                        s->ac3_booked,
                        s->sl_booked,
                        s->is_active) < 0) {
                (void)fclose(f);
                result = 0;
            }
        }

        if (result != 0) {
            if (fclose(f) != 0) {
                result = 0;
            }
        }
    }

    return result;
}

/* 3. OPERATIONS: Manipulate schedules in RAM */

int add_schedule(int sched_id, int train_id,
                 const char *date, const char *dep,
                 const char *arr,
                 int ac1, int ac2, int ac3, int sl)
{
    int result = 0;
    //int duplicate = 0;

    if (g_schedule_count < MAX_SCHEDULES) {
        /* Validate required string arguments */
        if ((date != NULL) && (dep != NULL) && (arr != NULL)) {
            /* Validate that the train exists and is active */
            if (find_train_by_id(train_id) != NULL) {
                int duplicate = 0;
                /* Check for duplicate schedule ID */
                for (int i = 0; (i < g_schedule_count) && (duplicate == 0); i++) {
                    if (g_schedules[i].schedule_id == sched_id) {
                        duplicate = 1;
                    }
                }

                /* Validate capacities */
                if ((duplicate == 0) &&
                    (ac1 >= 0) && (ac2 >= 0) && (ac3 >= 0) && (sl >= 0)) {
                    /* Populate new schedule structure */
                    Schedule new_s = {0};

                    new_s.schedule_id = sched_id;
                    new_s.train_id = train_id;

                    (void)snprintf(new_s.journey_date, sizeof(new_s.journey_date),
                                   "%s", date);
                    (void)snprintf(new_s.departure, sizeof(new_s.departure),
                                   "%s", dep);
                    (void)snprintf(new_s.arrival, sizeof(new_s.arrival),
                                   "%s", arr);

                    new_s.ac1_cap = ac1;
                    new_s.ac2_cap = ac2;
                    new_s.ac3_cap = ac3;
                    new_s.sl_cap = sl;

                    new_s.ac1_booked = 0;
                    new_s.ac2_booked = 0;
                    new_s.ac3_booked = 0;
                    new_s.sl_booked = 0;
                    new_s.is_active = 1;

                    g_schedules[g_schedule_count] = new_s;
                    g_schedule_count++;

                    /* Persist changes; roll back RAM count if saving fails */
                    if (save_schedules() != 0) {
                        log_message("INFO", "Schedule added successfully");
                        result = 1;
                    } else {
                        g_schedule_count--;
                    }
                }
            }
        }
    }

    return result;
}

void list_schedules(void)
{
    if (g_schedule_count == 0) {
        (void)printf("No schedules found.\n");
    } else {
        (void)printf("\n=== Schedule Master List ===\n");

        for (int i = 0; i < g_schedule_count; i++) {
            if (g_schedules[i].is_active != 0) {
                (void)printf("Sched ID: %d | Train ID: %d | Date: %s | "
                             "Dep: %s | Arr: %s | AC1: %d | AC2: %d | "
                             "AC3: %d | SL: %d\n",
                             g_schedules[i].schedule_id,
                             g_schedules[i].train_id,
                             g_schedules[i].journey_date,
                             g_schedules[i].departure,
                             g_schedules[i].arrival,
                             g_schedules[i].ac1_cap,
                             g_schedules[i].ac2_cap,
                             g_schedules[i].ac3_cap,
                             g_schedules[i].sl_cap);
            }
        }
    }
}

Schedule *find_schedule_by_id(int sched_id)
{
    Schedule *result = NULL;

    for (int i = 0; (i < g_schedule_count) && (result == NULL); i++) {
        if ((g_schedules[i].schedule_id == sched_id) &&
            (g_schedules[i].is_active != 0)) {
            result = &g_schedules[i];
        }
    }

    return result;
}