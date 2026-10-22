
#include "cancellation.h"
#include "types.h"
#include "logger.h"

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
    return sscanf(line,
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
        r->journey_date) == 13;
}

static void write_reservation(FILE *f, const ReservationRecord *r)
{
    fprintf(f,
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
        r->journey_date);
}

/*
 * Update booked seats for one schedule and class.
 * delta < 0 releases seats; delta > 0 reserves seats.
 * Returns the new booked-seat count through booked_after.
 */
static int update_schedule_seats(
    int schedule_id,
    int train_class,
    int delta,
    int *booked_after)
{
    FILE *f = fopen("data/schedules.txt", "r");
    if (!f) {
        return 0;
    }

    char lines[MAX_RECORDS][LINE_SIZE];
    char line[LINE_SIZE];
    int count = 0;

    while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
        snprintf(lines[count], sizeof(lines[count]), "%s", line);
        count++;
    }

    if (!feof(f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    int found = 0;
    int updated_count = 0;

    for (int i = 0; i < count; i++) {
        Schedule s = {0};

        int fields = sscanf(lines[i],
            "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
            &s.schedule_id, &s.train_id,
            s.journey_date, s.departure, s.arrival,
            &s.ac1_cap, &s.ac2_cap, &s.ac3_cap, &s.sl_cap,
            &s.ac1_booked, &s.ac2_booked,
            &s.ac3_booked, &s.sl_booked, &s.is_active);

        if (fields != 14 || s.schedule_id != schedule_id) {
            continue;
        }

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
                return 0;
        }

        int new_count = *booked + delta;

        if (new_count < 0 || new_count > capacity) {
            return 0;
        }

        *booked = new_count;
        updated_count = new_count;

        snprintf(lines[i], sizeof(lines[i]),
            "%d|%d|%s|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d|%d\n",
            s.schedule_id, s.train_id,
            s.journey_date, s.departure, s.arrival,
            s.ac1_cap, s.ac2_cap, s.ac3_cap, s.sl_cap,
            s.ac1_booked, s.ac2_booked,
            s.ac3_booked, s.sl_booked, s.is_active);

        found = 1;
        break;
    }

    if (!found) {
        return 0;
    }

    f = fopen("data/schedules.txt", "w");
    if (!f) {
        return 0;
    }

    for (int i = 0; i < count; i++) {
        fputs(lines[i], f);
    }

    int ok = !ferror(f);
    fclose(f);

    if (ok && booked_after != NULL) {
        *booked_after = updated_count;
    }

    return ok;
}

/*
 * Promote the earliest waitlisted reservation for the same
 * schedule and class, provided the whole group fits.
 */
static void promote_waitlist_fifo(int schedule_id, int train_class)
{
    for (;;) {
        FILE *f = fopen("data/reservations.txt", "r");
        if (!f) {
            return;
        }

        char lines[MAX_RECORDS][LINE_SIZE];
        char line[LINE_SIZE];
        int count = 0;
        int candidate_index = -1;
        ReservationRecord candidate = {0};

        while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
            snprintf(lines[count], sizeof(lines[count]), "%s", line);

            ReservationRecord r = {0};

            if (parse_reservation(line, &r) &&
                r.schedule_id == schedule_id &&
                r.train_class == train_class &&
                r.status == STATUS_WAITLIST &&
                candidate_index == -1) {
                candidate = r;
                candidate_index = count;
            }

            count++;
        }

        int read_ok = feof(f);
        fclose(f);

        if (!read_ok || candidate_index < 0) {
            return;
        }

        /*
         * The first waitlisted booking has priority.
         * Do not skip it if its entire group cannot fit.
         */
        int booked_after = 0;

        if (!update_schedule_seats(
                schedule_id, train_class,
                candidate.passenger_count, &booked_after)) {
            return;
        }

        candidate.status = STATUS_CONFIRMED;
        candidate.seat_or_wl_number = booked_after;

        FILE *wf = fopen("data/reservations.txt", "w");
        if (!wf) {
            /*
             * A file-write failure here needs manual recovery of the
             * schedule count. Do not continue promoting other tickets.
             */
            return;
        }

        for (int i = 0; i < count; i++) {
            if (i == candidate_index) {
                write_reservation(wf, &candidate);
            } else {
                fputs(lines[i], wf);
            }
        }

        int write_ok = !ferror(wf);
        fclose(wf);

        if (!write_ok) {
            return;
        }

        printf("[SUCCESS] Waitlisted ticket %s is now CONFIRMED.\n",
               candidate.pnr);

        log_message("INFO", "Waitlisted ticket promoted");

        /*
         * Check again in case more seats are available for the next
         * waiting group.
         */
    }
}

