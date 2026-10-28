#include "cancellation.h"
#include "schedule.h"
#include "types.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>

#define MAX_RECORDS 500
#define LINE_SIZE 512

typedef struct {
    char pnr[MAX_PNR_LEN];
    int train_id;
    int schedule_id;
    char username[MAX_NAME_LEN];
    int train_class;
    int passenger_count;
    char passengers[300];
    double total_fare;
    int status;
    int seat_or_wl_number;
    char transaction_id[20];
    char payment_status[20];
    char journey_date[MAX_DATE_LEN];
} ReservationRecord;

/* Parse one reservation line from reservations.txt. */
static int parse_reservation(const char *line, ReservationRecord *r)
{
    int fields;

    fields = sscanf(
        line,
        "%14[^|]|%d|%d|%49[^|]|%d|%d|%299[^|]|%lf|%d|%d|%19[^|]|%19[^|]|%11[^\r\n]",
        r->pnr,
        &r->train_id,
        &r->schedule_id,
        r->username,
        &r->train_class,
        &r->passenger_count,
        r->passengers,
        &r->total_fare,
        &r->status,
        &r->seat_or_wl_number,
        r->transaction_id,
        r->payment_status,
        r->journey_date
    );

    return (fields == 13) ? 1 : 0;
}

/* Write one reservation record to file. */
static int write_reservation_record(
    FILE *f,
    const ReservationRecord *r)
{
    int result;

    result = fprintf(
        f,
        "%s|%d|%d|%s|%d|%d|%s|%.2f|%d|%d|%s|%s|%s\n",
        r->pnr,
        r->train_id,
        r->schedule_id,
        r->username,
        r->train_class,
        r->passenger_count,
        r->passengers,
        r->total_fare,
        r->status,
        r->seat_or_wl_number,
        r->transaction_id,
        r->payment_status,
        r->journey_date
    );

    return (result >= 0) ? 1 : 0;
}

/* Update the saved seat count for one schedule and class. */
static int update_schedule_seats_ram(
    int schedule_id,
    int train_class,
    int delta,
    int *booked_after)
{
    FILE *f;
    char lines[MAX_RECORDS][LINE_SIZE];
    char line[LINE_SIZE];
    int result = 0;                                            /* 15.5 */
    int count = 0;
    int found = 0;
    int failed = 0;                                            /* 15.5: replaces early returns */
    int updated_count = 0;
    int read_ok = 1;

    f = fopen("data/schedules.txt", "r");

    if (f != NULL) {
        while ((count < MAX_RECORDS) &&
               (fgets(line, (int)sizeof(line), f) != NULL)) {
            int length;

            length = snprintf(
                lines[count],
                sizeof(lines[count]),
                "%s",
                line
            );

            if ((length < 0) ||
                ((size_t)length >= sizeof(lines[count]))) {
                read_ok = 0;
                break;
            }

            count++;
        }

        if (ferror(f) != 0) {
            read_ok = 0;
        }

        if (fclose(f) != 0) {
            read_ok = 0;
        }

        if (read_ok != 0) {
            for (int i = 0; (i < count) && (found == 0) && (failed == 0); i++) {
                Schedule s = {0};
                int fields;

                fields = sscanf(
                    lines[i],
                    "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                    &s.schedule_id,
                    &s.train_id,
                    s.journey_date,
                    s.departure,
                    s.arrival,
                    &s.ac1_cap,
                    &s.ac2_cap,
                    &s.ac3_cap,
                    &s.sl_cap,
                    &s.ac1_booked,
                    &s.ac2_booked,
                    &s.ac3_booked,
                    &s.sl_booked,
                    &s.is_active
                );

                /* 15.x: 'continue' removed, line is skipped by the if */
                if ((fields == 14) && (s.schedule_id == schedule_id)) {
                    int *booked = NULL;
                    int capacity = 0;

                    switch (train_class) {
                        case CLASS_AC1:
                            booked = &s.ac1_booked;
                            capacity = s.ac1_cap;
                            break;

                        case CLASS_AC2:
                            booked = &s.ac2_booked;
                            capacity = s.ac2_cap;
                            break;

                        case CLASS_AC3:
                            booked = &s.ac3_booked;
                            capacity = s.ac3_cap;
                            break;

                        case CLASS_SLEEPER:
                            booked = &s.sl_booked;
                            capacity = s.sl_cap;
                            break;

                        default:
                            failed = 1;                        /* was: return 0 */
                            break;
                    }

                    if (failed == 0) {
                        int new_count = (*booked) + delta;     /* 12.1 */

                        if ((new_count < 0) || (new_count > capacity)) {
                            failed = 1;                        /* was: return 0 */
                        } else {
                            int length;

                            *booked = new_count;
                            updated_count = new_count;

                            length = snprintf(
                                lines[i],
                                sizeof(lines[i]),
                                "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
                                s.schedule_id,
                                s.train_id,
                                s.journey_date,
                                s.departure,
                                s.arrival,
                                s.ac1_cap,
                                s.ac2_cap,
                                s.ac3_cap,
                                s.sl_cap,
                                s.ac1_booked,
                                s.ac2_booked,
                                s.ac3_booked,
                                s.sl_booked,
                                s.is_active
                            );

                            if ((length < 0) ||
                                ((size_t)length >= sizeof(lines[i]))) {
                                failed = 1;                    /* was: return 0 */
                            } else {
                                found = 1;
                            }
                        }
                    }
                }
            }

            if (found != 0) {
                f = fopen("data/schedules.txt", "w");

                if (f != NULL) {
                    int write_ok = 1;

                    for (int i = 0; i < count; i++) {
                        if (fputs(lines[i], f) == EOF) {
                            write_ok = 0;
                            break;
                        }
                    }

                    if (ferror(f) != 0) {
                        write_ok = 0;
                    }

                    if (fclose(f) != 0) {
                        write_ok = 0;
                    }

                    if ((write_ok != 0) && (booked_after != NULL)) {
                        *booked_after = updated_count;
                    }

                    result = write_ok;
                }
            }
        }
    }

    return result;                                             /* 15.5 */
}

