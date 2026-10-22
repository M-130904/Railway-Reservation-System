
#include "admin.h"
#include "train.h"
#include "schedule.h"
#include "report.h"
#include "utils.h"

void admin_menu(void)
{
    int choice = 0;

    do {
        printf("\n=== Administrator Menu ===\n");
        printf("1. Add Train\n");
        printf("2. List Trains\n");
        printf("3. Add Train Schedule\n");
        printf("4. List Train Schedules\n");
        printf("5. Generate System Reports\n");
        printf("6. Logout\n");
        printf("Enter choice: ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("[ERROR] Invalid menu choice.\n");
            choice = 0;
            continue;
        }

        clear_input_buffer();

        if (choice == 1) {
            int id, capacity;
            char name[50], src[20], dest[20];
            double base_fare;

            printf("Train ID: ");
            fflush(stdout);
            if (scanf("%d", &id) != 1) {
                clear_input_buffer();
                printf("[ERROR] Invalid train ID.\n");
                continue;
            }
            clear_input_buffer();

            printf("Train Name: ");
            fflush(stdout);
            safe_fgets(name, sizeof(name));

            printf("Source: ");
            fflush(stdout);
            safe_fgets(src, sizeof(src));

            printf("Destination: ");
            fflush(stdout);
            safe_fgets(dest, sizeof(dest));

            printf("Capacity: ");
            fflush(stdout);
            if (scanf("%d", &capacity) != 1) {
                clear_input_buffer();
                printf("[ERROR] Invalid capacity.\n");
                continue;
            }

            printf("Base Fare: ");
            fflush(stdout);
            if (scanf("%lf", &base_fare) != 1) {
                clear_input_buffer();
                printf("[ERROR] Invalid base fare.\n");
                continue;
            }
            clear_input_buffer();

            if (add_train(id, name, src, dest, capacity, base_fare)) {
                printf("[SUCCESS] Train added!\n");
            } else {
                printf("[ERROR] Failed to add train.\n");
            }

        } else if (choice == 2) {
            list_trains();

        } 
         else if (choice == 3) {
            int sched_id, train_id, ac1, ac2, ac3, sl;
            char date[12], dep[6], arr[6];

            printf("Schedule ID: ");
            fflush(stdout);
            if (scanf("%d", &sched_id) != 1) {
                clear_input_buffer();
                printf("[ERROR] Invalid schedule ID.\n");
                continue;
            }

            printf("Train ID: ");
            fflush(stdout);
            if (scanf("%d", &train_id) != 1) {
                clear_input_buffer();
                printf("[ERROR] Invalid train ID.\n");
                continue;
            }

            printf("Journey Date (YYYY-MM-DD): ");
            fflush(stdout);
            if (scanf("%11s", date) != 1) {
                clear_input_buffer();
                printf("[ERROR] Failed to read journey date.\n");
                continue;
            }

            printf("Departure (HH:MM): ");
            fflush(stdout);
            if (scanf("%5s", dep) != 1) {
                clear_input_buffer();
                printf("[ERROR] Failed to read departure time.\n");
                continue;
            }

            printf("Arrival (HH:MM): ");
            fflush(stdout);
            if (scanf("%5s", arr) != 1) {
                clear_input_buffer();
                printf("[ERROR] Failed to read arrival time.\n");
                continue;
            }

            printf("Capacities [AC1 AC2 AC3 Sleeper]: ");
            fflush(stdout);
            if (scanf("%d %d %d %d", &ac1, &ac2, &ac3, &sl) != 4) {
                clear_input_buffer();
                printf("[ERROR] Invalid capacity values.\n");
                continue;
            }
            clear_input_buffer();

            if (add_schedule(sched_id, train_id, date, dep, arr,
                             ac1, ac2, ac3, sl)) {
                printf("[SUCCESS] Schedule added!\n");
            } else {
                printf("[ERROR] Failed to add schedule.\n");
            }
        } else if (choice == 4) {
            list_schedules();

        } else if (choice == 5) {
            generate_system_reports();

        } else if (choice != 6) {
            printf("[ERROR] Please select a valid option.\n");
        }

    } while (choice != 6);
}
