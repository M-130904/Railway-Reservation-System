#include <stdio.h>
#include <stdlib.h>
#include <time.h> // <-- Required for time()

#include "train.h"
#include "schedule.h"
#include "auth.h"
#include "admin.h"
#include "passenger.h"
#include "file_io.h"
#include "utils.h"

int main(void) {
    int choice;

    srand(time(NULL)); // <-- Seeds the random number generator
    init_data_files();
    (void)init_trains();
    (void)init_schedules();
    (void)register_user("admin", "12345678", ROLE_ADMIN);

    do {
        (void)printf("\n==================================\n");
        (void)printf("    Railway Reservation System    \n");
        (void)printf("==================================\n");
        (void)printf("1. Passenger Registration\n");
        (void)printf("2. Passenger Login\n");
        (void)printf("3. Administrator Login\n");
        (void)printf("4. Exit\n");
        (void)printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (choice == 1) {
            char username[50];
            char password[50];

            (void)printf("Enter Username: ");
            safe_fgets(username, sizeof(username));
            (void)printf("Enter Password: ");
            safe_fgets(password, sizeof(password));
            if (register_user(username, password, ROLE_PASSENGER) != 0) {
                (void)printf("[SUCCESS] Registration successful! Please login.\n");
            } else {
                (void)printf("[ERROR] User already exists.\n");
            }
        } else if ((choice == 2) || (choice == 3)) {
            char username[50];
            char password[50];
            UserRole role;

            (void)printf("Username: ");
            safe_fgets(username, sizeof(username));
            (void)printf("Password: ");
            safe_fgets(password, sizeof(password));

            if (authenticate_user(username, password, &role) != 0) {
                if ((role == ROLE_ADMIN) && (choice == 3)) {
                    admin_menu();
                } else if ((role == ROLE_PASSENGER) && (choice == 2)) {
                    passenger_menu(username);
                } else {
                    (void)printf("[ACCESS DENIED] Role mismatch.\n");
                }
            } else {
                (void)printf("[ERROR] Invalid credentials.\n");
            }
        } else {
            /* choice 4 (exit) or unrecognised value: nothing to do */
        }
    } while (choice != 4);

    (void)printf("Exiting system. Goodbye!\n");
    return 0;
}