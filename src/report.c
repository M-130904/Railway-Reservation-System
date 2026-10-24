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
} ReservationRecord;

/* Helper function to load reservations into an in-memory array */
static int load_reservations_array(ReservationRecord reservations[], int max_size) {
    FILE *f = fopen(RESERVATIONS_FILE, "r");
    if (!f) return 0;
    
    char line[LINE_LENGTH];
    int count = 0;
    
    while (count < max_size && fgets(line, sizeof(line), f)) {
        ReservationRecord r;
        if (sscanf(line, "%[^|]|%d|%d|%[^|]|%d|%d|%[^|]|%lf|%d|%d|%[^|]|%[^|]|%s",
                   r.pnr, &r.train_id, &r.sched_id, r.username, &r.t_class, &r.p_count, 
                   r.p_details, &r.fare, &r.status, &r.seat_wl, r.txn_id, r.pay_status, r.date) >= 10) {
            reservations[count++] = r;
        }
    }
    fclose(f);
    return count;
}

void generate_system_reports(void) {
    ReservationRecord reservations[MAX_RESERVATIONS];
    int total_bookings = load_reservations_array(reservations, MAX_RESERVATIONS);

    if (total_bookings == 0) {
        printf("\n[REPORT] No reservations recorded yet.\n");
        return;
    }

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
        }
    }

    printf("\n==================================================\n");
    printf("         RAILWAY SYSTEM MANAGEMENT REPORT          \n");
    printf("==================================================\n");
    printf(" Total Reservations Recorded : %d\n", total_bookings);
    printf(" ------------------------------------------------\n");
    printf(" [CONFIRMED TICKETS]         : %d\n", confirmed_count);
    printf(" [WAITLISTED TICKETS]        : %d\n", waitlist_count);
    printf(" [CANCELLED TICKETS]         : %d\n", cancelled_count);
    printf(" ------------------------------------------------\n");
    printf(" Gross Revenue Collected     : Rs. %.2f\n", total_revenue);
    printf(" Total Refunds Processed     : Rs. %.2f\n", total_refunds);
    printf(" Net System Revenue          : Rs. %.2f\n", total_revenue - total_refunds);
    printf("==================================================\n");
}