
#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>

extern "C" {
#include "file_io.h"
}

class FileIOTest : public ::testing::Test {
protected:
    const char *paths[5] = {
        "data/users.txt",
        "data/trains.txt",
        "data/schedules.txt",
        "data/reservations.txt",
        "data/app.log"
    };

    std::string originals[5];
    bool existed[5]{};

    static std::string ReadFile(const char *path, bool &wasPresent)
    {
        std::ifstream file(path, std::ios::binary);
        wasPresent = file.good();

        if (!wasPresent) {
            return "";
        }

        return std::string{
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
    }

    void SetUp() override
    {
        struct stat info {};
        ASSERT_EQ(stat("data", &info), 0)
            << "Run tests from the project root; data/ must exist.";

        for (int i = 0; i < 5; ++i) {
            originals[i] = ReadFile(paths[i], existed[i]);
        }
    }

    void TearDown() override
    {
        for (int i = 0; i < 5; ++i) {
            if (existed[i]) {
                std::ofstream file(paths[i], std::ios::binary | std::ios::trunc);
                file << originals[i];
            } else {
                std::remove(paths[i]);
            }
        }
    }
};

TEST_F(FileIOTest, InitializesAllDataFiles)
{
    init_data_files();

    for (int i = 0; i < 5; ++i) {
        std::ifstream file(paths[i]);
        EXPECT_TRUE(file.good()) << "File missing: " << paths[i];
    }
}

TEST_F(FileIOTest, PreservesExistingFileContents)
{
    const std::string marker = "FILE_IO_TEST_MARKER\n";

    {
        std::ofstream file("data/users.txt", std::ios::app);
        ASSERT_TRUE(file.good());
        file << marker;
    }

    init_data_files();

    std::ifstream file("data/users.txt");
    std::string content{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    EXPECT_NE(content.find(marker), std::string::npos);
}

TEST_F(FileIOTest, CanInitializeRepeatedly)
{
    init_data_files();
    init_data_files();

    for (int i = 0; i < 5; ++i) {
        std::ifstream file(paths[i]);
        EXPECT_TRUE(file.good()) << "File missing: " << paths[i];
    }
}
