#include "payment.h"
#include "utils.h"
#include "logger.h"

int process_payment(double amount, char *out_txn_id) {
    int mode;
    printf("\n========================================\n");
    printf("         SECURE PAYMENT GATEWAY         \n");
    printf("========================================\n");
    printf("Total Fare to Pay : Rs. %.2f\n", amount);
    printf("Select Payment Method:\n");
    printf("1. UPI (GooglePay / PhonePe / Paytm)\n");
    printf("2. Credit / Debit Card\n");
    printf("3. Net Banking\n");
    printf("Enter choice (1-3): "); 
    fflush(stdout); // <-- THIS FORCES THE PROMPT TO APPEAR ON SCREEN IMMEDIATELY!

    if (scanf("%d", &mode) != 1) {
        clear_input_buffer();
        printf("[ERROR] Invalid payment input.\n");
        return 0;
    }
    clear_input_buffer();

    if (mode == 1) {
        char upi_id[50];
        printf("Enter UPI ID (e.g., user@okhdfcbank): "); fflush(stdout);
        safe_fgets(upi_id, sizeof(upi_id));
        if (strchr(upi_id, '@') == NULL) {
            printf("[FAILED] Invalid UPI ID format.\n");
            return 0;
        }
    } else {
        printf("Processing secure bank authorization...\n");
    }

    int rand_num = 10000000 + rand() % 90000000;
    sprintf(out_txn_id, "TXN%d", rand_num);
    printf("[SUCCESS] Payment complete! Ref: %s\n", out_txn_id);
    log_message("INFO", "Payment successful");
    return 1;
}