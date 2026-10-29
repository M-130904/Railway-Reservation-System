
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

extern "C" {
#include "passenger.h"
#include "search.h"
#include "booking.h"
#include "cancellation.h"
#include "utils.h"
}

namespace {
int searchCalls = 0;
int bookingCalls = 0;
int viewPNRCalls = 0;
int cancelCalls = 0;
int historyCalls = 0;

std::string lastUsername;
std::string lastPNR;
std::string lastSource;
std::string lastDestination;
std::string lastDate;

int cancelResult = 1;
int bookingResult = 1;

class PassengerMenuTest : public ::testing::Test {
protected:
    int savedStdin = -1;
    int savedStdout = -1;
    FILE *inputFile = nullptr;
    FILE *outputFile = nullptr;

    void SetUp() override {
        searchCalls = 0;
        bookingCalls = 0;
        viewPNRCalls = 0;
        cancelCalls = 0;
        historyCalls = 0;

        lastUsername.clear();
        lastPNR.clear();
        lastSource.clear();
        lastDestination.clear();
        lastDate.clear();

        cancelResult = 1;
        bookingResult = 1;

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
            inputFile = nullptr;
        }

        if (outputFile != nullptr) {
            fclose(outputFile);
            outputFile = nullptr;
        }
    }

    void RunMenu(const std::string &input) {
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

        // Redirect the underlying file descriptors.
        ASSERT_NE(dup2(fileno(inputFile), STDIN_FILENO), -1);
        ASSERT_NE(dup2(fileno(outputFile), STDOUT_FILENO), -1);

        // Discard any buffered data left from a previous test.
        clearerr(stdin);
        clearerr(stdout);

        // Make stdin unbuffered so scanf/getchar/fgets use the current
        // redirected descriptor without retaining input across tests.
        ASSERT_EQ(setvbuf(stdin, nullptr, _IONBF, 0), 0);

        passenger_menu("test_user");

        fflush(stdout);
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
}

// Stub dependencies so the tests exercise passenger_menu() only.
extern "C" {

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

void search_trains_between_stations(
    const char *src,
    const char *dest,
    const char *date
) {
    ++searchCalls;
    lastSource = src;
    lastDestination = dest;
    lastDate = date;
}

int book_ticket(
    const char *,
    int,
    int,
    TrainClass,
    int,
    PassengerDetail[],
    Ticket *
) {
    ++bookingCalls;
    return bookingResult;
}

void view_booking_by_pnr(const char *pnr) {
    ++viewPNRCalls;
    lastPNR = pnr;
}

int cancel_ticket(const char *pnr) {
    ++cancelCalls;
    lastPNR = pnr;
    return cancelResult;
}

void view_user_booking_history(const char *username) {
    ++historyCalls;
    lastUsername = username;
}

}

// Tests

TEST_F(PassengerMenuTest, LogoutImmediately) {
    RunMenu("6\n");

    EXPECT_NE(
        Output().find("=== Passenger Menu ==="),
        std::string::npos
    );
    EXPECT_EQ(searchCalls, 0);
    EXPECT_EQ(bookingCalls, 0);
}

TEST_F(PassengerMenuTest, InvalidNumericChoiceThenLogout) {
    RunMenu("9\n6\n");

    EXPECT_EQ(searchCalls, 0);
    EXPECT_EQ(bookingCalls, 0);
    EXPECT_EQ(historyCalls, 0);
}

TEST_F(PassengerMenuTest, NonnumericChoiceThenLogout) {
    RunMenu("x\n6\n");

    EXPECT_EQ(searchCalls, 0);
    EXPECT_EQ(bookingCalls, 0);
}

TEST_F(PassengerMenuTest, SearchTrains) {
    RunMenu("1\nChennai\nBengaluru\n2026-12-20\n6\n");

    EXPECT_EQ(searchCalls, 1);
    EXPECT_EQ(lastSource, "Chennai");
    EXPECT_EQ(lastDestination, "Bengaluru");
    EXPECT_EQ(lastDate, "2026-12-20");
}

TEST_F(PassengerMenuTest, ViewBookingByPNR) {
    RunMenu("3\nPNR12345\n6\n");

    EXPECT_EQ(viewPNRCalls, 1);
    EXPECT_EQ(lastPNR, "PNR12345");
}

TEST_F(PassengerMenuTest, CancelTicketSuccessfully) {
    cancelResult = 1;

    RunMenu("4\nPNR12345\n6\n");

    EXPECT_EQ(cancelCalls, 1);
    EXPECT_EQ(lastPNR, "PNR12345");
    EXPECT_NE(
        Output().find("Ticket cancelled and refund processed"),
        std::string::npos
    );
}

TEST_F(PassengerMenuTest, CancelTicketFailure) {
    cancelResult = 0;

    RunMenu("4\nPNR99999\n6\n");

    EXPECT_EQ(cancelCalls, 1);
    EXPECT_NE(
        Output().find("Cancellation failed"),
        std::string::npos
    );
}

TEST_F(PassengerMenuTest, ViewBookingHistory) {
    RunMenu("5\n6\n");

    EXPECT_EQ(historyCalls, 1);
    EXPECT_EQ(lastUsername, "test_user");
}

TEST_F(PassengerMenuTest, BookingWithInvalidPassengerCountIsNotSafelyValidated) {
    // Documents the current input-validation gap without entering the
    // unsafe passenger loop.
    SUCCEED();
}
