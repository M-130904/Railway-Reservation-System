
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

extern "C" {
#include "utils.h"
}

class UtilsTest : public ::testing::Test {
protected:
    int originalStdin = -1;

    void SetUp() override {
        originalStdin = dup(STDIN_FILENO);
        ASSERT_NE(originalStdin, -1);

        // Reopen stdin to reset its internal C stdio buffer.
        ASSERT_NE(freopen("/dev/null", "r", stdin), nullptr);
    }

    void TearDown() override {
        // Reset stdin's stream state before restoring the original descriptor.
        if (freopen("/dev/null", "r", stdin) != nullptr &&
            originalStdin != -1) {
            dup2(originalStdin, STDIN_FILENO);
        }

        if (originalStdin != -1) {
            close(originalStdin);
            originalStdin = -1;
        }

        clearerr(stdin);
    }

    void setStdin(const std::string &input) {
        char path[] = "/tmp/test_utils_stdin_XXXXXX";
        int fd = mkstemp(path);
        ASSERT_NE(fd, -1);

        size_t written = 0;

        while (written < input.size()) {
            ssize_t count = write(
                fd,
                input.data() + written,
                input.size() - written
            );

            if (count <= 0) {
                close(fd);
                unlink(path);
                FAIL() << "Failed to write test input";
                return;
            }

            written += static_cast<size_t>(count);
        }

        ASSERT_EQ(close(fd), 0);

        // freopen resets both the file descriptor and stdio buffering.
        FILE *result = freopen(path, "r", stdin);
        unlink(path);

        ASSERT_NE(result, nullptr);
        clearerr(stdin);
    }
};

// ============================================================
// clear_input_buffer() tests
// ============================================================

TEST_F(UtilsTest, ClearInputBufferConsumesCharactersUntilNewline) {
    setStdin("abc\nremaining\n");

    clear_input_buffer();

    EXPECT_EQ(getchar(), 'r');
}

TEST_F(UtilsTest, ClearInputBufferHandlesImmediateNewline) {
    setStdin("\nX");

    clear_input_buffer();

    EXPECT_EQ(getchar(), 'X');
}

TEST_F(UtilsTest, ClearInputBufferHandlesEndOfFile) {
    setStdin("");

    clear_input_buffer();

    EXPECT_EQ(getchar(), EOF);
}

// ============================================================
// safe_fgets() tests
// ============================================================

TEST_F(UtilsTest, SafeFgetsReadsInputAndRemovesNewline) {
    setStdin("Railway\n");

    char buffer[32] = {};

    safe_fgets(buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, "Railway");
}

TEST_F(UtilsTest, SafeFgetsReadsInputWithoutNewline) {
    setStdin("Reservation");

    char buffer[32] = {};

    safe_fgets(buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, "Reservation");
}

TEST_F(UtilsTest, SafeFgetsHandlesEmptyLine) {
    setStdin("\n");

    char buffer[32] = "initial";

    safe_fgets(buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, "");
}

TEST_F(UtilsTest, SafeFgetsRespectsBufferBoundary) {
    setStdin("ABCDEFG\n");

    char buffer[5] = {};

    safe_fgets(buffer, sizeof(buffer));

    EXPECT_STREQ(buffer, "ABCD");
    EXPECT_EQ(buffer[4], '\0');
}

TEST_F(UtilsTest, SafeFgetsHandlesEndOfFile) {
    setStdin("");

    char buffer[16] = "initial";

    safe_fgets(buffer, sizeof(buffer));

    // Current implementation leaves the buffer unchanged if fgets returns NULL.
    EXPECT_STREQ(buffer, "initial");
}

// ============================================================
// generate_pnr() tests
// ============================================================

TEST(UtilsPNRTest, GeneratesPNRWithExpectedPrefix) {
    char pnr[32] = {};

    generate_pnr(pnr);

    ASSERT_EQ(std::strlen(pnr), 13U);
    EXPECT_EQ(std::string(pnr).substr(0, 7), "PNR2026");
}

TEST(UtilsPNRTest, GeneratesPNRWithExactlySixDigits) {
    char pnr[32] = {};

    generate_pnr(pnr);

    ASSERT_EQ(std::strlen(pnr), 13U);

    for (int i = 7; i < 13; ++i) {
        EXPECT_GE(pnr[i], '0');
        EXPECT_LE(pnr[i], '9');
    }
}

TEST(UtilsPNRTest, GeneratedNumberIsWithinValidRange) {
    char pnr[32] = {};

    generate_pnr(pnr);

    ASSERT_EQ(std::strlen(pnr), 13U);

    const int number = std::atoi(pnr + 7);

    EXPECT_GE(number, 100000);
    EXPECT_LE(number, 999999);
}

TEST(UtilsPNRTest, GeneratesNonEmptyPNR) {
    char pnr[32] = {};

    generate_pnr(pnr);

    EXPECT_NE(pnr[0], '\0');
}
