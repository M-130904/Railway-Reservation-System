#include "schedule.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

#define MAX_SCHEDULES 200
#define SCHEDULES_FILE "data/schedules.txt"
#define LINE_LENGTH 256

/* Helper function to load schedules into an in-memory array */
static int load_schedules_array(Schedule schedules[], int max_size) {
    FILE *f = fopen(SCHEDULES_FILE, "r");
    if (!f) return 0;
    
    char line[LINE_LENGTH];
    int count = 0;
    
    while (count < max_size && fgets(line, sizeof(line), f)) {
        Schedule s;
        if (sscanf(line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                   &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                   &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                   &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked, &s.is_active) >= 13) {
            schedules[count++] = s;
        }
    }
    fclose(f);
    return count;
}

/* Helper function to save the in-memory schedule array back to disk */
static int save_schedules_array(const Schedule schedules[], int count) {
    FILE *f = fopen(SCHEDULES_FILE, "w");
    if (!f) return 0;
    
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                schedules[i].schedule_id, 
                schedules[i].train_id, 
                schedules[i].journey_date, 
                schedules[i].departure, 
                schedules[i].arrival,
                schedules[i].ac1_cap, 
                schedules[i].ac2_cap, 
                schedules[i].ac3_cap, 
                schedules[i].sl_cap,
                schedules[i].ac1_booked, 
                schedules[i].ac2_booked, 
                schedules[i].ac3_booked, 
                schedules[i].sl_booked, 
                schedules[i].is_active);
    }
    fclose(f);
    return 1;
}

int add_schedule(int sched_id, int train_id, const char *date, const char *dep, const char *arr, int ac1, int ac2, int ac3, int sl) {
    Schedule schedules[MAX_SCHEDULES];
    int count = load_schedules_array(schedules, MAX_SCHEDULES);

    if (count >= MAX_SCHEDULES) {
        return 0;
    }

    // Check for duplicate schedule ID
    for (int i = 0; i < count; i++) {
        if (schedules[i].schedule_id == sched_id) {
            return 0; // Duplicate ID
        }
    }

    // Populate new schedule structure
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

    schedules[count++] = new_s;

    // Save back to file cleanly
    if (!save_schedules_array(schedules, count)) {
        return 0;
    }

    log_message("INFO", "Schedule added successfully");
    return 1;
}

void list_schedules(void) {
    Schedule schedules[MAX_SCHEDULES];
    int count = load_schedules_array(schedules, MAX_SCHEDULES);

    if (count == 0) {
        printf("No schedules found.\n");
        return;
    }

    printf("\n=== Schedule Master List ===\n");
    for (int i = 0; i < count; i++) {
        if (schedules[i].is_active) {
            printf("Sched ID: %d | Train ID: %d | Date: %s | Dep: %s | Arr: %s | AC1: %d | AC2: %d | AC3: %d | SL: %d\n",
                   schedules[i].schedule_id, 
                   schedules[i].train_id, 
                   schedules[i].journey_date, 
                   schedules[i].departure, 
                   schedules[i].arrival,
                   schedules[i].ac1_cap,
                   schedules[i].ac2_cap,
                   schedules[i].ac3_cap,
                   schedules[i].sl_cap);
        }
    }
}

Schedule* find_schedule_by_id(int sched_id) {
    static Schedule schedules[MAX_SCHEDULES];
    int count = load_schedules_array(schedules, MAX_SCHEDULES);

    for (int i = 0; i < count; i++) {
        if (schedules[i].schedule_id == sched_id && schedules[i].is_active) {
            return &schedules[i];
        }
    }
    return NULL;
}