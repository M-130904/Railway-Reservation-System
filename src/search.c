#include "search.h"
#include "train.h"
#include "schedule.h"

void search_trains_between_stations(const char *src, const char *dest, const char *date) {
    FILE *ft = fopen("data/trains.txt", "r");
    FILE *fs = fopen("data/schedules.txt", "r");
    if (!ft || !fs) {
        if (ft) fclose(ft);
        if (fs) fclose(fs);
        printf("Database files missing.\n");
        return;
    }

    char t_line[256], s_line[256];
    int found = 0;
    printf("\n=== Search Results: %s to %s on %s ===\n", src, dest, date);

    while (fgets(t_line, sizeof(t_line), ft)) {
        Train t;
        if (sscanf(t_line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d", 
                   &t.train_id, t.train_name, t.source, t.destination, &t.capacity, &t.base_fare, &t.is_active) == 7) {
            if (t.is_active && strcasecmp(t.source, src) == 0 && strcasecmp(t.destination, dest) == 0) {
                rewind(fs);
                while (fgets(s_line, sizeof(s_line), fs)) {
                    Schedule s;
                    if (sscanf(s_line, "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                               &s.schedule_id, &s.train_id, s.journey_date, s.departure, s.arrival,
                               &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
                               &s.ac1_booked, &s.ac2_booked, &s.ac3_booked, &s.sl_booked, &s.is_active) >= 13) {
                        if (s.train_id == t.train_id && s.is_active && strcmp(s.journey_date, date) == 0) {
                            found = 1;
                            printf("Train ID: %d | Name: %s | Sched ID: %d | Dep: %s | Arr: %s | Base Fare: Rs. %.2f\n",
                                   t.train_id, t.train_name, s.schedule_id, s.departure, s.arrival, t.base_fare);
                        }
                    }
                }
            }
        }
    }
    if (!found) printf("No trains found for this route and date.\n");
    fclose(ft);
    fclose(fs);
}