
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include <unistd.h>

extern "C" {
#include "train.h"
}

namespace {

const char *TRAINS_FILE = "data/trains.txt";

class TrainTest : public ::testing::Test {
protected:
    std::string originalData;
    bool fileExisted = false;

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
        if (!file) {
            throw std::runtime_error(
                std::string("Failed writing test file: ") + path);
        }
    }

    void SetUp() override
    {
        originalData = readFile(TRAINS_FILE, fileExisted);

        captureFile = tmpfile();
        ASSERT_NE(captureFile, nullptr);

        fflush(stdout);

        savedStdout = dup(fileno(stdout));
        ASSERT_GE(savedStdout, 0);

        ASSERT_EQ(
            dup2(fileno(captureFile), fileno(stdout)),
            fileno(stdout));

        writeFile(TRAINS_FILE, "");
        ASSERT_EQ(init_trains(), 0);
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

        if (fileExisted) {
            writeFile(TRAINS_FILE, originalData);
        } else {
            std::remove(TRAINS_FILE);
        }

        // Restore in-memory state from the original file.
        init_trains();
    }

    std::string output()
    {
        fflush(stdout);

        if (captureFile == nullptr) {
            ADD_FAILURE() << "Output capture file is unavailable";
            return "";
        }

        if (fseek(captureFile, 0, SEEK_SET) != 0) {
            ADD_FAILURE() << "Cannot rewind output capture file";
            return "";
        }

        std::string result;
        char buffer[512];

        while (fgets(buffer, sizeof(buffer), captureFile) != nullptr) {
            result += buffer;
        }

        return result;
    }

    void writeTrain(
        int id,
        const char *name = "Test Express",
        const char *source = "Chennai",
        const char *destination = "Bengaluru",
        int capacity = 100,
        double fare = 450.0,
        int active = 1)
    {
        std::ofstream file(TRAINS_FILE, std::ios::app);

        if (!file) {
            throw std::runtime_error("Cannot append to train test file");
        }

        file << id << "|" << name << "|" << source << "|"
             << destination << "|" << capacity << "|"
             << fare << "|" << active << "\n";

        if (!file) {
            throw std::runtime_error("Failed writing train test record");
        }
    }
};

// ---------- init_trains() ----------

TEST_F(TrainTest, InitEmptyFileReturnsZero)
{
    EXPECT_EQ(init_trains(), 0);
}

TEST_F(TrainTest, InitLoadsValidRecords)
{
    writeTrain(101);
    writeTrain(102, "Express Two", "Mumbai", "Pune");

    EXPECT_EQ(init_trains(), 2);
    EXPECT_NE(find_train_by_id(101), nullptr);
    EXPECT_NE(find_train_by_id(102), nullptr);
}

TEST_F(TrainTest, InitIgnoresMalformedRecords)
{
    writeFile(TRAINS_FILE,
              "invalid record\n"
              "101|Express|Chennai|Bengaluru|100|450.0|1\n");

    EXPECT_EQ(init_trains(), 1);
    EXPECT_NE(find_train_by_id(101), nullptr);
}

TEST_F(TrainTest, InitStopsAtMaximumTrainCapacity)
{
    for (int i = 0; i < 105; ++i) {
        writeTrain(1000 + i);
    }

    EXPECT_EQ(init_trains(), 100);

    // The first 100 records are loaded.
    EXPECT_NE(find_train_by_id(1099), nullptr);

    // The 101st and later records are not loaded.
    EXPECT_EQ(find_train_by_id(1100), nullptr);
    EXPECT_EQ(find_train_by_id(1104), nullptr);
}

// ---------- save_trains() ----------

TEST_F(TrainTest, SaveEmptyArraySucceeds)
{
    EXPECT_EQ(save_trains(), 1);

    bool exists = false;
    EXPECT_EQ(readFile(TRAINS_FILE, exists), "");
    EXPECT_TRUE(exists);
}

TEST_F(TrainTest, SavePersistsAddedTrain)
{
    EXPECT_EQ(add_train(
        101, "Chennai Express", "Chennai", "Bengaluru", 100, 450.0), 1);

    EXPECT_EQ(init_trains(), 1);

    Train *train = find_train_by_id(101);
    ASSERT_NE(train, nullptr);
    EXPECT_STREQ(train->train_name, "Chennai Express");
}

