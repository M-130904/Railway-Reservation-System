
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <unistd.h>

extern "C" {
#include "search.h"
}

namespace {

const char *TRAINS_FILE = "data/trains.txt";
const char *SCHEDULES_FILE = "data/schedules.txt";

class SearchTest : public ::testing::Test {
protected:
    std::string originalTrains;
    std::string originalSchedules;
    bool trainsExisted = false;
    bool schedulesExisted = false;

    int savedStdout = -1;
    FILE *captureFile = nullptr;

    static std::string readFile(const char *path, bool &exists)
    {
        std::ifstream file(path);
        exists = file.good();

        if (!exists) {
            return "";
        }

        std::ostringstream content;
        content << file.rdbuf();
        return content.str();
    }

    static void writeFile(const char *path, const std::string &content)
    {
        std::ofstream file(path, std::ios::trunc);

        if (!file) {
            throw std::runtime_error(
                std::string("Cannot write test file: ") + path);
        }

        file << content;
    }

    void SetUp() override
    {
        originalTrains = readFile(TRAINS_FILE, trainsExisted);
        originalSchedules = readFile(SCHEDULES_FILE, schedulesExisted);

        captureFile = tmpfile();
        ASSERT_NE(captureFile, nullptr);

        fflush(stdout);

        savedStdout = dup(fileno(stdout));
        ASSERT_GE(savedStdout, 0);

        ASSERT_EQ(
            dup2(fileno(captureFile), fileno(stdout)),
            fileno(stdout));

        writeFile(TRAINS_FILE, "");
        writeFile(SCHEDULES_FILE, "");
    }

    void TearDown() override
    {
        fflush(stdout);

        if (savedStdout >= 0) {
            dup2(savedStdout, fileno(stdout));
            close(savedStdout);
            savedStdout = -1;
        }

        if (captureFile != nullptr) {
            fclose(captureFile);
            captureFile = nullptr;
        }

        if (trainsExisted) {
            writeFile(TRAINS_FILE, originalTrains);
        } else {
            std::remove(TRAINS_FILE);
        }

        if (schedulesExisted) {
            writeFile(SCHEDULES_FILE, originalSchedules);
        } else {
            std::remove(SCHEDULES_FILE);
        }
    }

    std::string output()
    {
        fflush(stdout);

        // Do not use ASSERT_* here: this function returns std::string.
        if (captureFile == nullptr) {
            ADD_FAILURE() << "stdout capture file is unavailable";
            return "";
        }

        if (fseek(captureFile, 0, SEEK_SET) != 0) {
            ADD_FAILURE() << "Could not rewind stdout capture file";
            return "";
        }

        std::string result;
        char buffer[512];

        while (fgets(buffer, sizeof(buffer), captureFile) != nullptr) {
            result += buffer;
        }

        return result;
    }

    void addTrain(
        int id,
        const char *name,
        const char *source,
        const char *destination,
        int capacity = 100,
        double fare = 450.0,
        int active = 1)
    {
        std::ofstream file(TRAINS_FILE, std::ios::app);

        file << id << "|" << name << "|" << source << "|"
             << destination << "|" << capacity << "|" << fare
             << "|" << active << "\n";
    }

    void addSchedule(
        int scheduleId,
        int trainId,
        const char *date = "2026-10-25",
        const char *departure = "08:00",
        const char *arrival = "12:00",
        int active = 1)
    {
        std::ofstream file(SCHEDULES_FILE, std::ios::app);

        file << scheduleId << "|" << trainId << "|" << date
             << "|" << departure << "|" << arrival
             << "|30|30|30|100"
             << "|0|0|0|0|" << active << "\n";
    }

    void search(
        const char *source = "Chennai",
        const char *destination = "Bengaluru",
        const char *date = "2026-10-25")
    {
        search_trains_between_stations(source, destination, date);
    }
};

// ---------- Positive tests ----------

TEST_F(SearchTest, FindsMatchingTrainAndSchedule)
{
    addTrain(101, "Chennai Express", "Chennai", "Bengaluru");
    addSchedule(1, 101);

    search();

    std::string result = output();

    EXPECT_NE(result.find("Chennai Express"), std::string::npos);
    EXPECT_NE(result.find("Sched ID: 1"), std::string::npos);
    EXPECT_NE(result.find("Rs. 450.00"), std::string::npos);
}

TEST_F(SearchTest, StationMatchingIsCaseInsensitive)
{
    addTrain(101, "Chennai Express", "CHENNAI", "BENGALURU");
    addSchedule(1, 101);

    search("chennai", "bengaluru");

    EXPECT_NE(output().find("Chennai Express"), std::string::npos);
}

TEST_F(SearchTest, FindsMultipleMatchingSchedules)
{
    addTrain(101, "Chennai Express", "Chennai", "Bengaluru");
    addSchedule(1, 101);
    addSchedule(2, 101, "2026-10-25", "18:00", "22:00");

    search();

    std::string result = output();

    EXPECT_NE(result.find("Sched ID: 1"), std::string::npos);
    EXPECT_NE(result.find("Sched ID: 2"), std::string::npos);
}

TEST_F(SearchTest, FindsMultipleMatchingTrains)
{
    addTrain(101, "Express One", "Chennai", "Bengaluru");
    addTrain(102, "Express Two", "Chennai", "Bengaluru");
    addSchedule(1, 101);
    addSchedule(2, 102);

    search();

    std::string result = output();

    EXPECT_NE(result.find("Express One"), std::string::npos);
    EXPECT_NE(result.find("Express Two"), std::string::npos);
}

// ---------- Negative tests ----------

TEST_F(SearchTest, NoTrainsMatchRoute)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");
    addSchedule(1, 101);

    search("Mumbai", "Pune");

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, NoScheduleMatchesRequestedDate)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");
    addSchedule(1, 101, "2026-10-24");

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, InactiveTrainIsNotDisplayed)
{
    addTrain(101, "Inactive Express", "Chennai", "Bengaluru",
             100, 450.0, 0);
    addSchedule(1, 101);

    search();

    EXPECT_EQ(output().find("Inactive Express"), std::string::npos);
}

TEST_F(SearchTest, InactiveScheduleIsNotDisplayed)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");
    addSchedule(1, 101, "2026-10-25", "08:00", "12:00", 0);

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, EmptyFilesReturnNoMatches)
{
    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, MalformedRecordsAreIgnored)
{
    writeFile(TRAINS_FILE, "this is not a valid train record\n");
    writeFile(SCHEDULES_FILE, "invalid schedule data\n");

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, ScheduleForDifferentTrainIsNotMatched)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");
    addSchedule(1, 999);

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

// ---------- Boundary tests ----------

TEST_F(SearchTest, HandlesMaximumTrainRecordCount)
{
    for (int i = 0; i < 100; ++i) {
        addTrain(1000 + i, "Train", "Other", "Destination");
    }

    addTrain(2000, "Beyond Limit", "Chennai", "Bengaluru");
    addSchedule(1, 2000);

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, HandlesMaximumScheduleRecordCount)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");

    for (int i = 0; i < 500; ++i) {
        addSchedule(1000 + i, 999, "2026-10-24");
    }

    addSchedule(2000, 101);

    search();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(SearchTest, EmptyStationNameDoesNotMatchNormalRoute)
{
    addTrain(101, "Express", "Chennai", "Bengaluru");
    addSchedule(1, 101);

    search("", "Bengaluru");

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

} // namespace
