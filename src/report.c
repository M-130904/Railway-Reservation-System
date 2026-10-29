#include <stdio.h>
#include <string.h>
#include "report.h"

#define MAX_RESERVATIONS 1000
#define RESERVATIONS_FILE "data/reservations.txt"
#define LINE_LENGTH 512

/* Structure representing a reservation record in memory */
typedef struct {
    char pnr[15];
    int train_id;
    int sched_id;
    char username[50];
    int t_class;
    int p_count;
    char p_details[256];
    double fare;
    int status; // 1: Confirmed, 2: Waitlist, 3: Cancelled
    int seat_wl;
    char txn_id[20];
    char pay_status[20];
    char date[12];
} ReportReservation;

/* Helper function to load reservations into an in-memory array */
static int load_reservations_array(ReportReservation reservations[], int max_size) {
    int count = 0;
    FILE *f = fopen(RESERVATIONS_FILE, "r");

    if (f != NULL) {
        char line[LINE_LENGTH];

        while ((count < max_size) && (fgets(line, sizeof(line), f) != NULL)) {
            ReportReservation r;

            if (sscanf(line,
               "%14[^|]|%d|%d|%49[^|]|%d|%d|%255[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11s",
               r.pnr, &r.train_id, &r.sched_id, r.username,
               &r.t_class, &r.p_count, r.p_details, &r.fare,
               &r.status, &r.seat_wl, r.txn_id, r.pay_status, r.date) == 13) {
                reservations[count] = r;
                count++;
            }
        }
        (void)fclose(f);
    }

    return count;
}

void generate_system_reports(void) {
    ReportReservation reservations[MAX_RESERVATIONS];
    int total_bookings = load_reservations_array(reservations, MAX_RESERVATIONS);

    if (total_bookings == 0) {
        (void)printf("\n[REPORT] No reservations recorded yet.\n");
    } else {
        int confirmed_count = 0;
        int waitlist_count = 0;
        int cancelled_count = 0;
        double total_revenue = 0.0;
        double total_refunds = 0.0;

        // Process entirely using the in-memory array of structures
        for (int i = 0; i < total_bookings; i++) {
            if (reservations[i].status == 1) { // CONFIRMED
                confirmed_count++;
                total_revenue += reservations[i].fare;
            } else if (reservations[i].status == 2) { // WAITLIST
                waitlist_count++;
                total_revenue += reservations[i].fare; // Waitlisted tickets are also paid for
            } else if (reservations[i].status == 3) { // CANCELLED
                cancelled_count++;
                total_refunds += reservations[i].fare;
            } else {
                /* unknown status: not counted */
            }
        }

        (void)printf("\n==================================================\n");
        (void)printf("         RAILWAY SYSTEM MANAGEMENT REPORT          \n");
        (void)printf("==================================================\n");
        (void)printf(" Total Reservations Recorded : %d\n", total_bookings);
        (void)printf(" ------------------------------------------------\n");
        (void)printf(" [CONFIRMED TICKETS]         : %d\n", confirmed_count);
        (void)printf(" [WAITLISTED TICKETS]        : %d\n", waitlist_count);
        (void)printf(" [CANCELLED TICKETS]         : %d\n", cancelled_count);
        (void)printf(" ------------------------------------------------\n");
        (void)printf(" Gross Revenue Collected     : Rs. %.2f\n", total_revenue);
        (void)printf(" Total Refunds Processed     : Rs. %.2f\n", total_refunds);
        (void)printf(" Net System Revenue          : Rs. %.2f\n", total_revenue - total_refunds);
        (void)printf("==================================================\n");
    }
}