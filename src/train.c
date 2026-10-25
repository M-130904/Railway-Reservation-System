#include "train.h"
#include "logger.h"
#include <stdio.h>
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
    g_train_count = 0; // Reset RAM count
    if (!f) return 0;
    
    char line[256];
    while (g_train_count < MAX_TRAINS && fgets(line, sizeof(line), f)) {
        Train t;
        if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d", 
                   &t.train_id, t.train_name, t.source, t.destination, &t.capacity, &t.base_fare, &t.is_active) == 7) {
            g_trains[g_train_count++] = t;
        }
    }
    fclose(f);
    return g_train_count;
}

/* 2. TERMINATION / PERSISTENCE: Save RAM array back to disk */
int save_trains(void) {
    FILE *f = fopen("data/trains.txt", "w");
    if (!f) return 0;
    
    for (int i = 0; i < g_train_count; i++) {
        fprintf(f, "%d|%s|%s|%s|%d|%.2f|%d\n", 
                g_trains[i].train_id, g_trains[i].train_name, 
                g_trains[i].source, g_trains[i].destination, 
                g_trains[i].capacity, g_trains[i].base_fare, 
                g_trains[i].is_active);
    }
    fclose(f);
    return 1;
}

/* 3. OPERATIONS: Instant manipulation in RAM */

int add_train(int id, const char *name, const char *src, const char *dest, int capacity, double base_fare) {
    if (g_train_count >= MAX_TRAINS) {
        return 0; // RAM Array full
    }

    // Check if train ID already exists in RAM
    for (int i = 0; i < g_train_count; i++) {
        if (g_trains[i].train_id == id) {
            return 0; // Duplicate ID
        }
    }

    // Populate a new Train structure and add it to the RAM array
    Train new_t;
    new_t.train_id = id;
    snprintf(new_t.train_name, sizeof(new_t.train_name), "%s", name);
    snprintf(new_t.source, sizeof(new_t.source), "%s", src);
    snprintf(new_t.destination, sizeof(new_t.destination), "%s", dest);
    new_t.capacity = capacity;
    new_t.base_fare = base_fare;
    new_t.is_active = 1;

    g_trains[g_train_count++] = new_t;

    // Persist changes to disk
    if (!save_trains()) {
        return 0;
    }

    log_message("INFO", "Train added successfully");
    return 1;
}

void list_trains(void) {
    // Reads directly from RAM (super fast!)
    if (g_train_count == 0) {
        printf("No trains found.\n");
        return;
    }

    printf("\n=== Train Master List ===\n");
    for (int i = 0; i < g_train_count; i++) {
        if (g_trains[i].is_active) {
            printf("ID: %d | Name: %s | From: %s | To: %s | Cap: %d | Fare: Rs. %.2f\n",
                   g_trains[i].train_id, g_trains[i].train_name, 
                   g_trains[i].source, g_trains[i].destination, 
                   g_trains[i].capacity, g_trains[i].base_fare);
        }
    }
}

Train* find_train_by_id(int id) {
    // Directly search the RAM array and return a safe pointer to the element
    for (int i = 0; i < g_train_count; i++) {
        if (g_trains[i].train_id == id && g_trains[i].is_active) {
            return &g_trains[i];
        }
    }
    return NULL;
}