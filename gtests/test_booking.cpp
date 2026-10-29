#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>

extern "C" {
#include "booking.h"
#include "train.h"
#include "schedule.h"
#include "types.h"
}

class BookingTest : public ::testing::Test {
protected:
    PassengerDetail passengers[MAX_PASSENGERS]{};
    Ticket ticket{};

    std::string originalSchedules;
    std::string originalReservations;
    bool schedulesExisted = false;
    bool reservationsExisted = false;

    static std::string ReadFile(const char *path, bool &existed) {
        std::ifstream file(path);
        existed = file.good();
        if (!existed) return "";
        return std::string(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }

    static void RestoreFile(const char *path, const std::string &content,
                            bool existed) {
        if (!existed) {
            std::remove(path);
            return;
        }
        std::ofstream file(path);
        file << content;
    }

    int BookWithPayment(int trainId, int scheduleId,
                        TrainClass trainClass, int count) {
        FILE *input = tmpfile();
        if (!input) return 0;

        fputs("2\n", input);
        rewind(input);

        int savedStdin = dup(STDIN_FILENO);
        if (savedStdin == -1 ||
            dup2(fileno(input), STDIN_FILENO) == -1) {
            if (savedStdin != -1) close(savedStdin);
            fclose(input);
            return 0;
        }

        clearerr(stdin);
        int result = book_ticket("testuser", trainId, scheduleId,
                                 trainClass, count, passengers, &ticket);

        dup2(savedStdin, STDIN_FILENO);
        close(savedStdin);
        clearerr(stdin);
        fclose(input);
        return result;
    }

    void SetUp() override {
        originalSchedules =
            ReadFile("data/schedules.txt", schedulesExisted);
        originalReservations =
            ReadFile("data/reservations.txt", reservationsExisted);

        std::strcpy(passengers[0].name, "Test Passenger");
        passengers[0].age = 25;
        passengers[0].gender = 'M';

        std::strcpy(passengers[1].name, "Second Passenger");
        passengers[1].age = 30;
        passengers[1].gender = 'F';

        init_trains();
        init_schedules();
    }

    void TearDown() override {
        RestoreFile("data/schedules.txt", originalSchedules,
                    schedulesExisted);
        RestoreFile("data/reservations.txt", originalReservations,
                    reservationsExisted);
    }
};

TEST_F(BookingTest, InvalidTrain) {
    EXPECT_EQ(book_ticket("testuser", -999, 1, CLASS_SLEEPER,
                          1, passengers, &ticket), 0);
}

TEST_F(BookingTest, InvalidSchedule) {
    EXPECT_NE(find_train_by_id(101), nullptr);

    EXPECT_EQ(book_ticket("testuser", 101, -999, CLASS_SLEEPER,
                          1, passengers, &ticket), 0);
}

TEST_F(BookingTest, ConfirmedBooking) {
    ASSERT_NE(find_train_by_id(101), nullptr);
    ASSERT_NE(find_schedule_by_id(1), nullptr);

    EXPECT_EQ(BookWithPayment(101, 1, CLASS_AC1, 1), 1);
    EXPECT_EQ(ticket.status, STATUS_CONFIRMED);
    EXPECT_DOUBLE_EQ(ticket.total_fare, 1125.0);
    EXPECT_GT(std::strlen(ticket.pnr), 0U);
    EXPECT_STREQ(ticket.payment_status, "SUCCESS");
}

TEST_F(BookingTest, WaitlistedBooking) {
    ASSERT_NE(find_train_by_id(101), nullptr);
    ASSERT_NE(find_schedule_by_id(2), nullptr);

    Schedule *schedule = find_schedule_by_id(2);
    ASSERT_NE(schedule, nullptr);

    // Reset sleeper bookings for a predictable test.
    schedule->sl_booked = 0;

    EXPECT_EQ(BookWithPayment(101, 2, CLASS_SLEEPER, 2), 1);
    EXPECT_EQ(ticket.status, STATUS_WAITLIST);
    EXPECT_EQ(ticket.seat_or_wl_number, 1);
}

TEST_F(BookingTest, ViewBookingByPNR_NotFound) {
    EXPECT_NO_THROW(view_booking_by_pnr("PNR_DOES_NOT_EXIST"));
}

TEST_F(BookingTest, ViewHistory_NoMatchingUser) {
    EXPECT_NO_THROW(view_user_booking_history("user_not_found"));
}
TEST_F(BookingTest, ViewBookingByPNR_Found) {
    std::ofstream file("data/reservations.txt", std::ios::app);
    ASSERT_TRUE(file.is_open());

    file << "PNR_TEST123|101|1|testuser|4|1|Test Passenger:25:M"
            "|450.00|1|1|TXN123|SUCCESS|2026-12-20\n";
    file.close();

    EXPECT_NO_THROW(view_booking_by_pnr("PNR_TEST123"));
}

TEST_F(BookingTest, ViewHistory_MatchingUser) {
    std::ofstream file("data/reservations.txt", std::ios::app);
    ASSERT_TRUE(file.is_open());

    file << "PNR_TEST456|101|1|testuser|4|1|Test Passenger:25:M"
            "|450.00|1|1|TXN456|SUCCESS|2026-12-20\n";
    file.close();

    EXPECT_NO_THROW(view_user_booking_history("testuser"));
}