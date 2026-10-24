#include "train.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

#define MAX_TRAINS 100

/* Helper function to load file records into an array of structures */
static int load_trains_array(Train trains[], int max_size) {
    FILE *f = fopen("data/trains.txt", "r");
    if (!f) return 0;
    
    char line[256];
    int count = 0;
    while (count < max_size && fgets(line, sizeof(line), f)) {
        Train t;
        if (sscanf(line, "%d|%49[^|]|%19[^|]|%19[^|]|%d|%lf|%d", 
                   &t.train_id, t.train_name, t.source, t.destination, &t.capacity, &t.base_fare, &t.is_active) == 7) {
            trains[count++] = t;
        }
    }
    fclose(f);
    return count;
}

/* Helper function to save the array of structures back to the file */
static int save_trains_array(const Train trains[], int count) {
    FILE *f = fopen("data/trains.txt", "w");
    if (!f) return 0;
    
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d|%s|%s|%s|%d|%.2f|%d\n", 
                trains[i].train_id, trains[i].train_name, 
                trains[i].source, trains[i].destination, 
                trains[i].capacity, trains[i].base_fare, 
                trains[i].is_active);
    }
    fclose(f);
    return 1;
}

int add_train(int id, const char *name, const char *src, const char *dest, int capacity, double base_fare) {
    Train trains[MAX_TRAINS];
    int count = load_trains_array(trains, MAX_TRAINS);

    if (count >= MAX_TRAINS) {
        return 0; // Array full
    }

    // Check if train ID already exists in the array
    for (int i = 0; i < count; i++) {
        if (trains[i].train_id == id) {
            return 0; // Duplicate ID
        }
    }

    // Populate a new Train structure and add it to the array
    Train new_t;
    new_t.train_id = id;
    snprintf(new_t.train_name, sizeof(new_t.train_name), "%s", name);
    snprintf(new_t.source, sizeof(new_t.source), "%s", src);
    snprintf(new_t.destination, sizeof(new_t.destination), "%s", dest);
    new_t.capacity = capacity;
    new_t.base_fare = base_fare;
    new_t.is_active = 1;

    trains[count++] = new_t;

    // Save the updated array back to disk
    if (!save_trains_array(trains, count)) {
        return 0;
    }

    log_message("INFO", "Train added successfully");
    return 1;
}

void list_trains(void) {
    Train trains[MAX_TRAINS];
    int count = load_trains_array(trains, MAX_TRAINS);

    if (count == 0) {
        printf("No trains found.\n");
        return;
    }

    printf("\n=== Train Master List ===\n");
    for (int i = 0; i < count; i++) {
        if (trains[i].is_active) {
            printf("ID: %d | Name: %s | From: %s | To: %s | Cap: %d | Fare: Rs. %.2f\n",
                   trains[i].train_id, trains[i].train_name, 
                   trains[i].source, trains[i].destination, 
                   trains[i].capacity, trains[i].base_fare);
        }
    }
}

Train* find_train_by_id(int id) {
    // Using a static array so returning a pointer to an element is safe
    static Train trains[MAX_TRAINS];
    int count = load_trains_array(trains, MAX_TRAINS);

    for (int i = 0; i < count; i++) {
        if (trains[i].train_id == id && trains[i].is_active) {
            return &trains[i];
        }
    }
    return NULL;
}