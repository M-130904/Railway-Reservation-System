#include "waitlist.h"
#include "logger.h"

int process_waitlist_promotion(int schedule_id, TrainClass t_class) {
    (void)schedule_id;
    (void)t_class;
    log_message("INFO", "Waitlist promotion evaluated");
    return 1;
}