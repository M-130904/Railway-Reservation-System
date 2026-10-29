#ifndef UTILS_H
#define UTILS_H
#include "types.h"
void clear_input_buffer(void);
void safe_fgets(char *buffer, int size);
void generate_pnr(char *pnr_buffer);
#endif