// ---------- add_train() ----------

TEST_F(TrainTest, AddValidTrainSucceeds)
{
    EXPECT_EQ(add_train(
        101, "Chennai Express", "Chennai", "Bengaluru", 100, 450.0), 1);

    Train *train = find_train_by_id(101);
    ASSERT_NE(train, nullptr);

    EXPECT_STREQ(train->train_name, "Chennai Express");
    EXPECT_STREQ(train->source, "Chennai");
    EXPECT_STREQ(train->destination, "Bengaluru");
    EXPECT_EQ(train->capacity, 100);
    EXPECT_DOUBLE_EQ(train->base_fare, 450.0);
    EXPECT_EQ(train->is_active, 1);
}

TEST_F(TrainTest, AddDuplicateIdFails)
{
    ASSERT_EQ(add_train(
        101, "First", "Chennai", "Bengaluru", 100, 450.0), 1);

    EXPECT_EQ(add_train(
        101, "Duplicate", "Mumbai", "Pune", 50, 200.0), 0);

    Train *train = find_train_by_id(101);
    ASSERT_NE(train, nullptr);
    EXPECT_STREQ(train->train_name, "First");
}

TEST_F(TrainTest, AddTrainWithZeroCapacity)
{
    EXPECT_EQ(add_train(
        101, "Zero Capacity", "Chennai", "Bengaluru", 0, 450.0), 1);

    EXPECT_NE(find_train_by_id(101), nullptr);
}

TEST_F(TrainTest, AddTrainWithZeroFare)
{
    EXPECT_EQ(add_train(
        101, "Zero Fare", "Chennai", "Bengaluru", 100, 0.0), 1);

    Train *train = find_train_by_id(101);
    ASSERT_NE(train, nullptr);
    EXPECT_DOUBLE_EQ(train->base_fare, 0.0);
}

TEST_F(TrainTest, AddTrainWithLongNamesTruncatesSafely)
{
    const std::string longName(200, 'A');

    EXPECT_EQ(add_train(
        101, longName.c_str(), "Chennai", "Bengaluru", 100, 450.0), 1);

    Train *train = find_train_by_id(101);
    ASSERT_NE(train, nullptr);

    EXPECT_LT(
        std::strlen(train->train_name),
        sizeof(train->train_name));
}

TEST_F(TrainTest, AddTrainFailsWhenArrayIsFull)
{
    for (int i = 0; i < 100; ++i) {
        ASSERT_EQ(add_train(
            1000 + i, "Train", "Chennai", "Bengaluru", 100, 450.0), 1);
    }

    EXPECT_EQ(add_train(
        9999, "Overflow", "Chennai", "Bengaluru", 100, 450.0), 0);
}

// ---------- list_trains() ----------

TEST_F(TrainTest, ListEmptyTrains)
{
    list_trains();

    EXPECT_NE(output().find("No trains found"), std::string::npos);
}

TEST_F(TrainTest, ListDisplaysActiveTrain)
{
    writeTrain(101);
    ASSERT_EQ(init_trains(), 1);

    list_trains();

    EXPECT_NE(output().find("Test Express"), std::string::npos);
}

TEST_F(TrainTest, ListDoesNotDisplayInactiveTrain)
{
    writeTrain(101, "Inactive Express", "Chennai", "Bengaluru",
               100, 450.0, 0);
    ASSERT_EQ(init_trains(), 1);

    list_trains();

    EXPECT_EQ(output().find("Inactive Express"), std::string::npos);
}

// ---------- find_train_by_id() ----------

TEST_F(TrainTest, FindExistingActiveTrain)
{
    writeTrain(101);
    ASSERT_EQ(init_trains(), 1);

    EXPECT_NE(find_train_by_id(101), nullptr);
}

TEST_F(TrainTest, FindUnknownTrainReturnsNull)
{
    EXPECT_EQ(find_train_by_id(9999), nullptr);
}

TEST_F(TrainTest, FindInactiveTrainReturnsNull)
{
    writeTrain(101, "Inactive", "Chennai", "Bengaluru",
               100, 450.0, 0);
    ASSERT_EQ(init_trains(), 1);

    EXPECT_EQ(find_train_by_id(101), nullptr);
}

} // namespace
