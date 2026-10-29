
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

extern "C" {
#include "payment.h"
#include "utils.h"
#include "logger.h"
void clear_input_buffer(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

void safe_fgets(char *buffer, int size) {
    if (buffer == nullptr || size <= 0) {
        return;
    }

    if (fgets(buffer, size, stdin) == nullptr) {
        buffer[0] = '\0';
        return;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
}
}

class PaymentTest : public ::testing::Test {
protected:
    int savedStdin = -1;
    int savedStdout = -1;
    FILE *inputFile = nullptr;
    FILE *outputFile = nullptr;

    void SetUp() override {
        fflush(stdout);

        savedStdin = dup(STDIN_FILENO);
        savedStdout = dup(STDOUT_FILENO);

        inputFile = tmpfile();
        outputFile = tmpfile();

        ASSERT_NE(savedStdin, -1);
        ASSERT_NE(savedStdout, -1);
        ASSERT_NE(inputFile, nullptr);
        ASSERT_NE(outputFile, nullptr);
    }

    void TearDown() override {
        fflush(stdout);

        if (savedStdin >= 0) {
            dup2(savedStdin, STDIN_FILENO);
            close(savedStdin);
        }

        if (savedStdout >= 0) {
            dup2(savedStdout, STDOUT_FILENO);
            close(savedStdout);
        }

        clearerr(stdin);
        clearerr(stdout);

        if (inputFile != nullptr) {
            fclose(inputFile);
        }

        if (outputFile != nullptr) {
            fclose(outputFile);
        }
    }

    void RunPayment(const std::string &input) {
        ASSERT_NE(inputFile, nullptr);
        ASSERT_NE(outputFile, nullptr);

        ASSERT_EQ(
            fwrite(input.data(), 1, input.size(), inputFile),
            input.size()
        );
        ASSERT_EQ(fflush(inputFile), 0);

        rewind(inputFile);
        rewind(outputFile);

        clearerr(stdin);
        clearerr(stdout);

        ASSERT_NE(dup2(fileno(inputFile), STDIN_FILENO), -1);
        ASSERT_NE(dup2(fileno(outputFile), STDOUT_FILENO), -1);

        clearerr(stdin);
        clearerr(stdout);
        ASSERT_EQ(setvbuf(stdin, nullptr, _IONBF, 0), 0);

        // Provide a deterministic random sequence for repeatable tests.
        srand(12345);
    }

    std::string Output() {
        fflush(stdout);
        rewind(outputFile);

        std::string result;
        char buffer[256];
        size_t count;

        while ((count = fread(buffer, 1, sizeof(buffer), outputFile)) > 0) {
            result.append(buffer, count);
        }

        return result;
    }
};

// Positive: UPI payment succeeds with a valid UPI ID.
TEST_F(PaymentTest, ValidUPIPayment) {
    char txnId[32] = {};

    RunPayment("1\nuser@okhdfcbank\n");

    EXPECT_EQ(process_payment(450.00, txnId), 1);
    EXPECT_TRUE(strncmp(txnId, "TXN", 3) == 0);
    EXPECT_NE(Output().find("[SUCCESS] Payment complete!"), std::string::npos);
}

// Positive: card payment succeeds.
TEST_F(PaymentTest, CreditCardPayment) {
    char txnId[32] = {};

    RunPayment("2\n");

    EXPECT_EQ(process_payment(1000.00, txnId), 1);
    EXPECT_TRUE(strncmp(txnId, "TXN", 3) == 0);
    EXPECT_NE(Output().find("[SUCCESS] Payment complete!"), std::string::npos);
}

// Positive: net banking payment succeeds.
TEST_F(PaymentTest, NetBankingPayment) {
    char txnId[32] = {};

    RunPayment("3\n");

    EXPECT_EQ(process_payment(250.00, txnId), 1);
    EXPECT_TRUE(strncmp(txnId, "TXN", 3) == 0);
}

// Negative: nonnumeric payment method.
TEST_F(PaymentTest, NonnumericPaymentChoice) {
    char txnId[32] = {};

    RunPayment("abc\n");

    EXPECT_EQ(process_payment(450.00, txnId), 0);
    EXPECT_NE(Output().find("Invalid payment input"), std::string::npos);
}

// Negative: UPI ID without an at-sign.
TEST_F(PaymentTest, InvalidUPIFormat) {
    char txnId[32] = {};

    RunPayment("1\ninvalid-upi-id\n");

    EXPECT_EQ(process_payment(450.00, txnId), 0);
    EXPECT_NE(Output().find("Invalid UPI ID format"), std::string::npos);
}

// Boundary: smallest positive amount.
TEST_F(PaymentTest, SmallPositiveAmount) {
    char txnId[32] = {};

    RunPayment("2\n");

    EXPECT_EQ(process_payment(0.01, txnId), 1);
    EXPECT_TRUE(strncmp(txnId, "TXN", 3) == 0);
}

// Boundary: zero amount currently accepted by the implementation.
TEST_F(PaymentTest, ZeroAmountCurrentlyAccepted) {
    char txnId[32] = {};

    RunPayment("2\n");

    EXPECT_EQ(process_payment(0.00, txnId), 1);
    EXPECT_TRUE(strncmp(txnId, "TXN", 3) == 0);
}

extern "C" void generate_pnr(char *pnr_buffer) {
    if (pnr_buffer != nullptr) {
        strcpy(pnr_buffer, "PNR0001");
    }
}
