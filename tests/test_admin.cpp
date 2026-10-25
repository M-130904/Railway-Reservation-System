
#include <cstdio>
#include <gtest/gtest.h>

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
        1
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
