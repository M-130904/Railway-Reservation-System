#include "auth.h"
#include "logger.h"

int register_user(const char *username, const char *password, UserRole role) {
    FILE *f = fopen("data/users.txt", "r");
    char line[256];
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            char u_name[50];
            if (sscanf(line, "%49[^|]", u_name) == 1) {
                if (strcmp(u_name, username) == 0) {
                    fclose(f);
                    return 0;
                }
            }
        }
        fclose(f);
    }

    f = fopen("data/users.txt", "a");
    if (!f) return 0;
    fprintf(f, "%s|%s|%d\n", username, password, role);
    fclose(f);
    log_message("INFO", "User registration successful");
    return 1;
}

int authenticate_user(const char *username, const char *password, UserRole *role) {
    FILE *f = fopen("data/users.txt", "r");
    if (!f) return 0;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char u_name[50], u_pass[50];
        int u_role;
        if (sscanf(line, "%49[^|]|%49[^|]|%d", u_name, u_pass, &u_role) == 3) {
            if (strcmp(u_name, username) == 0 && strcmp(u_pass, password) == 0) {
                *role = (UserRole)u_role;
                fclose(f);
                log_message("INFO", "Authentication successful");
                return 1;
            }
        }
    }
    fclose(f);
    log_message("WARNING", "Authentication failed");
    return 0;
}