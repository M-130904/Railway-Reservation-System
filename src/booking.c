#include "booking.h"
#include "train.h"
#include "schedule.h"
#include "payment.h"
#include "utils.h"
#include "logger.h"

#include <stdio.h>      /* MISRA 21.6 deviation: file I/O + console */
#include <string.h>
#include <pthread.h>
/* #include <stdlib.h> removed: no malloc/free left after fixes */

int book_ticket(const char *username, int train_id, int schedule_id,
                TrainClass t_class, int p_count,
                PassengerDetail passengers[], Ticket *out_ticket)
{
    static pthread_mutex_t booking_mutex = PTHREAD_MUTEX_INITIALIZER;
    int result = 0;

    if (pthread_mutex_lock(&booking_mutex) == 0) {
        const Train *t = find_train_by_id(train_id);

        if (t == NULL) {
            (void)printf("[ERROR] Train ID %d not found or inactive!\n",
                         train_id);
        } else {
            Schedule *s = find_schedule_by_id(schedule_id);

            if (s == NULL) {
                (void)printf("[ERROR] Schedule ID %d not found or inactive!\n",
                             schedule_id);
            } else {
                double multiplier = 1.0;
                double total_fare;
                char txn_id[20];

                if (t_class == CLASS_AC1) {
                    multiplier = 2.5;
                } else if (t_class == CLASS_AC2) {
                    multiplier = 1.8;
                } else if (t_class == CLASS_AC3) {
                    multiplier = 1.4;
                } else {
                    /* 15.7: no action, default multiplier */
                }

                total_fare = t->base_fare * multiplier * (double)p_count;

                if (process_payment(total_fare, txn_id) == 0) {
                    (void)printf("[ERROR] Payment process aborted or failed.\n");
                } else {
                    const int *cap = NULL;
                    int *booked = NULL;
                    FILE *fs_read;
                    FILE *f;

                    generate_pnr(out_ticket->pnr);
                    out_ticket->train_id = train_id;
                    out_ticket->schedule_id = schedule_id;
                    (void)strcpy(out_ticket->username, username);
                    out_ticket->train_class = t_class;
                    out_ticket->passenger_count = p_count;

                    for (int i = 0; i < p_count; i++) {
                        out_ticket->passengers[i] = passengers[i];
                    }

                    out_ticket->total_fare = total_fare;
                    (void)strcpy(out_ticket->journey_date, s->journey_date);
                    (void)strcpy(out_ticket->transaction_id, txn_id);
                    (void)strcpy(out_ticket->payment_status, "SUCCESS");

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

                    if ((((*booked) + p_count)) <= (*cap)) {
                        (*booked) += p_count;
                        out_ticket->status = STATUS_CONFIRMED;
                        out_ticket->seat_or_wl_number = *booked;
                    } else {
                        out_ticket->status = STATUS_WAITLIST;
                        out_ticket->seat_or_wl_number =
                            ((*booked) + p_count) - (*cap);
                    }

                    fs_read = fopen("data/schedules.txt", "r");

                    if (fs_read != NULL) {
                        char lines[100][512];
                        int s_count = 0;
                        FILE *fs_write;

                        while ((s_count < 100) &&
                               (fgets(lines[s_count], (int)sizeof(lines[s_count]),
                                      fs_read) != NULL)) {
                            s_count++;
                        }

                        (void)fclose(fs_read);

                        for (int i = 0; i < s_count; i++) {
                            int sid;
                            int tid;

                            if ((sscanf(lines[i], "%d|%d", &sid, &tid) == 2) &&
                                (sid == schedule_id)) {
                                (void)snprintf(lines[i], sizeof(lines[i]),
                                               "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                                               s->schedule_id, s->train_id,
                                               s->journey_date, s->departure, s->arrival,
                                               s->ac1_cap, s->ac2_cap, s->ac3_cap, s->sl_cap,
                                               s->ac1_booked, s->ac2_booked,
                                               s->ac3_booked, s->sl_booked, s->is_active);
                                break;
                            }
                        }

                        fs_write = fopen("data/schedules.txt", "w");

                        if (fs_write != NULL) {
                            for (int i = 0; i < s_count; i++) {
                                (void)fputs(lines[i], fs_write);
                            }
                            (void)fclose(fs_write);
                        }
                    }

                    f = fopen("data/reservations.txt", "a");

                    if (f == NULL) {
                        (void)printf("[ERROR] Could not open reservations.txt for writing!\n");
                    } else {
                        (void)fprintf(f, "%s|%d|%d|%s|%d|%d|",
                                      out_ticket->pnr, out_ticket->train_id,
                                      out_ticket->schedule_id, out_ticket->username,
                                      (int)out_ticket->train_class,
                                      out_ticket->passenger_count);

                        for (int i = 0; i < p_count; i++) {
                            (void)fprintf(f, "%s:%d:%c%s",
                                          out_ticket->passengers[i].name,
                                          out_ticket->passengers[i].age,
                                          out_ticket->passengers[i].gender,
                                          (i < (p_count - 1)) ? ";" : "");
                        }

                        (void)fprintf(f, "|%.2f|%d|%d|%s|%s|%s\n",
                                      out_ticket->total_fare,
                                      (int)out_ticket->status,
                                      out_ticket->seat_or_wl_number,
                                      out_ticket->transaction_id,
                                      out_ticket->payment_status,
                                      out_ticket->journey_date);

                        (void)fclose(f);

                        log_message("INFO", "Ticket booked successfully");
                        result = 1;
                    }
                }
            }
        }

        (void)pthread_mutex_unlock(&booking_mutex);
    }

    return result;
}

