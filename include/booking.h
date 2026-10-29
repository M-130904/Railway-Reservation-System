#ifndef BOOKING_H
#define BOOKING_H
#include "types.h"
int book_ticket(const char *username, int train_id, int schedule_id, TrainClass t_class, int p_count, PassengerDetail passengers[], Ticket *out_ticket);
void view_booking_by_pnr(const char *pnr_target);
void view_user_booking_history(const char *username_target);
#endif