/* Promote waitlisted tickets in FIFO file order. */
static void promote_waitlist_fifo(int schedule_id, int train_class)
{
    FILE *f;
    ReservationRecord records[MAX_RECORDS];
    int count = 0;
    int read_ok = 1;
    int promoted_any = 0;
    char line[LINE_SIZE];

    f = fopen("data/reservations.txt", "r");

    if (f != NULL) {                                           /* 15.5: no early return */
        while ((count < MAX_RECORDS) &&
               (fgets(line, (int)sizeof(line), f) != NULL)) {
            if (parse_reservation(line, &records[count]) == 0) {
                read_ok = 0;
                break;
            }

            count++;
        }

        if (ferror(f) != 0) {
            read_ok = 0;
        }

        if (fclose(f) != 0) {
            read_ok = 0;
        }

        if (read_ok != 0) {
            int continue_promotion = 1;

            while (continue_promotion != 0) {
                int candidate_index = -1;

                for (int i = 0; (i < count) && (candidate_index < 0); i++) {
                    if ((records[i].schedule_id == schedule_id) &&
                        (records[i].train_class == train_class) &&
                        (records[i].status == (int)STATUS_WAITLIST)) {   /* 10.4 */
                        candidate_index = i;
                    }
                }

                if (candidate_index < 0) {
                    continue_promotion = 0;
                } else {
                    int booked_after = 0;

                    if (update_schedule_seats_ram(
                            schedule_id,
                            train_class,
                            records[candidate_index].passenger_count,
                            &booked_after) == 0) {
                        continue_promotion = 0;
                    } else {
                        records[candidate_index].status = (int)STATUS_CONFIRMED; /* 10.4 */
                        records[candidate_index].seat_or_wl_number = booked_after;
                        promoted_any = 1;

                        (void)printf(                                    /* 17.7 */
                            "[SUCCESS] Waitlisted ticket %s is now CONFIRMED.\n",
                            records[candidate_index].pnr
                        );

                        log_message("INFO", "Waitlisted ticket promoted");
                    }
                }
            }

            if (promoted_any != 0) {
                FILE *wf = fopen("data/reservations.txt", "w");

                if (wf != NULL) {
                    int write_ok = 1;

                    for (int i = 0; i < count; i++) {
                        if (write_reservation_record(wf, &records[i]) == 0) {
                            write_ok = 0;
                            break;
                        }
                    }

                    if (ferror(wf) != 0) {
                        write_ok = 0;
                    }

                    if (fclose(wf) != 0) {
                        write_ok = 0;
                    }

                    if (write_ok == 0) {
                        (void)printf("[ERROR] Failed to save waitlist promotions.\n"); /* 17.7 */
                    }
                }
            }
        }
    }
}

