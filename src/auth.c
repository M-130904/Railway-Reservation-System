#include "auth.h"
#include "logger.h"

int register_user(const char *username, const char *password, UserRole role) {
    int result = 0;
    int duplicate = 0;
    char line[256];
    FILE *f = fopen("data/users.txt", "r");

    if (f != NULL) {                                        /* 14.4 */
        while ((duplicate == 0) &&
               (fgets(line, sizeof(line), f) != NULL)) {    /* 14.4 */
            char u_name[50];
            if (sscanf(line, "%49[^|]", u_name) == 1) {
                if (strcmp(u_name, username) == 0) {
                    duplicate = 1;                          /* 15.5: no early return */
                }
            }
        }
        (void)fclose(f);                                    /* 17.7 */
    }

    if (duplicate == 0) {
        f = fopen("data/users.txt", "a");
        if (f != NULL) {                                    /* 14.4 */
            (void)fprintf(f, "%s|%s|%d\n", username, password,
                          (int)role);                       /* 17.7, enum to int for %d */
            (void)fclose(f);                                /* 17.7 */
            log_message("INFO", "User registration successful");
            result = 1;
        }
    }

    return result;                                          /* 15.5: single exit */
}

int authenticate_user(const char *username, const char *password, UserRole *role) {
    int result = 0;
    //int found = 0;
    char line[256];
    FILE *f = fopen("data/users.txt", "r");

    if (f != NULL) { 
        int found = 0;                                       /* 14.4 */
        while ((found == 0) &&
               (fgets(line, sizeof(line), f) != NULL)) {    /* 14.4 */
            char u_name[50];                                /* 12.3: split declarations */
            char u_pass[50];
            int u_role;
            if (sscanf(line, "%49[^|]|%49[^|]|%d", u_name, u_pass, &u_role) == 3) {
                if ((strcmp(u_name, username) == 0) &&
                    (strcmp(u_pass, password) == 0)) {      /* 12.1: parentheses */
                    *role = (UserRole)u_role;
                    found = 1;
                }
            }
        }
        (void)fclose(f);                                    /* 17.7 */

        if (found != 0) {
            log_message("INFO", "Authentication successful");
            result = 1;
        } else {
            log_message("WARNING", "Authentication failed");
        }
    }

    return result;                                          /* 15.5: single exit */
}