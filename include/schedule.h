#ifndef SCHEDULE_H
#define SCHEDULE_H
#include "types.h"
int add_schedule(int sched_id, int train_id, const char *date, const char *dep, const char *arr, int ac1, int ac2, int ac3, int sl);
void list_schedules(void);
Schedule* find_schedule_by_id(int sched_id);
#endif