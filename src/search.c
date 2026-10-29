#include "search.h"
#include "train.h"
#include "schedule.h"
#include <stdio.h>
#include <string.h>
#include <strings.h> // for strcasecmp

#define _POSIX_C_SOURCE 200809L
#define MAX_TRAINS 100
#define MAX_SCHEDULES 500

void search_trains_between_stations(const char *src, const char *dest, const char *date) {
    FILE *ft = fopen("data/trains.txt", "r");
    FILE *fs = fopen("data/schedules.txt", "r");

    if ((ft == NULL) || (fs == NULL)) {
        if (ft != NULL) {
            (void)fclose(ft);
        }
        if (fs != NULL) {
            (void)fclose(fs);
        }
        (void)printf("[ERROR] Database files missing.\n");
    } else {
        char line[256];
        int found = 0;
        int train_count = 0;
        int schedule_count = 0;

        // 1. Organize and store train records using Structures and Arrays
        Train trains[MAX_TRAINS];

        while ((train_count < MAX_TRAINS) && (fgets(line, sizeof(line), ft) != NULL)) {
            Train t;

            if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d",
                       &t.train_id, t.train_name, t.source, t.destination,
                       &t.capacity, &t.base_fare, &t.is_active) == 7) {
                trains[train_count] = t;
                train_count++;
            }
        }
        (void)fclose(ft);

        // 2. Organize and store schedule records using Structures and Arrays
        Schedule schedules[MAX_SCHEDULES];

        while ((schedule_count < MAX_SCHEDULES) && (fgets(line, sizeof(line), fs) != NULL)) {
            Schedule s;

            if (sscanf(line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                       &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                       &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                       &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked,
                       &s.is_active) >= 13) {
                schedules[schedule_count] = s;
                schedule_count++;
            }
        }
        (void)fclose(fs);

        (void)printf("\n=== Search Results: %s to %s on %s ===\n", src, dest, date);

        // 3. Search through the in-memory arrays and structures cleanly (No file I/O inside loops)
        for (int i = 0; i < train_count; i++) {
            if ((trains[i].is_active != 0) &&
                (strcasecmp(trains[i].source, src) == 0) &&
                (strcasecmp(trains[i].destination, dest) == 0)) {

                for (int j = 0; j < schedule_count; j++) {
                    if ((schedules[j].train_id == trains[i].train_id) &&
                        (schedules[j].is_active != 0) &&
                        (strcmp(schedules[j].journey_date, date) == 0)) {

                        found = 1;
                        (void)printf("Train ID: %d | Name: %s | Sched ID: %d | Dep: %s | Arr: %s | Base Fare: Rs. %.2f\n",
                                     trains[i].train_id, trains[i].train_name, schedules[j].schedule_id,
                                     schedules[j].departure, schedules[j].arrival, trains[i].base_fare);
                    }
                }
            }
        }

        if (found == 0) {
            (void)printf("No trains found for this route and date.\n");
        }
    }
}