#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NAME_LEN 50
#define MAX_CODE_LEN 20
#define MAX_DATE_LEN 12
#define MAX_TIME_LEN 6
#define MAX_PNR_LEN 15
#define MAX_PASSENGERS 5

typedef enum {
    ROLE_ADMIN = 1,
    ROLE_PASSENGER = 2
} UserRole;

typedef enum {
    CLASS_AC1 = 1,
    CLASS_AC2 = 2,
    CLASS_AC3 = 3,
    CLASS_SLEEPER = 4
} TrainClass;

typedef enum {
    STATUS_CONFIRMED = 1,
    STATUS_WAITLIST = 2,
    STATUS_CANCELLED = 3
} TicketStatus;

// typedef struct {
//     char username[MAX_NAME_LEN];
//     char password[MAX_NAME_LEN];
//     UserRole role;
// } User;

typedef struct {
    int train_id;
    char train_name[MAX_NAME_LEN];
    char source[MAX_CODE_LEN];
    char destination[MAX_CODE_LEN];
    int capacity;
    double base_fare;
    int is_active;
} Train;

typedef struct {
    int schedule_id;
    int train_id;
    char journey_date[MAX_DATE_LEN];
    char departure[MAX_TIME_LEN];
    char arrival[MAX_TIME_LEN];
    int ac1_cap;
    int ac2_cap;
    int ac3_cap;
    int sl_cap;
    int ac1_booked;
    int ac2_booked;
    int ac3_booked;
    int sl_booked;
    int is_active;
} Schedule;

typedef struct {
    char name[MAX_NAME_LEN];
    int age;
    char gender;
} PassengerDetail;

typedef struct {
    char pnr[MAX_PNR_LEN];
    int train_id;
    int schedule_id;
    char username[MAX_NAME_LEN];
    TrainClass train_class;
    int passenger_count;
    PassengerDetail passengers[MAX_PASSENGERS];
    double total_fare;
    TicketStatus status;
    int seat_or_wl_number;
    char transaction_id[20];
    char payment_status[20];
    char journey_date[MAX_DATE_LEN];
} Ticket;

#endif
