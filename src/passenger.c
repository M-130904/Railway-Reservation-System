#include "passenger.h"
#include "search.h"
#include "booking.h"
#include "cancellation.h"
#include "utils.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* MISRA 21.6 deviation: stdio used for console UI (documented) */

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
    /* MISRA 11.5 fix: implicit conversion from void* to object pointer
       is allowed in C; the explicit cast triggers the violation. */
    BookingThreadArgs *args = arg;

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
        (void)printf("\n=== Passenger Menu ===\n");
        (void)printf("1. Search Trains Between Stations\n");
        (void)printf("2. Book Ticket & Pay\n");
        (void)printf("3. View Booking by PNR\n");
        (void)printf("4. Cancel Ticket\n");
        (void)printf("5. View Booking History\n");
        (void)printf("6. Logout\n");
        (void)printf("Enter choice: ");
        (void)fflush(stdout);

        if (scanf("%d", &choice) != 1) {
            (void)printf("[ERROR] Please enter a valid menu option.\n");
            clear_input_buffer();
            choice = 0;
            continue;
        }

        clear_input_buffer();

        if (choice == 1) {
            char src[20];
            char dest[20];
            char date[12];

            (void)printf("Source City: ");
            (void)fflush(stdout);
            safe_fgets(src, sizeof(src));

            (void)printf("Destination City: ");
            (void)fflush(stdout);
            safe_fgets(dest, sizeof(dest));

            (void)printf("Journey Date (YYYY-MM-DD): ");
            (void)fflush(stdout);
            safe_fgets(date, sizeof(date));

            search_trains_between_stations(src, dest, date);
        }
        else if (choice == 2) {
            int train_id;
            int sched_id;
            int class_choice;
            int p_count;

            (void)printf("Train ID: ");
            (void)fflush(stdout);
            if (scanf("%d", &train_id) != 1) {
                (void)printf("[ERROR] Invalid train ID.\n");
                clear_input_buffer();
                continue;
            }

            (void)printf("Schedule ID: ");
            (void)fflush(stdout);
            if (scanf("%d", &sched_id) != 1) {
                (void)printf("[ERROR] Invalid schedule ID.\n");
                clear_input_buffer();
                continue;
            }

            (void)printf("Class (1:AC1, 2:AC2, 3:AC3, 4:Sleeper): ");
            (void)fflush(stdout);
            if (scanf("%d", &class_choice) != 1) {
                (void)printf("[ERROR] Invalid class selection.\n");
                clear_input_buffer();
                continue;
            }

            (void)printf("Number of Passengers (max %d): ", MAX_PASSENGERS);
            (void)fflush(stdout);
            if (scanf("%d", &p_count) != 1) {
                (void)printf("[ERROR] Invalid passenger count.\n");
                clear_input_buffer();
                continue;
            }

            clear_input_buffer();

            /* Validate before accessing the passenger array */
            if ((p_count < 1) || (p_count > MAX_PASSENGERS) ||
                (class_choice < 1) || (class_choice > 4)) {
                (void)printf("[ERROR] Invalid passenger count or class.\n");
                continue;
            }

            PassengerDetail passengers[MAX_PASSENGERS];
            int input_error = 0;

            for (int i = 0; (i < p_count) && (input_error == 0); i++) {
                (void)printf("Passenger %d Name: ", i + 1);
                (void)fflush(stdout);
                safe_fgets(passengers[i].name,
                           sizeof(passengers[i].name));

                (void)printf("Passenger %d Age: ", i + 1);
                (void)fflush(stdout);

                if (scanf("%d", &passengers[i].age) != 1) {
                    (void)printf("[ERROR] Invalid passenger age.\n");
                    clear_input_buffer();
                    input_error = 1;
                }
                else {
                    clear_input_buffer();

                    (void)printf("Passenger %d Gender (M/F/O): ", i + 1);
                    (void)fflush(stdout);

                    if (scanf(" %c", &passengers[i].gender) != 1) {
                        (void)printf("[ERROR] Invalid passenger gender input.\n");
                        clear_input_buffer();
                        input_error = 1;
                    }
                    else {
                        clear_input_buffer();

                        if ((passengers[i].age < 1) ||
                            ((passengers[i].gender != 'M') &&
                             (passengers[i].gender != 'F') &&
                             (passengers[i].gender != 'O'))) {
                            (void)printf("[ERROR] Invalid passenger age or gender.\n");
                            input_error = 1;
                        }
                    }
                }
            }

            if (input_error != 0) {
                continue;
            }

            /* Prepare data for the booking worker thread */
            Ticket t;

            BookingThreadArgs args = {
                username,
                train_id,
                sched_id,
                (TrainClass)class_choice,   /* MISRA 10.3/10.5 ok: cast is explicit */
                p_count,
                passengers,
                &t,
                0
            };

            pthread_t booking_tid;

            /* Start booking in a separate thread */
            if (pthread_create(&booking_tid, NULL,
                               booking_thread, &args) != 0) {
                (void)printf("[ERROR] Could not create booking thread.\n");
                continue;
            }

            /* Wait until the booking thread completes */
            if (pthread_join(booking_tid, NULL) != 0) {
                (void)printf("[ERROR] Could not wait for booking thread.\n");
                continue;
            }

            if (args.result != 0) {
                (void)printf("\n[SUCCESS] Booking processed! PNR: %s | Status: %s\n",
                             t.pnr,
                             (t.status == STATUS_CONFIRMED)
                                 ? "CONFIRMED"
                                 : "WAITLIST");
            }
            else {
                (void)printf("[ERROR] Booking failed.\n");
            }
        }
        else if (choice == 3) {
            char pnr[MAX_PNR_LEN];

            (void)printf("Enter PNR: ");
            (void)fflush(stdout);
            safe_fgets(pnr, sizeof(pnr));

            view_booking_by_pnr(pnr);
        }
        else if (choice == 4) {
            char pnr[MAX_PNR_LEN];

            (void)printf("Enter PNR to cancel: ");
            (void)fflush(stdout);
            safe_fgets(pnr, sizeof(pnr));

            if (cancel_ticket(pnr) != 0) {
                (void)printf("[SUCCESS] Ticket cancelled and refund processed.\n");
            }
            else {
                (void)printf("[ERROR] Cancellation failed.\n");
            }
        }
        else if (choice == 5) {
            view_user_booking_history(username);
        }
        else if (choice != 6) {
            (void)printf("[ERROR] Invalid menu option.\n");
        }
        else {
            /* choice 6 (logout): loop condition ends the menu */
        }

    } while (choice != 6);
}