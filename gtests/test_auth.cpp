#include <gtest/gtest.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <limits.h>

#ifdef _WIN32
#include <direct.h>
#define MAKE_DIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MAKE_DIR(path) mkdir((path), 0700)
#endif

extern "C" {
#include "auth.h"
#include "logger.h"
}

// Positive: register a new user
TEST(AuthTest, RegisterUser_Positive) {
    EXPECT_EQ(register_user("testuser101", "pass123", ROLE_PASSENGER), 1);
}

// Negative: duplicate username
TEST(AuthTest, RegisterUser_DuplicateUsername) {
    register_user("testuser102", "pass123", ROLE_PASSENGER);

    EXPECT_EQ(register_user("testuser102", "pass456", ROLE_PASSENGER), 0);
}

// Positive: authenticate with correct credentials
TEST(AuthTest, AuthenticateUser_Positive) {
    register_user("testuser103", "pass123", ROLE_PASSENGER);

    UserRole role;
    EXPECT_EQ(authenticate_user("testuser103", "pass123", &role), 1);
    EXPECT_EQ(role, ROLE_PASSENGER);
}

// Negative: incorrect password
TEST(AuthTest, AuthenticateUser_WrongPassword) {
    register_user("testuser104", "pass123", ROLE_PASSENGER);

    UserRole role;
    EXPECT_EQ(authenticate_user("testuser104", "wrongpass", &role), 0);
}

// Negative: username does not exist
TEST(AuthTest, AuthenticateUser_UnknownUsername) {
    UserRole role;
    EXPECT_EQ(authenticate_user("unknownuser999", "pass123", &role), 0);
}

// Boundary: empty username and password
TEST(AuthTest, RegisterUser_EmptyCredentials) {
    EXPECT_EQ(register_user("", "", ROLE_PASSENGER), 1);
}
TEST(AuthTest, RegisterUser_FileOpenFailure) {
    char original_dir[PATH_MAX];
    ASSERT_NE(getcwd(original_dir, sizeof(original_dir)), nullptr);

    ASSERT_EQ(MAKE_DIR("build/auth_failure_test") == 0 ||
          errno == EEXIST, true);
    ASSERT_EQ(chdir("build/auth_failure_test"), 0);

    // No data directory exists here, so both file opens fail.
    EXPECT_EQ(register_user("failure_user", "pass123", ROLE_PASSENGER), 0);

    ASSERT_EQ(chdir(original_dir), 0);
}

TEST(AuthTest, AuthenticateUser_FileOpenFailure) {
    char original_dir[PATH_MAX];
    ASSERT_NE(getcwd(original_dir, sizeof(original_dir)), nullptr);

    ASSERT_EQ(chdir("build/auth_failure_test"), 0);

    UserRole role;
    EXPECT_EQ(authenticate_user("failure_user", "pass123", &role), 0);

    ASSERT_EQ(chdir(original_dir), 0);
}