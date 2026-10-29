#ifndef AUTH_H
#define AUTH_H
#include "types.h"
int register_user(const char *username, const char *password, UserRole role);
int authenticate_user(const char *username, const char *password, UserRole *role);
#endif