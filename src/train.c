#include "train.h"
#include "logger.h"
#include <stdio.h>      /* MISRA 21.6 deviation: file I/O + console */
#include <string.h>

#define MAX_TRAINS 100

/* ==========================================
 * IN-MEMORY DATA STRUCTURE (RAM STORAGE)
 * ========================================== */
static Train g_trains[MAX_TRAINS];
static int g_train_count = 0;

/* 1. INITIALIZATION: Load data from file into RAM once */
int init_trains(void) {
    FILE *f = fopen("data/trains.txt", "r");
    g_train_count = 0;

    if (f != NULL) {
        /* FIX 11.5 + 21.3: stack buffer instead of malloc/free,
           removes void*->char* conversion and heap allocation. */
        char line[256];

        while ((g_train_count < MAX_TRAINS) &&
               (fgets(line, (int)sizeof(line), f) != NULL)) {
            Train t;

            if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d",
                       &t.train_id, t.train_name, t.source,
                       t.destination, &t.capacity, &t.base_fare,
                       &t.is_active) == 7) {
                g_trains[g_train_count] = t;
                g_train_count++;
            }
        }

        (void)fclose(f);
    }

    return g_train_count;
}

/* 2. TERMINATION / PERSISTENCE: Save RAM array back to disk */
int save_trains(void) {
    int result = 1;
    FILE *f = fopen("data/trains.txt", "w");

    if (f == NULL) {
        result = 0;
    } else {
        for (int i = 0; i < g_train_count; i++) {
            (void)fprintf(f, "%d|%s|%s|%s|%d|%.2f|%d\n",
                          g_trains[i].train_id, g_trains[i].train_name,
                          g_trains[i].source, g_trains[i].destination,
                          g_trains[i].capacity, g_trains[i].base_fare,
                          g_trains[i].is_active);
        }
        (void)fclose(f);
    }

    return result;
}

/* 3. OPERATIONS: Instant manipulation in RAM */

int add_train(int id, const char *name, const char *src, const char *dest,
              int capacity, double base_fare) {
    int result = 0;

    if (g_train_count < MAX_TRAINS) {
        int duplicate = 0;

        for (int i = 0; (i < g_train_count) && (duplicate == 0); i++) {
            if (g_trains[i].train_id == id) {
                duplicate = 1;
            }
        }

        if (duplicate == 0) {
            Train new_t;

            new_t.train_id = id;
            (void)snprintf(new_t.train_name, sizeof(new_t.train_name), "%s", name);
            (void)snprintf(new_t.source, sizeof(new_t.source), "%s", src);
            (void)snprintf(new_t.destination, sizeof(new_t.destination), "%s", dest);
            new_t.capacity = capacity;
            new_t.base_fare = base_fare;
            new_t.is_active = 1;

            g_trains[g_train_count] = new_t;
            g_train_count++;

            if (save_trains() != 0) {
                log_message("INFO", "Train added successfully");
                result = 1;
            }
        }
    }

    return result;
}

void list_trains(void) {
    if (g_train_count == 0) {
        (void)printf("No trains found.\n");
    } else {
        (void)printf("\n=== Train Master List ===\n");
        for (int i = 0; i < g_train_count; i++) {
            if (g_trains[i].is_active != 0) {
                (void)printf("ID: %d | Name: %s | From: %s | To: %s | Cap: %d | Fare: Rs. %.2f\n",
                             g_trains[i].train_id, g_trains[i].train_name,
                             g_trains[i].source, g_trains[i].destination,
                             g_trains[i].capacity, g_trains[i].base_fare);
            }
        }
    }
}

Train* find_train_by_id(int id) {
    Train *result = NULL;

    for (int i = 0; (i < g_train_count) && (result == NULL); i++) {
        if ((g_trains[i].train_id == id) && (g_trains[i].is_active != 0)) {
            result = &g_trains[i];
        }
    }

    return result;
}