#include "admin.h"
#include "train.h"
#include "schedule.h"
#include "report.h"
#include "utils.h"

void admin_menu(void)
{
    int choice = 0;
    int input_ended = 0;

    do {
        (void)printf("\n=== Administrator Menu ===\n");              /* 17.7 */
        (void)printf("1. Add Train\n");                              /* 17.7 */
        (void)printf("2. List Trains\n");                            /* 17.7 */
        (void)printf("3. Add Train Schedule\n");                     /* 17.7 */
        (void)printf("4. List Train Schedules\n");                   /* 17.7 */
        (void)printf("5. Generate System Reports\n");                /* 17.7 */
        (void)printf("6. Logout\n");                                 /* 17.7 */
        (void)printf("Enter choice: ");                              /* 17.7 */
        (void)fflush(stdout);                                        /* 17.7 */

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();

            if (feof(stdin) != 0) {
                input_ended = 1;
            } else {
                (void)printf("[ERROR] Invalid menu choice.\n");      /* 17.7 */
                choice = 0;
            }
        } else {
            clear_input_buffer();

            if (choice == 1) {
                int id = 0;
                int capacity = 0;
                double base_fare = 0.0;

                (void)printf("Train ID: ");                          /* 17.7 */
                (void)fflush(stdout);                                /* 17.7 */

                if (scanf("%d", &id) != 1) {
                    clear_input_buffer();
                    (void)printf("[ERROR] Invalid train ID.\n");     /* 17.7 */
                } else {
                    char name[50];                                   /* variableScope */
                    char src[20];                                    /* variableScope */
                    char dest[20];                                   /* variableScope */

                    clear_input_buffer();

                    (void)printf("Train Name: ");                    /* 17.7 */
                    (void)fflush(stdout);                            /* 17.7 */
                    safe_fgets(name, sizeof(name));

                    (void)printf("Source: ");                        /* 17.7 */
                    (void)fflush(stdout);                            /* 17.7 */
                    safe_fgets(src, sizeof(src));

                    (void)printf("Destination: ");                   /* 17.7 */
                    (void)fflush(stdout);                            /* 17.7 */
                    safe_fgets(dest, sizeof(dest));

                    (void)printf("Capacity: ");                      /* 17.7 */
                    (void)fflush(stdout);                            /* 17.7 */

                    if (scanf("%d", &capacity) != 1) {
                        clear_input_buffer();
                        (void)printf("[ERROR] Invalid capacity.\n"); /* 17.7 */
                    } else {
                        (void)printf("Base Fare: ");                 /* 17.7 */
                        (void)fflush(stdout);                        /* 17.7 */

                        if (scanf("%lf", &base_fare) != 1) {
                            clear_input_buffer();
                            (void)printf("[ERROR] Invalid base fare.\n"); /* 17.7 */
                        } else {
                            clear_input_buffer();

                            if (add_train(id, name, src, dest,
                                          capacity, base_fare) != 0) {
                                (void)printf("[SUCCESS] Train added!\n");      /* 17.7 */
                            } else {
                                (void)printf("[ERROR] Failed to add train.\n"); /* 17.7 */
                            }
                        }
                    }
                }
            } else if (choice == 2) {
                list_trains();
            } else if (choice == 3) {
                int sched_id = 0;
                int train_id = 0;
                int ac1 = 0;
                int ac2 = 0;
                int ac3 = 0;
                int sl = 0;

                (void)printf("Schedule ID: ");                       /* 17.7 */
                (void)fflush(stdout);                                /* 17.7 */

                if (scanf("%d", &sched_id) != 1) {
                    clear_input_buffer();
                    (void)printf("[ERROR] Invalid schedule ID.\n");  /* 17.7 */
                } else {
                    (void)printf("Train ID: ");                      /* 17.7 */
                    (void)fflush(stdout);                            /* 17.7 */

                    if (scanf("%d", &train_id) != 1) {
                        clear_input_buffer();
                        (void)printf("[ERROR] Invalid train ID.\n"); /* 17.7 */
                    } else {
                        char date[12];                               /* variableScope */

                        (void)printf("Journey Date (YYYY-MM-DD): "); /* 17.7 */
                        (void)fflush(stdout);                        /* 17.7 */

                        if (scanf("%11s", date) != 1) {
                            clear_input_buffer();

                            if (feof(stdin) != 0) {
                                input_ended = 1;
                            } else {
                                (void)printf("[ERROR] Failed to read journey date.\n"); /* 17.7 */
                            }
                        } else {
                            char dep[6];                             /* variableScope */

                            (void)printf("Departure (HH:MM): ");     /* 17.7 */
                            (void)fflush(stdout);                    /* 17.7 */

                            if (scanf("%5s", dep) != 1) {
                                clear_input_buffer();

                                if (feof(stdin) != 0) {
                                    input_ended = 1;
                                } else {
                                    (void)printf("[ERROR] Failed to read departure time.\n"); /* 17.7 */
                                }
                            } else {
                                char arr[6];                         /* variableScope */

                                (void)printf("Arrival (HH:MM): ");   /* 17.7 */
                                (void)fflush(stdout);                /* 17.7 */

                                if (scanf("%5s", arr) != 1) {
                                    clear_input_buffer();

                                    if (feof(stdin) != 0) {
                                        input_ended = 1;
                                    } else {
                                        (void)printf("[ERROR] Failed to read arrival time.\n"); /* 17.7 */
                                    }
                                } else {
                                    (void)printf("Capacities [AC1 AC2 AC3 Sleeper]: "); /* 17.7 */
                                    (void)fflush(stdout);            /* 17.7 */

                                    if (scanf("%d %d %d %d",
                                              &ac1, &ac2, &ac3, &sl) != 4) {
                                        clear_input_buffer();
                                        (void)printf("[ERROR] Invalid capacity values.\n"); /* 17.7 */
                                    } else {
                                        clear_input_buffer();

                                        if (add_schedule(
                                                sched_id, train_id, date, dep,
                                                arr, ac1, ac2, ac3, sl) != 0) {
                                            (void)printf("[SUCCESS] Schedule added!\n");      /* 17.7 */
                                        } else {
                                            (void)printf("[ERROR] Failed to add schedule.\n"); /* 17.7 */
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (choice == 4) {
                list_schedules();
            } else if (choice == 5) {
                generate_system_reports();
            } else if ((choice != 6) && (input_ended == 0)) {
                (void)printf("[ERROR] Please select a valid option.\n"); /* 17.7 */
            } else {
                /* 15.7: final else, no action (choice 6 or input ended) */
            }
        }
    } while ((choice != 6) && (input_ended == 0));
}