
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include <unistd.h>

extern "C" {
#include "schedule.h"
#include "train.h"
#include "types.h"
#include "logger.h"
}

/*
 * Controlled test double:
 * schedule.c only needs to know whether the requested train exists.
 * Do not link train.o when using this stub.
 */
extern "C" __attribute__((weak))
Train *find_train_by_id(int train_id)
{
    static Train test_train = {};
    if (train_id == 9001) {
        return &test_train;
    }
    return nullptr;
}

namespace {

const char *SCHEDULE_FILE = "data/schedules.txt";

class ScheduleTest : public ::testing::Test {
protected:
    std::string originalSchedules;
    bool schedulesExisted = false;

    void SetUp() override {
        originalSchedules = ReadFile(SCHEDULE_FILE, schedulesExisted);
        ASSERT_TRUE(WriteFile(SCHEDULE_FILE, ""));
        ASSERT_EQ(init_schedules(), 0);
    }

    void TearDown() override {
        RestoreFile(SCHEDULE_FILE, originalSchedules, schedulesExisted);
        init_schedules();
    }

    static std::string ReadFile(const char *path, bool &exists) {
        std::ifstream file(path, std::ios::binary);
        exists = file.good();

        if (!exists) {
            return "";
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        return contents.str();
    }

    static bool WriteFile(const char *path, const std::string &contents) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }

        file << contents;
        return file.good();
    }

    static void RestoreFile(const char *path,
                            const std::string &contents,
                            bool existed) {
        if (existed) {
            WriteFile(path, contents);
        } else {
            std::remove(path);
        }
    }

    static std::string MakeSchedule(
        int id, int trainId = 9001,
        const char *date = "2026-12-15",
        const char *dep = "08:00",
        const char *arr = "14:00",
        int ac1 = 10, int ac2 = 20, int ac3 = 30, int sl = 40,
        int ac1Booked = 0, int ac2Booked = 0,
        int ac3Booked = 0, int slBooked = 0,
        int active = 1) {
        std::ostringstream out;
        out << id << '|' << trainId << '|'
            << date << '|' << dep << '|' << arr << '|'
            << ac1 << '|' << ac2 << '|' << ac3 << '|' << sl << '|'
            << ac1Booked << '|' << ac2Booked << '|'
            << ac3Booked << '|' << slBooked << '|'
            << active << '\n';
        return out.str();
    }

    static std::string CaptureListOutput() {
        fflush(stdout);

        FILE *temp = tmpfile();
        if (temp == nullptr) {
            return "";
        }

        const int savedStdout = dup(fileno(stdout));
        if (savedStdout == -1) {
            fclose(temp);
            return "";
        }

        if (dup2(fileno(temp), fileno(stdout)) == -1) {
            close(savedStdout);
            fclose(temp);
            return "";
        }

        list_schedules();
        fflush(stdout);

        std::string output;
        if (fseek(temp, 0, SEEK_SET) == 0) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), temp) != nullptr) {
                output += buffer;
            }
        }

        dup2(savedStdout, fileno(stdout));
        close(savedStdout);
        fclose(temp);

        return output;
    }
};

/* ---------- Initialization ---------- */

TEST_F(ScheduleTest, InitSchedulesReturnsZeroForEmptyFile) {
    EXPECT_EQ(init_schedules(), 0);
    EXPECT_EQ(find_schedule_by_id(1), nullptr);
}

TEST_F(ScheduleTest, InitSchedulesLoadsValidRecord) {
    ASSERT_TRUE(WriteFile(SCHEDULE_FILE, MakeSchedule(1)));

    EXPECT_EQ(init_schedules(), 1);

    Schedule *schedule = find_schedule_by_id(1);
    ASSERT_NE(schedule, nullptr);

    EXPECT_EQ(schedule->schedule_id, 1);
    EXPECT_EQ(schedule->train_id, 9001);
    EXPECT_STREQ(schedule->journey_date, "2026-12-15");
    EXPECT_STREQ(schedule->departure, "08:00");
    EXPECT_STREQ(schedule->arrival, "14:00");
    EXPECT_EQ(schedule->ac1_cap, 10);
    EXPECT_EQ(schedule->sl_cap, 40);
    EXPECT_EQ(schedule->is_active, 1);
}

TEST_F(ScheduleTest, InitSchedulesIgnoresMalformedRecord) {
    ASSERT_TRUE(WriteFile(
        SCHEDULE_FILE, "not-a-valid-record\n" + MakeSchedule(2)));

    EXPECT_EQ(init_schedules(), 1);
    EXPECT_NE(find_schedule_by_id(2), nullptr);
}

TEST_F(ScheduleTest, InitSchedulesStopsAtMaximumCapacity) {
    std::string data;

    for (int i = 1; i <= 201; ++i) {
        data += MakeSchedule(i);
    }

    ASSERT_TRUE(WriteFile(SCHEDULE_FILE, data));

    EXPECT_EQ(init_schedules(), 200);
    EXPECT_NE(find_schedule_by_id(1), nullptr);
    EXPECT_NE(find_schedule_by_id(200), nullptr);
    EXPECT_EQ(find_schedule_by_id(201), nullptr);
}

/* ---------- Persistence ---------- */

TEST_F(ScheduleTest, SaveSchedulesSucceedsForEmptyArray) {
    EXPECT_EQ(init_schedules(), 0);
    EXPECT_EQ(save_schedules(), 1);
}

