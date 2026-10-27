#include "booking.h"
#include "train.h"
#include "schedule.h"
#include "payment.h"
#include "utils.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

static pthread_mutex_t booking_mutex = PTHREAD_MUTEX_INITIALIZER;

int book_ticket(const char *username, int train_id, int schedule_id,
                TrainClass t_class, int p_count,
                PassengerDetail passengers[], Ticket *out_ticket) {

                if (pthread_mutex_lock(&booking_mutex) != 0) {
                        return 0;
                }
    const Train *t = find_train_by_id(train_id);
    if (!t) {
        printf("[ERROR] Train ID %d not found or inactive!\n", train_id);
        pthread_mutex_unlock(&booking_mutex);
        return 0;
    }

    Schedule *s = find_schedule_by_id(schedule_id);
    if (!s) {
        printf("[ERROR] Schedule ID %d not found or inactive!\n", schedule_id);
        pthread_mutex_unlock(&booking_mutex);
        return 0;
    }

    double multiplier = (t_class == CLASS_AC1) ? 2.5 :
                        (t_class == CLASS_AC2 ? 1.8 :
                        (t_class == CLASS_AC3 ? 1.4 : 1.0));
    double total_fare = t->base_fare * multiplier * p_count;

    char txn_id[20];
    if (!process_payment(total_fare, txn_id)) {
        printf("[ERROR] Payment process aborted or failed.\n");
        pthread_mutex_unlock(&booking_mutex);
        return 0;
    }

    generate_pnr(out_ticket->pnr);
    out_ticket->train_id = train_id;
    out_ticket->schedule_id = schedule_id;
    strcpy(out_ticket->username, username);
    out_ticket->train_class = t_class;
    out_ticket->passenger_count = p_count;

    for (int i = 0; i < p_count; i++) {
        out_ticket->passengers[i] = passengers[i];
    }

    out_ticket->total_fare = total_fare;
    strcpy(out_ticket->journey_date, s->journey_date);
    strcpy(out_ticket->transaction_id, txn_id);
    strcpy(out_ticket->payment_status, "SUCCESS");

    const int *cap = NULL;
    int *booked = NULL;

    if (t_class == CLASS_AC1) {
        cap = &s->ac1_cap;
        booked = &s->ac1_booked;
    } else if (t_class == CLASS_AC2) {
        cap = &s->ac2_cap;
        booked = &s->ac2_booked;
    } else if (t_class == CLASS_AC3) {
        cap = &s->ac3_cap;
        booked = &s->ac3_booked;
    } else {
        cap = &s->sl_cap;
        booked = &s->sl_booked;
    }

    /* Seat allocation and waitlist */
    if ((*booked) + p_count <= (*cap)) {
        (*booked) += p_count;
        out_ticket->status = STATUS_CONFIRMED;
        out_ticket->seat_or_wl_number = *booked;
    } else {
        out_ticket->status = STATUS_WAITLIST;
        /* Waitlisted passengers do not consume confirmed capacity. */
        out_ticket->seat_or_wl_number = (*booked) + p_count - (*cap);
    }

    /* Persist updated seat counts to data/schedules.txt */
    FILE *fs_read = fopen("data/schedules.txt", "r");

    if (fs_read) {
        char lines[100][512];
        int s_count = 0;

        while (s_count < 100 &&
               fgets(lines[s_count], sizeof(lines[s_count]), fs_read)) {
            s_count++;
        }

        fclose(fs_read);

        for (int i = 0; i < s_count; i++) {
            int sid, tid;

            if (sscanf(lines[i], "%d|%d", &sid, &tid) == 2 &&
                sid == schedule_id) {
                snprintf(lines[i], sizeof(lines[i]),
                         "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                         s->schedule_id, s->train_id,
                         s->journey_date, s->departure, s->arrival,
                         s->ac1_cap, s->ac2_cap, s->ac3_cap, s->sl_cap,
                         s->ac1_booked, s->ac2_booked,
                         s->ac3_booked, s->sl_booked, s->is_active);
                break;
            }
        }

        FILE *fs_write = fopen("data/schedules.txt", "w");

        if (fs_write) {
            for (int i = 0; i < s_count; i++) {
                fputs(lines[i], fs_write);
            }
            fclose(fs_write);
        }
    }

    /* Append reservation to data/reservations.txt */
    FILE *f = fopen("data/reservations.txt", "a");

    if (!f) {
        printf("[ERROR] Could not open reservations.txt for writing!\n");
        pthread_mutex_unlock(&booking_mutex);
        return 0;
    }

    fprintf(f, "%s|%d|%d|%s|%d|%d|",
            out_ticket->pnr, out_ticket->train_id,
            out_ticket->schedule_id, out_ticket->username,
            out_ticket->train_class, out_ticket->passenger_count);

    for (int i = 0; i < p_count; i++) {
        fprintf(f, "%s:%d:%c%s",
                out_ticket->passengers[i].name,
                out_ticket->passengers[i].age,
                out_ticket->passengers[i].gender,
                i < p_count - 1 ? ";" : "");
    }

    fprintf(f, "|%.2f|%d|%d|%s|%s|%s\n",
            out_ticket->total_fare, out_ticket->status,
            out_ticket->seat_or_wl_number, out_ticket->transaction_id,
            out_ticket->payment_status, out_ticket->journey_date);

    fclose(f);

    log_message("INFO", "Ticket booked successfully");
    pthread_mutex_unlock(&booking_mutex);
    return 1;
}