void view_booking_by_pnr(const char *pnr_target)
{
    FILE *f = fopen("data/reservations.txt", "r");

    if (f == NULL) {
        (void)printf("No bookings found.\n");
    } else {
        /* FIX 11.5 + 21.3: stack buffer replaces malloc/free */
        char line[512];
        int found = 0;

        while (fgets(line, (int)sizeof(line), f) != NULL) {
            char pnr[15];
            char username[50];
            char p_details[256];
            char txn_id[20];
            char pay_status[20];
            char date[12];
            int train_id;
            int sched_id;
            int t_class;
            int p_count;
            int status;
            int seat_wl;
            double fare;

            if (sscanf(line,
                       "%14[^|]|%d|%d|%49[^|]|%d|%d|%255[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11[^\r\n]",
                       pnr, &train_id, &sched_id, username, &t_class, &p_count,
                       p_details, &fare, &status, &seat_wl,
                       txn_id, pay_status, date) == 13) {

                if (strcmp(pnr, pnr_target) == 0) {
                    const char *status_str = "CANCELLED";
                    const char *class_str = "Sleeper";

                    found = 1;

                    if (status == 1) {
                        status_str = "CONFIRMED";
                    } else if (status == 2) {
                        status_str = "WAITLIST";
                    } else {
                        /* 15.7: no action, stays CANCELLED */
                    }

                    if (t_class == 1) {
                        class_str = "AC 1-Tier";
                    } else if (t_class == 2) {
                        class_str = "AC 2-Tier";
                    } else if (t_class == 3) {
                        class_str = "AC 3-Tier";
                    } else {
                        /* 15.7: no action, stays Sleeper */
                    }

                    (void)printf("\n========================================\n");
                    (void)printf("             TICKET DETAILS             \n");
                    (void)printf("========================================\n");
                    (void)printf(" PNR Number   : %s\n", pnr);
                    (void)printf(" Train ID     : %d\n", train_id);
                    (void)printf(" Schedule ID  : %d\n", sched_id);
                    (void)printf(" Journey Date : %s\n", date);
                    (void)printf(" Username     : %s\n", username);
                    (void)printf(" Class        : %s\n", class_str);
                    (void)printf(" Status       : %s\n", status_str);

                    if (status == 1) {
                        (void)printf(" Seat Number  : %d\n", seat_wl);
                    }

                    if (status == 2) {
                        (void)printf(" Waitlist No  : WL %d\n", seat_wl);
                    }

                    (void)printf(" Total Fare   : Rs. %.2f (%s)\n", fare, pay_status);
                    (void)printf(" Passengers   : %s\n", p_details);
                    (void)printf("========================================\n");
                    break;
                }
            }
        }

        (void)fclose(f);

        if (found == 0) {
            (void)printf("PNR not found.\n");
        }
    }
}

void view_user_booking_history(const char *username_target)
{
    FILE *f = fopen("data/reservations.txt", "r");

    if (f == NULL) {
        (void)printf("No history found.\n");
    } else {
        /* FIX 11.5 + 21.3: stack buffer replaces malloc/free */
        char line[512];
        int count = 0;

        (void)printf("\n=== Booking History for %s ===\n", username_target);

        while (fgets(line, (int)sizeof(line), f) != NULL) {
            char pnr[15];
            char username[50];
            char p_details[256];
            char txn_id[20];
            char pay_status[20];
            char date[12];
            int train_id;
            int sched_id;
            int t_class;
            int p_count;
            int status;
            int seat_wl;
            double fare;

            if (sscanf(line,
                       "%14[^|]|%d|%d|%49[^|]|%d|%d|%255[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11[^\r\n]",
                       pnr, &train_id, &sched_id, username, &t_class, &p_count,
                       p_details, &fare, &status, &seat_wl,
                       txn_id, pay_status, date) == 13) {

                if (strcmp(username, username_target) == 0) {
                    const char *status_str = "CANCELLED";

                    count++;

                    if (status == 1) {
                        status_str = "CNF";
                    } else if (status == 2) {
                        status_str = "WL";
                    } else {
                        /* 15.7: no action, stays CANCELLED */
                    }

                    (void)printf("[%d] PNR: %s | Train: %d | Date: %s | Status: %s | Fare: Rs. %.2f\n",
                                 count, pnr, train_id, date, status_str, fare);
                }
            }
        }

        if (count == 0) {
            (void)printf("No bookings found for this user.\n");
        }

        (void)fclose(f);
    }
}