int cancel_ticket(const char *pnr)
{
    FILE *f;
    ReservationRecord records[MAX_RECORDS];
    int count = 0;
    int target_index = -1;
    int read_ok = 1;
    int result = 0;                                            /* 15.5 */
    char line[LINE_SIZE];

    if ((pnr == NULL) || (pnr[0] == '\0')) {
        (void)printf("[ERROR] Invalid PNR.\n");                /* 17.7 */
    } else {
        f = fopen("data/reservations.txt", "r");

        if (f == NULL) {
            (void)printf("[ERROR] Cannot open reservations.txt.\n"); /* 17.7 */
        } else {
            while ((count < MAX_RECORDS) &&
                   (fgets(line, (int)sizeof(line), f) != NULL)) {
                if (parse_reservation(line, &records[count]) == 0) {
                    read_ok = 0;
                    break;
                }

                if (strcmp(records[count].pnr, pnr) == 0) {
                    target_index = count;
                }

                count++;
            }

            if (ferror(f) != 0) {
                read_ok = 0;
            }

            if (fclose(f) != 0) {
                read_ok = 0;
            }

            if (read_ok == 0) {
                (void)printf("[ERROR] Failed to read reservation records.\n"); /* 17.7 */
            } else if (target_index < 0) {
                (void)printf("[ERROR] PNR not found.\n");                      /* 17.7 */
            } else if (records[target_index].status == (int)STATUS_CANCELLED) { /* 10.4 */
                (void)printf("[INFO] This ticket is already cancelled.\n");    /* 17.7 */
            } else {
                int previous_status = records[target_index].status;
                FILE *wf;

                records[target_index].status = (int)STATUS_CANCELLED;           /* 10.4 */

                wf = fopen("data/reservations.txt", "w");

                if (wf == NULL) {
                    (void)printf("[ERROR] Cannot save cancellation.\n");        /* 17.7 */
                } else {
                    int write_ok = 1;

                    for (int i = 0; i < count; i++) {
                        if (write_reservation_record(wf, &records[i]) == 0) {
                            write_ok = 0;
                            break;
                        }
                    }

                    if (ferror(wf) != 0) {
                        write_ok = 0;
                    }

                    if (fclose(wf) != 0) {
                        write_ok = 0;
                    }

                    if (write_ok == 0) {
                        (void)printf("[ERROR] Failed to write cancellation records.\n"); /* 17.7 */
                    } else {
                        (void)printf(                                           /* 17.7 */
                            "[SUCCESS] Ticket %s has been cancelled.\n",
                            records[target_index].pnr
                        );

                        if (previous_status == (int)STATUS_CONFIRMED) {         /* 10.4 */
                            if (update_schedule_seats_ram(
                                    records[target_index].schedule_id,
                                    records[target_index].train_class,
                                    -records[target_index].passenger_count,
                                    NULL) == 0) {
                                (void)printf(                                   /* 17.7 */
                                    "[ERROR] Ticket cancelled, but seat counts could not be updated.\n"
                                );
                            } else {
                                (void)printf(                                   /* 17.7 */
                                    "[INFO] Released %d seat(s).\n",
                                    records[target_index].passenger_count
                                );

                                promote_waitlist_fifo(
                                    records[target_index].schedule_id,
                                    records[target_index].train_class
                                );

                                log_message("INFO", "Ticket cancelled");
                                result = 1;
                            }
                        } else {
                            log_message("INFO", "Ticket cancelled");
                            result = 1;
                        }
                    }
                }
            }
        }
    }

    return result;                                             /* 15.5 */
}