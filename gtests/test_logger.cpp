
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

extern "C" {
#include "logger.h"
}

class LoggerTest : public ::testing::Test {
protected:
    const char *logPath = "data/app.log";
    std::string originalContent;
    bool fileExisted = false;

    void SetUp() override
    {
        std::ifstream file(logPath, std::ios::binary);
        fileExisted = file.good();

        if (fileExisted) {
            originalContent = std::string{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };
        }

        // Start each test with an empty log.
        std::ofstream clearFile(logPath, std::ios::trunc);
        ASSERT_TRUE(clearFile.good());
    }

    void TearDown() override
    {
        if (fileExisted) {
            std::ofstream file(logPath, std::ios::binary | std::ios::trunc);
            file << originalContent;
        } else {
            std::remove(logPath);
        }
    }

    std::string ReadLog()
    {
        std::ifstream file(logPath);
        return std::string{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }
};

TEST_F(LoggerTest, WritesLogMessage)
{
    log_message("INFO", "Test message");

    std::string content = ReadLog();

    EXPECT_NE(content.find("[INFO]"), std::string::npos);
    EXPECT_NE(content.find("Test message"), std::string::npos);
}

TEST_F(LoggerTest, AppendsMultipleMessages)
{
    log_message("INFO", "First message");
    log_message("ERROR", "Second message");

    std::string content = ReadLog();

    EXPECT_NE(content.find("First message"), std::string::npos);
    EXPECT_NE(content.find("Second message"), std::string::npos);
    EXPECT_NE(content.find("[INFO]"), std::string::npos);
    EXPECT_NE(content.find("[ERROR]"), std::string::npos);

    EXPECT_LT(content.find("First message"),
              content.find("Second message"));
}


TEST_F(LoggerTest, HandlesEmptyStrings)
{
    log_message("", "");

    std::string content = ReadLog();

    // The timestamp is followed by an empty level and empty message.
    EXPECT_NE(content.find("[] "), std::string::npos);
    EXPECT_FALSE(content.empty());
}


TEST_F(LoggerTest, HandlesLongMessage)
{
    const std::string message(200, 'A');

    log_message("DEBUG", message.c_str());

    std::string content = ReadLog();

    EXPECT_NE(content.find(message), std::string::npos);
}
