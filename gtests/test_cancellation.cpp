
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

extern "C" {
#include "cancellation.h"
#include "types.h"
}

class CancellationTest : public ::testing::Test {
protected:
    std::string originalSchedules;
    std::string originalReservations;
    bool schedulesExisted = false;
    bool reservationsExisted = false;

    static std::string ReadFile(const char *path, bool &existed)
    {
        std::ifstream file(path);
        existed = file.good();

        if (!existed) {
            return "";
        }

        return std::string{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }

    static void WriteFile(const char *path, const std::string &content)
    {
        std::ofstream file(path, std::ios::trunc);
        if (!file) {
            throw std::runtime_error(
                std::string("Cannot write fixture: ") + path);
        }

        file << content;

        if (!file) {
            throw std::runtime_error(
                std::string("Cannot finish writing fixture: ") + path);
        }
    }

    static void RestoreFile(
        const char *path,
        const std::string &content,
        bool existed)
    {
        if (!existed) {
            std::remove(path);
            return;
        }

        std::ofstream file(path, std::ios::trunc);
        file << content;
    }

    std::string MakeReservation(
        const char *pnr,
        int status,
        int passengerCount = 1)
    {
        return std::string(pnr) +
            "|101|1|testuser|4|" +
            std::to_string(passengerCount) +
            "|Test Passenger:25:M|450.00|" +
            std::to_string(status) +
            "|1|TXN123|SUCCESS|2026-12-20\n";
    }

    void SetUp() override
    {
        originalSchedules =
            ReadFile("data/schedules.txt", schedulesExisted);

        originalReservations =
            ReadFile("data/reservations.txt", reservationsExisted);

        // Schedule 1: sleeper capacity 5, with 2 seats booked.
        WriteFile(
            "data/schedules.txt",
            "1|101|2026-12-20|06:00|10:00|2|2|2|5|0|0|0|2|1\n");

        WriteFile("data/reservations.txt", "");
    }

    void TearDown() override
    {
        RestoreFile(
            "data/schedules.txt",
            originalSchedules,
            schedulesExisted);

        RestoreFile(
            "data/reservations.txt",
            originalReservations,
            reservationsExisted);
    }
};

TEST_F(CancellationTest, NullPNR)
{
    EXPECT_EQ(cancel_ticket(nullptr), 0);
}

TEST_F(CancellationTest, EmptyPNR)
{
    EXPECT_EQ(cancel_ticket(""), 0);
}

TEST_F(CancellationTest, PNRNotFound)
{
    EXPECT_EQ(cancel_ticket("PNR_NOT_FOUND"), 0);
}

TEST_F(CancellationTest, AlreadyCancelled)
{
    WriteFile(
        "data/reservations.txt",
        MakeReservation("PNR_CANCELLED", STATUS_CANCELLED));

    EXPECT_EQ(cancel_ticket("PNR_CANCELLED"), 0);
}

TEST_F(CancellationTest, CancelConfirmedTicket)
{
    WriteFile(
        "data/reservations.txt",
        MakeReservation("PNR_CONFIRMED", STATUS_CONFIRMED));

    EXPECT_EQ(cancel_ticket("PNR_CONFIRMED"), 1);

    std::ifstream file("data/reservations.txt");
    std::string content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    EXPECT_NE(
        content.find("|" + std::to_string(STATUS_CANCELLED) + "|"),
        std::string::npos);

    std::ifstream schedules("data/schedules.txt");
    std::string scheduleLine;
    ASSERT_TRUE(std::getline(schedules, scheduleLine));

    // Parse the sleeper booked count (field 13).
    int scheduleId, trainId;
    char date[12], departure[6], arrival[6];
    int ac1Cap, ac2Cap, ac3Cap, sleeperCap;
    int ac1Booked, ac2Booked, ac3Booked, sleeperBooked, active;

    ASSERT_EQ(
        sscanf(
            scheduleLine.c_str(),
            "%d|%d|%11[^|]|%5[^|]|%5[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d",
            &scheduleId, &trainId, date, departure, arrival,
            &ac1Cap, &ac2Cap, &ac3Cap, &sleeperCap,
            &ac1Booked, &ac2Booked, &ac3Booked, &sleeperBooked, &active),
        14);

    EXPECT_EQ(sleeperBooked, 1);
}

TEST_F(CancellationTest, CancelWaitlistedTicket)
{
    WriteFile(
        "data/reservations.txt",
        MakeReservation("PNR_WAITLIST", STATUS_WAITLIST));

    EXPECT_EQ(cancel_ticket("PNR_WAITLIST"), 1);

    std::ifstream file("data/reservations.txt");
    std::string content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    EXPECT_NE(
        content.find("|" + std::to_string(STATUS_CANCELLED) + "|"),
        std::string::npos);
}
