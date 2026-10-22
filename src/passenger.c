#include "passenger.h"
#include "search.h"
#include "booking.h"
#include "cancellation.h"
#include "utils.h"

void passenger_menu(const char *username) {
    int choice;
    do {
        printf("\n=== Passenger Menu ===\n");
        printf("1. Search Trains Between Stations\n");
        printf("2. Book Ticket & Pay\n");
        printf("3. View Booking by PNR\n");
        printf("4. Cancel Ticket\n");
        printf("5. View Booking History\n");
        printf("6. Logout\n");
        printf("Enter choice: "); fflush(stdout);
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (choice == 1) {
            char src[20], dest[20], date[12];
            printf("Source City: "); fflush(stdout); safe_fgets(src, sizeof(src));
            printf("Destination City: "); fflush(stdout); safe_fgets(dest, sizeof(dest));
            printf("Journey Date (YYYY-MM-DD): "); fflush(stdout); safe_fgets(date, sizeof(date));
            search_trains_between_stations(src, dest, date);
        } else if (choice == 2) {
            int train_id, sched_id, class_choice, p_count;
            printf("Train ID: "); fflush(stdout); scanf("%d", &train_id);
            printf("Schedule ID: "); fflush(stdout); scanf("%d", &sched_id);
            printf("Class (1:AC1, 2:AC2, 3:AC3, 4:Sleeper): "); fflush(stdout); scanf("%d", &class_choice);
            printf("Number of Passengers (max %d): ", MAX_PASSENGERS); fflush(stdout); scanf("%d", &p_count);
            clear_input_buffer();

            PassengerDetail passengers[MAX_PASSENGERS];
            for (int i = 0; i < p_count; i++) {
                printf("Passenger %d Name: ", i + 1); fflush(stdout); safe_fgets(passengers[i].name, sizeof(passengers[i].name));
                printf("Passenger %d Age: ", i + 1); fflush(stdout); scanf("%d", &passengers[i].age); clear_input_buffer();
                printf("Passenger %d Gender (M/F/O): ", i + 1); fflush(stdout); scanf("%c", &passengers[i].gender); clear_input_buffer();
            }

            Ticket t;
            if (book_ticket(username, train_id, sched_id, (TrainClass)class_choice, p_count, passengers, &t)) {
                printf("\n[SUCCESS] Booking confirmed! PNR: %s | Status: %s\n",
                       t.pnr, t.status == STATUS_CONFIRMED ? "CONFIRMED" : "WAITLIST");
            }
        } else if (choice == 3) {
            char pnr[MAX_PNR_LEN];
            printf("Enter PNR: "); fflush(stdout); safe_fgets(pnr, sizeof(pnr));
            view_booking_by_pnr(pnr);
        } else if (choice == 4) {
            char pnr[MAX_PNR_LEN];
            printf("Enter PNR to cancel: "); fflush(stdout); safe_fgets(pnr, sizeof(pnr));
            if (cancel_ticket(pnr)) {
                printf("[SUCCESS] Ticket cancelled and refund processed.\n");
            } else {
                printf("[ERROR] Cancellation failed.\n");
            }
        } else if (choice == 5) {
            view_user_booking_history(username);
        }
    } while (choice != 6);
}