#include "schedule.h"
#include "logger.h"
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

/* 1. INITIALIZATION: Load data from file into RAM once at startup */
int init_schedules(void) {
    FILE *f = fopen(SCHEDULES_FILE, "r");
    g_schedule_count = 0; // Reset count
    if (!f) return 0;
    
    char line[LINE_LENGTH];
    while (g_schedule_count < MAX_SCHEDULES && fgets(line, sizeof(line), f)) {
        Schedule s;
        if (sscanf(line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                   &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                   &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                   &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked, &s.is_active) >= 13) {
            g_schedules[g_schedule_count++] = s;
        }
    }
    fclose(f);
    return g_schedule_count;
}

/* 2. PERSISTENCE: Save RAM array back to disk */
int save_schedules(void) {
    FILE *f = fopen(SCHEDULES_FILE, "w");
    if (!f) return 0;
    
    for (int i = 0; i < g_schedule_count; i++) {
        fprintf(f, "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                g_schedules[i].schedule_id, 
                g_schedules[i].train_id, 
                g_schedules[i].journey_date, 
                g_schedules[i].departure, 
                g_schedules[i].arrival,
                g_schedules[i].ac1_cap, 
                g_schedules[i].ac2_cap, 
                g_schedules[i].ac3_cap, 
                g_schedules[i].sl_cap,
                g_schedules[i].ac1_booked, 
                g_schedules[i].ac2_booked, 
                g_schedules[i].ac3_booked, 
                g_schedules[i].sl_booked, 
                g_schedules[i].is_active);
    }
    fclose(f);
    return 1;
}

/* 3. OPERATIONS: Instant RAM manipulation */

int add_schedule(int sched_id, int train_id, const char *date, const char *dep, const char *arr, int ac1, int ac2, int ac3, int sl) {
    if (g_schedule_count >= MAX_SCHEDULES) {
        return 0; // RAM full
    }

    // Check for duplicate schedule ID in RAM
    for (int i = 0; i < g_schedule_count; i++) {
        if (g_schedules[i].schedule_id == sched_id) {
            return 0; // Duplicate ID
        }
    }

    // Populate new schedule structure in RAM
    Schedule new_s;
    new_s.schedule_id = sched_id;
    new_s.train_id = train_id;
    snprintf(new_s.journey_date, sizeof(new_s.journey_date), "%s", date);
    snprintf(new_s.departure, sizeof(new_s.departure), "%s", dep);
    snprintf(new_s.arrival, sizeof(new_s.arrival), "%s", arr);
    new_s.ac1_cap = ac1;
    new_s.ac2_cap = ac2;
    new_s.ac3_cap = ac3;
    new_s.sl_cap = sl;
    new_s.ac1_booked = 0;
    new_s.ac2_booked = 0;
    new_s.ac3_booked = 0;
    new_s.sl_booked = 0;
    new_s.is_active = 1;

    g_schedules[g_schedule_count++] = new_s;

    // Save changes to disk
    if (!save_schedules()) {
        return 0;
    }

    log_message("INFO", "Schedule added successfully");
    return 1;
}

void list_schedules(void) {
    // Reads directly from RAM (lightning fast!)
    if (g_schedule_count == 0) {
        printf("No schedules found.\n");
        return;
    }

    printf("\n=== Schedule Master List ===\n");
    for (int i = 0; i < g_schedule_count; i++) {
        if (g_schedules[i].is_active) {
            printf("Sched ID: %d | Train ID: %d | Date: %s | Dep: %s | Arr: %s | AC1: %d | AC2: %d | AC3: %d | SL: %d\n",
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

Schedule* find_schedule_by_id(int sched_id) {
    // Search the RAM array directly and return a pointer
    for (int i = 0; i < g_schedule_count; i++) {
        if (g_schedules[i].schedule_id == sched_id && g_schedules[i].is_active) {
            return &g_schedules[i];
        }
    }
    return NULL;
}