#ifndef TRAIN_H
#define TRAIN_H

#include "types.h"

// Initialization and Persistence
int init_trains(void);
int save_trains(void);

// Core Train Functions
int add_train(int id, const char *name, const char *src, const char *dest, int capacity, double base_fare);
void list_trains(void);
Train* find_train_by_id(int id);

#endif