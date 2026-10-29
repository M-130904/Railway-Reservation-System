
#include <gtest/gtest.h>

extern "C" {
#include "waitlist.h"
#include "logger.h"
}

// ============================================================
// process_waitlist_promotion() tests
// ============================================================

// Positive: valid schedule and class.
TEST(WaitlistTest, ValidScheduleAndClassReturnsSuccess) {
    EXPECT_EQ(process_waitlist_promotion(1, (TrainClass)0), 1);
}

// Boundary: schedule ID is zero.
TEST(WaitlistTest, ZeroScheduleIdReturnsSuccess) {
    EXPECT_EQ(process_waitlist_promotion(0, (TrainClass)0), 1);
}

// Negative: schedule ID is negative.
TEST(WaitlistTest, NegativeScheduleIdReturnsSuccess) {
    EXPECT_EQ(process_waitlist_promotion(-1, (TrainClass)0), 1);
}

// Boundary: maximum positive int schedule ID.
TEST(WaitlistTest, MaximumScheduleIdReturnsSuccess) {
    EXPECT_EQ(process_waitlist_promotion(2147483647, (TrainClass)0), 1);
}

// Negative: invalid class value.
// Current stub ignores the class and still returns success.
TEST(WaitlistTest, InvalidClassReturnsSuccessInCurrentStub) {
    EXPECT_EQ(process_waitlist_promotion(1, (TrainClass)-1), 1);
}

// Boundary: large class value.
// Current stub ignores the class and still returns success.
TEST(WaitlistTest, LargeClassValueReturnsSuccessInCurrentStub) {
    EXPECT_EQ(process_waitlist_promotion(1, (TrainClass)999), 1);
}