int cancel_ticket(const char *pnr_target)
{
    if (pnr_target == NULL || pnr_target[0] == '\0') {
        printf("[ERROR] Invalid PNR.\n");
        return 0;
    }

    FILE *f = fopen("data/reservations.txt", "r");
    if (!f) {
        printf("[ERROR] Cannot open reservations.txt.\n");
        return 0;
    }

    char lines[MAX_RECORDS][LINE_SIZE];
    char line[LINE_SIZE];
    int count = 0;
    int target_index = -1;
    ReservationRecord target = {0};

    while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
        snprintf(lines[count], sizeof(lines[count]), "%s", line);

        ReservationRecord r = {0};

        if (parse_reservation(line, &r) &&
            strcmp(r.pnr, pnr_target) == 0) {
            target_index = count;
            target = r;
        }

        count++;
    }

    int read_ok = feof(f);
    fclose(f);

    if (!read_ok) {
        printf("[ERROR] Too many reservation records or read failure.\n");
        return 0;
    }

    if (target_index < 0) {
        printf("[ERROR] PNR not found.\n");
        return 0;
    }

    if (target.status == STATUS_CANCELLED) {
        printf("[INFO] This ticket is already cancelled.\n");
        return 0;
    }

    if (target.status != STATUS_CONFIRMED &&
        target.status != STATUS_WAITLIST) {
        printf("[ERROR] Invalid ticket status.\n");
        return 0;
    }

    /* Preserve the original status before changing it. */
    int previous_status = target.status;

    target.status = STATUS_CANCELLED;

    snprintf(lines[target_index], sizeof(lines[target_index]),
        "%s|%d|%d|%s|%d|%d|%s|%.2f|%d|%d|%s|%s|%s\n",
        target.pnr,
        target.train_id,
        target.schedule_id,
        target.username,
        target.train_class,
        target.passenger_count,
        target.passengers,
        target.total_fare,
        target.status,
        target.seat_or_wl_number,
        target.transaction_id,
        target.payment_status,
        target.journey_date);

    f = fopen("data/reservations.txt", "w");
    if (!f) {
        printf("[ERROR] Cannot save cancellation.\n");
        return 0;
    }

    for (int i = 0; i < count; i++) {
        fputs(lines[i], f);
    }

    int write_ok = !ferror(f);
    fclose(f);

    if (!write_ok) {
        printf("[ERROR] Failed to save cancellation.\n");
        return 0;
    }

    printf("[SUCCESS] Ticket %s has been cancelled.\n", target.pnr);

    /* A waitlisted cancellation does not release physical seats. */
    if (previous_status == STATUS_CONFIRMED) {
        if (!update_schedule_seats(
                target.schedule_id,
                target.train_class,
                -target.passenger_count,
                NULL)) {
            printf("[ERROR] Ticket was cancelled, but seat counts could "
                   "not be updated. Please check schedules.txt.\n");
            return 0;
        }

        printf("[INFO] Released %d seat(s).\n", target.passenger_count);

        promote_waitlist_fifo(target.schedule_id, target.train_class);
    }

    log_message("INFO", "Ticket cancelled");
    return 1;
}
