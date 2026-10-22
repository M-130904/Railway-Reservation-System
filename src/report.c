#include "report.h"

void generate_system_reports(void) {
    FILE *f = fopen("data/reservations.txt", "r");
    if (!f) {
        printf("\n[REPORT] No reservations recorded yet.\n");
        return;
    }

    char line[512];
    int total_bookings = 0;
    int confirmed_count = 0;
    int waitlist_count = 0;
    int cancelled_count = 0;
    double total_revenue = 0.0;
    double total_refunds = 0.0;

    while (fgets(line, sizeof(line), f)) {
        char pnr[15], username[50], p_details[256], txn_id[20], pay_status[20], date[12];
        int train_id, sched_id, t_class, p_count, status, seat_wl;
        double fare;

        // Parse pipe-delimited reservation line format:
        // PNR|TrainID|ScheduleID|Username|Class|PassengerCount|Details|Fare|Status|SeatWL|TxnID|PayStatus|Date
        if (sscanf(line, "%[^|]|%d|%d|%[^|]|%d|%d|%[^|]|%lf|%d|%d|%[^|]|%[^|]|%s",
                   pnr, &train_id, &sched_id, username, &t_class, &p_count, 
                   p_details, &fare, &status, &seat_wl, txn_id, pay_status, date) >= 10) {
            
            total_bookings++;
            if (status == 1) { // CONFIRMED
                confirmed_count++;
                total_revenue += fare;
            } else if (status == 2) { // WAITLIST
                waitlist_count++;
                total_revenue += fare; // Waitlisted tickets are also paid for
            } else if (status == 3) { // CANCELLED
                cancelled_count++;
                total_refunds += fare;
            }
        }
    }
    fclose(f);

    printf("\n==================================================\n");
    printf("        RAILWAY SYSTEM MANAGEMENT REPORT          \n");
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