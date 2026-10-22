#include "train.h"
#include "logger.h"

int add_train(int id, const char *name, const char *src, const char *dest, int capacity, double base_fare) {
    FILE *f = fopen("data/trains.txt", "a");
    if (!f) return 0;
    fprintf(f, "%d|%s|%s|%s|%d|%.2f|1\n", id, name, src, dest, capacity, base_fare);
    fclose(f);
    log_message("INFO", "Train added successfully");
    return 1;
}

void list_trains(void) {
    FILE *f = fopen("data/trains.txt", "r");
    if (!f) { printf("No trains found.\n"); return; }
    char line[256];
    printf("\n=== Train Master List ===\n");
    while (fgets(line, sizeof(line), f)) {
        Train t;
        if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d", 
                   &t.train_id, t.train_name, t.source, t.destination, &t.capacity, &t.base_fare, &t.is_active) == 7) {
            printf("ID: %d | Name: %s | From: %s | To: %s | Cap: %d | Fare: Rs. %.2f\n",
                   t.train_id, t.train_name, t.source, t.destination, t.capacity, t.base_fare);
        }
    }
    fclose(f);
}

Train* find_train_by_id(int id) {
    static Train t;
    FILE *f = fopen("data/trains.txt", "r");
    if (!f) return NULL;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d", 
                   &t.train_id, t.train_name, t.source, t.destination, &t.capacity, &t.base_fare, &t.is_active) == 7) {
            if (t.train_id == id && t.is_active) {
                fclose(f);
                return &t;
            }
        }
    }
    fclose(f);
    return NULL;
}