TEST_F(ScheduleTest, SaveSchedulesPersistsAddedSchedule) {
    ASSERT_EQ(add_schedule(
        40, 9001, "2026-12-15", "08:00", "14:00",
        10, 20, 30, 40), 1);

    EXPECT_EQ(save_schedules(), 1);

    // Reload from disk to verify the persisted record.
    EXPECT_EQ(init_schedules(), 1);
    EXPECT_NE(find_schedule_by_id(40), nullptr);
}

/* ---------- Add schedule ---------- */

TEST_F(ScheduleTest, AddScheduleSucceedsForValidInput) {
    EXPECT_EQ(add_schedule(
        10, 9001, "2026-12-15", "08:00", "14:00",
        10, 20, 30, 40), 1);

    Schedule *schedule = find_schedule_by_id(10);
    ASSERT_NE(schedule, nullptr);

    EXPECT_EQ(schedule->train_id, 9001);
    EXPECT_EQ(schedule->ac1_cap, 10);
    EXPECT_EQ(schedule->ac2_cap, 20);
    EXPECT_EQ(schedule->ac3_cap, 30);
    EXPECT_EQ(schedule->sl_cap, 40);
    EXPECT_EQ(schedule->ac1_booked, 0);
    EXPECT_EQ(schedule->is_active, 1);
}

TEST_F(ScheduleTest, AddScheduleRejectsNullDate) {
    EXPECT_EQ(add_schedule(
        11, 9001, nullptr, "08:00", "14:00",
        10, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsNullDeparture) {
    EXPECT_EQ(add_schedule(
        11, 9001, "2026-12-15", nullptr, "14:00",
        10, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsNullArrival) {
    EXPECT_EQ(add_schedule(
        11, 9001, "2026-12-15", "08:00", nullptr,
        10, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsUnknownTrain) {
    EXPECT_EQ(add_schedule(
        12, 999999, "2026-12-15", "08:00", "14:00",
        10, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsDuplicateId) {
    ASSERT_EQ(add_schedule(
        13, 9001, "2026-12-15", "08:00", "14:00",
        10, 20, 30, 40), 1);

    EXPECT_EQ(add_schedule(
        13, 9001, "2026-12-16", "09:00", "15:00",
        10, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsNegativeCapacity) {
    EXPECT_EQ(add_schedule(
        14, 9001, "2026-12-15", "08:00", "14:00",
        -1, 20, 30, 40), 0);
}

TEST_F(ScheduleTest, AddScheduleAcceptsZeroCapacity) {
    EXPECT_EQ(add_schedule(
        15, 9001, "2026-12-15", "08:00", "14:00",
        0, 0, 0, 0), 1);

    Schedule *schedule = find_schedule_by_id(15);
    ASSERT_NE(schedule, nullptr);
    EXPECT_EQ(schedule->ac1_cap, 0);
    EXPECT_EQ(schedule->sl_cap, 0);
}

TEST_F(ScheduleTest, AddScheduleRejectsCapacityWhenArrayIsFull) {
    std::string data;

    for (int i = 1; i <= 200; ++i) {
        data += MakeSchedule(i);
    }

    ASSERT_TRUE(WriteFile(SCHEDULE_FILE, data));
    ASSERT_EQ(init_schedules(), 200);

    EXPECT_EQ(add_schedule(
        201, 9001, "2026-12-16", "09:00", "15:00",
        10, 20, 30, 40), 0);
}

/* ---------- Lookup ---------- */

TEST_F(ScheduleTest, FindScheduleReturnsNullForUnknownId) {
    EXPECT_EQ(find_schedule_by_id(12345), nullptr);
}

TEST_F(ScheduleTest, FindScheduleDoesNotReturnInactiveSchedule) {
    ASSERT_TRUE(WriteFile(
        SCHEDULE_FILE,
        MakeSchedule(20, 9001, "2026-12-15", "08:00", "14:00",
                     10, 20, 30, 40, 0, 0, 0, 0, 0)));

    init_schedules();

    EXPECT_EQ(find_schedule_by_id(20), nullptr);
}

/* ---------- Listing ---------- */

TEST_F(ScheduleTest, ListSchedulesPrintsEmptyMessage) {
    EXPECT_EQ(init_schedules(), 0);

    const std::string output = CaptureListOutput();
    EXPECT_NE(output.find("No schedules found."), std::string::npos);
}

TEST_F(ScheduleTest, ListSchedulesShowsActiveSchedule) {
    ASSERT_TRUE(WriteFile(SCHEDULE_FILE, MakeSchedule(30)));
    init_schedules();

    const std::string output = CaptureListOutput();

    EXPECT_NE(output.find("Sched ID: 30"), std::string::npos);
    EXPECT_NE(output.find("Train ID: 9001"), std::string::npos);
}

TEST_F(ScheduleTest, ListSchedulesHidesInactiveSchedule) {
    ASSERT_TRUE(WriteFile(
        SCHEDULE_FILE,
        MakeSchedule(31, 9001, "2026-12-15", "08:00", "14:00",
                     10, 20, 30, 40, 0, 0, 0, 0, 0)));
    init_schedules();

    const std::string output = CaptureListOutput();

    EXPECT_EQ(output.find("Sched ID: 31"), std::string::npos);
}

} // namespace
