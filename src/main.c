#include "auth.h"
#include "admin.h"
#include "passenger.h"
#include "file_io.h"
#include "utils.h"

int main(void) {
    init_data_files();
    register_user("admin", "12345678", ROLE_ADMIN);

    int choice;
    do {
        printf("\n==================================\n");
        printf("   Railway Reservation System     \n");
        printf("==================================\n");
        printf("1. Passenger Registration\n");
        printf("2. Passenger Login\n");
        printf("3. Administrator Login\n");
        printf("4. Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (choice == 1) {
            char username[50], password[50];
            printf("Enter Username: "); safe_fgets(username, sizeof(username));
            printf("Enter Password: "); safe_fgets(password, sizeof(password));
            if (register_user(username, password, ROLE_PASSENGER)) {
                printf("[SUCCESS] Registration successful! Please login.\n");
            } else {
                printf("[ERROR] User already exists.\n");
            }
        } else if (choice == 2 || choice == 3) {
            char username[50], password[50];
            printf("Username: "); safe_fgets(username, sizeof(username));
            printf("Password: "); safe_fgets(password, sizeof(password));

            UserRole role;
            if (authenticate_user(username, password, &role)) {
                if (role == ROLE_ADMIN && choice == 3) {
                    admin_menu();
                } else if (role == ROLE_PASSENGER && choice == 2) {
                    passenger_menu(username);
                } else {
                    printf("[ACCESS DENIED] Role mismatch.\n");
                }
            } else {
                printf("[ERROR] Invalid credentials.\n");
            }
        }
    } while (choice != 4);

    printf("Exiting system. Goodbye!\n");
    return 0;
}