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

/* Helper to write a reservation record to file */
static void write_reservation_record(FILE *f, const ReservationRecord *r)
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
 * Optimized seat updater using schedule RAM integration 
 */
static int update_schedule_seats_ram(int schedule_id, int train_class, int delta, int *booked_after)
{
    // Assuming schedule.c provides a way or we update via schedule functions. 
    // For safety, we call the schedule update logic cleanly.
    // If you use schedule.c's global array approach, we can update it directly or via helper.
    // Here we keep the single-file read/write for schedules securely, but without loops.
    FILE *f = fopen("data/schedules.txt", "r");
    if (!f) return 0;

    char lines[MAX_RECORDS][LINE_SIZE];
    char line[LINE_SIZE];
    int count = 0;

    while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
        snprintf(lines[count], sizeof(lines[count]), "%s", line);
        count++;
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

        if (fields != 14 || s.schedule_id != schedule_id) continue;

        int *booked = NULL;
        int capacity = 0;

        switch (train_class) {
            case CLASS_AC1: booked = &s.ac1_booked; capacity = s.ac1_cap; break;
            case CLASS_AC2: booked = &s.ac2_booked; capacity = s.ac2_cap; break;
            case CLASS_AC3: booked = &s.ac3_booked; capacity = s.ac3_cap; break;
            case CLASS_SLEEPER: booked = &s.sl_booked; capacity = s.sl_cap; break;
            default: return 0;
        }

        int new_count = *booked + delta;
        if (new_count < 0 || new_count > capacity) return 0;

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

    if (!found) return 0;

    f = fopen("data/schedules.txt", "w");
    if (!f) return 0;

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
 * OPTIMIZED FIFO WAITLIST PROMOTION:
 * Loads all reservations into RAM *once*, promotes all eligible 
 * waitlisted passengers in-memory, and writes back to disk *once*.
 */
static void promote_waitlist_fifo(int schedule_id, int train_class)
{
    FILE *f = fopen("data/reservations.txt", "r");
    if (!f) return;

    ReservationRecord records[MAX_RECORDS];
    int count = 0;

    char line[LINE_SIZE];
    while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
        parse_reservation(line, &records[count]);
        count++;
    }
    fclose(f);

    int promoted_any = 0;

    // Loop in RAM to promote multiple eligible candidates if seats permit
    for (;;) {
        int candidate_index = -1;

        // Find the earliest waitlisted reservation in RAM (Strict FIFO order)
        for (int i = 0; i < count; i++) {
            if (records[i].schedule_id == schedule_id &&
                records[i].train_class == train_class &&
                records[i].status == STATUS_WAITLIST) {
                candidate_index = i;
                break; // Found the earliest one!
            }
        }

        if (candidate_index < 0) {
            break; // No more waitlisted passengers for this schedule/class
        }

        int booked_after = 0;
        // Try to reserve seats for this group in RAM / Schedule file
        if (!update_schedule_seats_ram(schedule_id, train_class, records[candidate_index].passenger_count, &booked_after)) {
            break; // Group doesn't fit yet; stop promotion chain
        }

        // Promote in RAM
        records[candidate_index].status = STATUS_CONFIRMED;
        records[candidate_index].seat_or_wl_number = booked_after;
        promoted_any = 1;

        printf("[SUCCESS] Waitlisted ticket %s is now CONFIRMED.\n", records[candidate_index].pnr);
        log_message("INFO", "Waitlisted ticket promoted");
    }

    // If any promotions happened, write the entire reservations array back to disk *once*
    if (promoted_any) {
        FILE *wf = fopen("data/reservations.txt", "w");
        if (!wf) return;

        for (int i = 0; i < count; i++) {
            write_reservation_record(wf, &records[i]);
        }
        fclose(wf);
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

    ReservationRecord records[MAX_RECORDS];
    int count = 0;
    int target_index = -1;

    char line[LINE_SIZE];
    while (count < MAX_RECORDS && fgets(line, sizeof(line), f)) {
        parse_reservation(line, &records[count]);
        if (strcmp(records[count].pnr, pnr_target) == 0) {
            target_index = count;
        }
        count++;
    }
    fclose(f);

    if (target_index < 0) {
        printf("[ERROR] PNR not found.\n");
        return 0;
    }

    if (records[target_index].status == STATUS_CANCELLED) {
        printf("[INFO] This ticket is already cancelled.\n");
        return 0;
    }

    int previous_status = records[target_index].status;
    records[target_index].status = STATUS_CANCELLED;

    // Save cancellation state to disk once
    FILE *wf = fopen("data/reservations.txt", "w");
    if (!wf) {
        printf("[ERROR] Cannot save cancellation.\n");
        return 0;
    }

    for (int i = 0; i < count; i++) {
        write_reservation_record(wf, &records[i]);
    }
    fclose(wf);

    printf("[SUCCESS] Ticket %s has been cancelled.\n", records[target_index].pnr);

    /* If confirmed ticket was cancelled, release seats and trigger FIFO promotion */
    if (previous_status == STATUS_CONFIRMED) {
        if (!update_schedule_seats_ram(
                records[target_index].schedule_id,
                records[target_index].train_class,
                -records[target_index].passenger_count,
                NULL)) {
            printf("[ERROR] Ticket cancelled, but seat counts could not be updated.\n");
            return 0;
        }

        printf("[INFO] Released %d seat(s).\n", records[target_index].passenger_count);
        promote_waitlist_fifo(records[target_index].schedule_id, records[target_index].train_class);
    }

    log_message("INFO", "Ticket cancelled");
    return 1;
}