void view_booking_by_pnr(const char *pnr_target) {
    FILE *f = fopen("data/reservations.txt", "r");

    if (!f) {
        printf("No bookings found.\n");
        return;
    }

    /* Dynamic allocation for Valgrind demonstration */
    char *line = malloc(512);

    if (line == NULL) {
        fclose(f);
        printf("Memory allocation failed.\n");
        return;
    }

    int found = 0;

    while (fgets(line, 512, f)) {
        char pnr[15], username[50], p_details[256];
        char txn_id[20], pay_status[20], date[12];
        int train_id, sched_id, t_class, p_count, status, seat_wl;
        double fare;

        if (sscanf(line,
                   "%14[^|]|%d|%d|%49[^|]|%d|%d|%255[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11[^\r\n]",
                   pnr, &train_id, &sched_id, username, &t_class, &p_count,
                   p_details, &fare, &status, &seat_wl,
                   txn_id, pay_status, date) == 13) {

            if (strcmp(pnr, pnr_target) == 0) {
                found = 1;

                const char *status_str =
                    (status == 1) ? "CONFIRMED" :
                    (status == 2 ? "WAITLIST" : "CANCELLED");

                const char *class_str =
                    (t_class == 1) ? "AC 1-Tier" :
                    (t_class == 2 ? "AC 2-Tier" :
                    (t_class == 3 ? "AC 3-Tier" : "Sleeper"));

                printf("\n========================================\n");
                printf("             TICKET DETAILS             \n");
                printf("========================================\n");
                printf(" PNR Number   : %s\n", pnr);
                printf(" Train ID     : %d\n", train_id);
                printf(" Schedule ID  : %d\n", sched_id);
                printf(" Journey Date : %s\n", date);
                printf(" Username     : %s\n", username);
                printf(" Class        : %s\n", class_str);
                printf(" Status       : %s\n", status_str);

                if (status == 1) {
                    printf(" Seat Number  : %d\n", seat_wl);
                }

                if (status == 2) {
                    printf(" Waitlist No  : WL %d\n", seat_wl);
                }

                printf(" Total Fare   : Rs. %.2f (%s)\n", fare, pay_status);
                printf(" Passengers   : %s\n", p_details);
                printf("========================================\n");
                break;
            }
        }
    }

    fclose(f);

    if (!found) {
        printf("PNR not found.\n");
    }

    //Intentionally omitted for the before-fix leak demonstration:
    free(line);
    
}

void view_user_booking_history(const char *username_target) {
    FILE *f = fopen("data/reservations.txt", "r");

    if (!f) {
        printf("No history found.\n");
        return;
    }

    /* Dynamic allocation for Valgrind demonstration */
    char *line = malloc(512);

    if (line == NULL) {
        fclose(f);
        printf("Memory allocation failed.\n");
        return;
    }

    printf("\n=== Booking History for %s ===\n", username_target);

    int count = 0;

    while (fgets(line, 512, f)) {
        char pnr[15], username[50], p_details[256];
        char txn_id[20], pay_status[20], date[12];
        int train_id, sched_id, t_class, p_count, status, seat_wl;
        double fare;

        if (sscanf(line,
                   "%14[^|]|%d|%d|%49[^|]|%d|%d|%255[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11[^\r\n]",
                   pnr, &train_id, &sched_id, username, &t_class, &p_count,
                   p_details, &fare, &status, &seat_wl,
                   txn_id, pay_status, date) == 13) {

            if (strcmp(username, username_target) == 0) {
                count++;

                const char *status_str =
                    (status == 1) ? "CNF" :
                    (status == 2 ? "WL" : "CANCELLED");

                printf("[%d] PNR: %s | Train: %d | Date: %s | Status: %s | Fare: Rs. %.2f\n",
                       count, pnr, train_id, date, status_str, fare);
            }
        }
    }

    if (count == 0) {
        printf("No bookings found for this user.\n");
    }

    fclose(f);

    //Intentionally omitted for the before-fix leak demonstration:
       free(line);
    
}
