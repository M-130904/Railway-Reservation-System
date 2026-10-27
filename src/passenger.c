
#include "passenger.h"
#include "search.h"
#include "booking.h"
#include "cancellation.h"
#include "utils.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* Arguments passed to the booking worker thread */
typedef struct {
    const char *username;
    int train_id;
    int schedule_id;
    TrainClass t_class;
    int p_count;
    PassengerDetail *passengers;
    Ticket *ticket;
    int result;
} BookingThreadArgs;

/* Worker thread executes the booking operation */
static void *booking_thread(void *arg)
{
    BookingThreadArgs *args = (BookingThreadArgs *)arg;

    args->result = book_ticket(
        args->username,
        args->train_id,
        args->schedule_id,
        args->t_class,
        args->p_count,
        args->passengers,
        args->ticket
    );

    return NULL;
}

void passenger_menu(const char *username)
{
    int choice = 0;

    do {
        printf("\n=== Passenger Menu ===\n");
        printf("1. Search Trains Between Stations\n");
        printf("2. Book Ticket & Pay\n");
        printf("3. View Booking by PNR\n");
        printf("4. Cancel Ticket\n");
        printf("5. View Booking History\n");
        printf("6. Logout\n");
        printf("Enter choice: ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1) {
            printf("[ERROR] Please enter a valid menu option.\n");
            clear_input_buffer();
            choice = 0;
            continue;
        }

        clear_input_buffer();

        if (choice == 1) {
            char src[20], dest[20], date[12];

            printf("Source City: ");
            fflush(stdout);
            safe_fgets(src, sizeof(src));

            printf("Destination City: ");
            fflush(stdout);
            safe_fgets(dest, sizeof(dest));

            printf("Journey Date (YYYY-MM-DD): ");
            fflush(stdout);
            safe_fgets(date, sizeof(date));

            search_trains_between_stations(src, dest, date);
        }
        else if (choice == 2) {
            int train_id, sched_id, class_choice, p_count;

            printf("Train ID: ");
            fflush(stdout);
            if (scanf("%d", &train_id) != 1) {
                printf("[ERROR] Invalid train ID.\n");
                clear_input_buffer();
                continue;
            }

            printf("Schedule ID: ");
            fflush(stdout);
            if (scanf("%d", &sched_id) != 1) {
                printf("[ERROR] Invalid schedule ID.\n");
                clear_input_buffer();
                continue;
            }

            printf("Class (1:AC1, 2:AC2, 3:AC3, 4:Sleeper): ");
            fflush(stdout);
            if (scanf("%d", &class_choice) != 1) {
                printf("[ERROR] Invalid class selection.\n");
                clear_input_buffer();
                continue;
            }

            printf("Number of Passengers (max %d): ", MAX_PASSENGERS);
            fflush(stdout);
            if (scanf("%d", &p_count) != 1) {
                printf("[ERROR] Invalid passenger count.\n");
                clear_input_buffer();
                continue;
            }

            clear_input_buffer();

            /* Validate before accessing the passenger array */
            if (p_count < 1 || p_count > MAX_PASSENGERS ||
                class_choice < 1 || class_choice > 4) {
                printf("[ERROR] Invalid passenger count or class.\n");
                continue;
            }

            PassengerDetail passengers[MAX_PASSENGERS];
            int input_error = 0;

            for (int i = 0; i < p_count; i++) {
                printf("Passenger %d Name: ", i + 1);
                fflush(stdout);
                safe_fgets(passengers[i].name,
                           sizeof(passengers[i].name));

                printf("Passenger %d Age: ", i + 1);
                fflush(stdout);

                if (scanf("%d", &passengers[i].age) != 1) {
                    printf("[ERROR] Invalid passenger age.\n");
                    clear_input_buffer();
                    input_error = 1;
                    break;
                }

                clear_input_buffer();

                printf("Passenger %d Gender (M/F/O): ", i + 1);
                fflush(stdout);

                if (scanf(" %c", &passengers[i].gender) != 1) {
                    printf("[ERROR] Invalid passenger gender input.\n");
                    clear_input_buffer();
                    input_error = 1;
                    break;
                }

                clear_input_buffer();

                if (passengers[i].age < 1 ||
                    (passengers[i].gender != 'M' &&
                     passengers[i].gender != 'F' &&
                     passengers[i].gender != 'O')) {
                    printf("[ERROR] Invalid passenger age or gender.\n");
                    input_error = 1;
                    break;
                }
            }

            if (input_error) {
                continue;
            }

            /* Prepare data for the booking worker thread */
            Ticket t;

            BookingThreadArgs args = {
                username,
                train_id,
                sched_id,
                (TrainClass)class_choice,
                p_count,
                passengers,
                &t,
                0
            };

            pthread_t booking_tid;

            /* Start booking in a separate thread */
            if (pthread_create(&booking_tid, NULL,
                               booking_thread, &args) != 0) {
                printf("[ERROR] Could not create booking thread.\n");
                continue;
            }

            /* Wait until the booking thread completes */
            if (pthread_join(booking_tid, NULL) != 0) {
                printf("[ERROR] Could not wait for booking thread.\n");
                continue;
            }

            if (args.result) {
                printf("\n[SUCCESS] Booking processed! PNR: %s | Status: %s\n",
                       t.pnr,
                       t.status == STATUS_CONFIRMED
                           ? "CONFIRMED"
                           : "WAITLIST");
            }
            else {
                printf("[ERROR] Booking failed.\n");
            }
        }
        else if (choice == 3) {
            char pnr[MAX_PNR_LEN];

            printf("Enter PNR: ");
            fflush(stdout);
            safe_fgets(pnr, sizeof(pnr));

            view_booking_by_pnr(pnr);
        }
        else if (choice == 4) {
            char pnr[MAX_PNR_LEN];

            printf("Enter PNR to cancel: ");
            fflush(stdout);
            safe_fgets(pnr, sizeof(pnr));

            if (cancel_ticket(pnr)) {
                printf("[SUCCESS] Ticket cancelled and refund processed.\n");
            }
            else {
                printf("[ERROR] Cancellation failed.\n");
            }
        }
        else if (choice == 5) {
            view_user_booking_history(username);
        }
        else if (choice != 6) {
            printf("[ERROR] Invalid menu option.\n");
        }

    } while (choice != 6);
}
