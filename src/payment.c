#include "payment.h"
#include "utils.h"
#include "logger.h"

int process_payment(double amount, char *out_txn_id) {
    int mode = 0;
    int result = 0;
    int proceed = 1;

    (void)printf("\n========================================\n");
    (void)printf("         SECURE PAYMENT GATEWAY         \n");
    (void)printf("========================================\n");
    (void)printf("Total Fare to Pay : Rs. %.2f\n", amount);
    (void)printf("Select Payment Method:\n");
    (void)printf("1. UPI (GooglePay / PhonePe / Paytm)\n");
    (void)printf("2. Credit / Debit Card\n");
    (void)printf("3. Net Banking\n");
    (void)printf("Enter choice (1-3): ");
    (void)fflush(stdout); // <-- THIS FORCES THE PROMPT TO APPEAR ON SCREEN IMMEDIATELY!

    if (scanf("%d", &mode) != 1) {
        clear_input_buffer();
        (void)printf("[ERROR] Invalid payment input.\n");
        proceed = 0;
    } else {
        clear_input_buffer();
    }

    if (proceed != 0) {
        if (mode == 1) {
            char upi_id[50];

            (void)printf("Enter UPI ID (e.g., user@okhdfcbank): ");
            (void)fflush(stdout);
            safe_fgets(upi_id, sizeof(upi_id));
            if (strchr(upi_id, '@') == NULL) {
                (void)printf("[FAILED] Invalid UPI ID format.\n");
                proceed = 0;
            }
        } else {
            (void)printf("Processing secure bank authorization...\n");
        }
    }

    if (proceed != 0) {
        int rand_num = 10000000 + (rand() % 90000000);

        (void)sprintf(out_txn_id, "TXN%d", rand_num);
        (void)printf("[SUCCESS] Payment complete! Ref: %s\n", out_txn_id);
        log_message("INFO", "Payment successful");
        result = 1;
    }

    return result;
}