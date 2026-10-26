#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <unistd.h>

extern "C" {
    #include "admin.h"
    #include "train.h"
    #include "schedule.h"
    #include "report.h"
    #include "utils.h"
}

class AdminModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        init_trains();
        init_schedules();
    }
};

// 1. Add Train: Positive Test
TEST_F(AdminModuleTest, AddTrain_Positive) {
    EXPECT_EQ(
        add_train(
            9001,
            "Rajdhani Express",
            "Delhi",
            "Mumbai",
            500,
            1500.0
        ),
        1
    );

    list_trains();
}

// 2. Add Train: Duplicate ID Negative Test
TEST_F(AdminModuleTest, AddTrain_DuplicateID) {
    ASSERT_EQ(
        add_train(
            9001,
            "Rajdhani Express",
            "Delhi",
            "Mumbai",
            500,
            1500.0
        ),
        0
    );

    EXPECT_EQ(
        add_train(
            9001,
            "Duplicate Train",
            "Delhi",
            "Pune",
            300,
            1000.0
        ),
        0
    );

    list_trains();
}

// 3. Add Schedule: Positive Test
TEST_F(AdminModuleTest, AddSchedule_Positive) {
    ASSERT_EQ(
        add_train(
            9002,
            "Shatabdi Test",
            "Delhi",
            "Agra",
            300,
            800.0
        ),
        1
    );

    EXPECT_EQ(
        add_schedule(
            9501,
            9002,
            "2027-06-10",
            "06:00",
            "09:00",
            50,
            50,
            50,
            50
        ),
        1
    );

    list_schedules();
}

// 4. Add Schedule: Invalid Train ID Negative Test
TEST_F(AdminModuleTest, AddSchedule_InvalidTrainID) {
    EXPECT_EQ(
        add_schedule(
            9502,
            9999,
            "2027-06-11",
            "06:00",
            "09:00",
            50,
            50,
            50,
            50
        ),
        0
    );

    list_schedules();
}

// 5. Add Schedule: Duplicate ID Negative Test
TEST_F(AdminModuleTest, AddSchedule_DuplicateID) {
    ASSERT_EQ(
        add_train(
            9003,
            "Schedule Test Train",
            "Delhi",
            "Agra",
            300,
            800.0
        ),
        1
    );

    ASSERT_EQ(
        add_schedule(
            9503,
            9003,
            "2027-06-12",
            "06:00",
            "09:00",
            50,
            50,
            50,
            50
        ),
        1
    );

    EXPECT_EQ(
        add_schedule(
            9503,
            9003,
            "2027-06-13",
            "06:00",
            "09:00",
            50,
            50,
            50,
            50
        ),
        0
    );

    list_schedules();
}

// 6. System Reports
TEST_F(AdminModuleTest, SystemReports_Coverage) {
    generate_system_reports();

    SUCCEED();
}
static void run_admin_menu(const std::string& input) {
    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);

    ASSERT_EQ(
        fwrite(input.data(), 1, input.size(), file),
        input.size()
    );

    rewind(file);

    int saved_stdin = dup(STDIN_FILENO);
    ASSERT_NE(saved_stdin, -1);

    ASSERT_NE(dup2(fileno(file), STDIN_FILENO), -1);
    clearerr(stdin);

    admin_menu();

    ASSERT_NE(dup2(saved_stdin, STDIN_FILENO), -1);
    close(saved_stdin);
    fclose(file);
    clearerr(stdin);
}
TEST_F(AdminModuleTest, AdminMenu_AllNormalPathsAndNumericErrors) {
    int train_id = 700000 + (getpid() % 100000);
    int schedule_id = 800000 + (getpid() % 100000);

    std::ostringstream input;

    // Add train successfully
    input << "1\n"
          << train_id << "\n"
          << "Coverage Test Train\n"
          << "Delhi\n"
          << "Mumbai\n"
          << "100\n"
          << "500.0\n";

    // Add the same train again: failure
    input << "1\n"
          << train_id << "\n"
          << "Duplicate Train\n"
          << "Delhi\n"
          << "Mumbai\n"
          << "100\n"
          << "500.0\n";

    // List trains
    input << "2\n";

    // Add schedule successfully
    input << "3\n"
          << schedule_id << "\n"
          << train_id << "\n"
          << "2027-06-10\n"
          << "06:00\n"
          << "09:00\n"
          << "25 25 25 25\n";

    // Duplicate schedule: failure
    input << "3\n"
          << schedule_id << "\n"
          << train_id << "\n"
          << "2027-06-11\n"
          << "06:00\n"
          << "09:00\n"
          << "25 25 25 25\n";

    // List schedules and generate reports
    input << "4\n"
          << "5\n";

    // Invalid menu choice
    input << "9\n";

    // Invalid train ID
    input << "1\nabc\n";

    // Invalid train capacity
    input << "1\n"
          << train_id + 1 << "\n"
          << "Capacity Test\nDelhi\nMumbai\n"
          << "abc\n";

    // Invalid base fare
    input << "1\n"
          << train_id + 2 << "\n"
          << "Fare Test\nDelhi\nMumbai\n"
          << "100\nabc\n";

    // Invalid schedule ID
    input << "3\nabc\n";

    // Invalid schedule train ID
    input << "3\n"
          << schedule_id + 1 << "\n"
          << "abc\n";

    // Invalid schedule capacity values
    input << "3\n"
          << schedule_id + 2 << "\n"
          << train_id << "\n"
          << "2027-06-12\n"
          << "06:00\n"
          << "09:00\n"
          << "25 abc 25 25\n";

    // Logout
    input << "6\n";

    run_admin_menu(input.str());
}
TEST_F(AdminModuleTest, AdminMenu_InvalidMenuInput) {
    run_admin_menu("abc\n6\n");
}

TEST_F(AdminModuleTest, AdminMenu_EOFAtJourneyDate) {
    run_admin_menu("3\n95001\n101\n");
}

TEST_F(AdminModuleTest, AdminMenu_EOFAtDeparture) {
    run_admin_menu("3\n95002\n101\n2027-06-15\n");
}

TEST_F(AdminModuleTest, AdminMenu_EOFAtArrival) {
    run_admin_menu("3\n95003\n101\n2027-06-16\n06:00\n");
}
