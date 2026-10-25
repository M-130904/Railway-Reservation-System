
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>

extern "C" {
#include "report.h"
}

class ReportTest : public ::testing::Test {
protected:
    std::string originalContents;
    bool originalFileExisted = false;
    FILE *outputFile = nullptr;
    int savedStdout = -1;

    static const char *ReservationFile() {
        return "data/reservations.txt";
    }

    static std::string MakeReservation(
        const std::string &pnr, int status, double fare = 450.0) {
        return pnr + "|101|1|alice|4|1|John:30:M|" +
               std::to_string(fare) + "|" +
               std::to_string(status) +
               "|0|TXN12345678|PAID|2026-10-25\n";
    }

    void WriteReservations(const std::string &contents) {
        std::ofstream file(ReservationFile(), std::ios::trunc);
        ASSERT_TRUE(file.is_open());

        file << contents;
        file.close();

        ASSERT_TRUE(file.good());
    }

    void SetUp() override {
        std::ifstream existing(ReservationFile(), std::ios::binary);
        originalFileExisted = existing.is_open();

        if (originalFileExisted) {
            originalContents.assign(
                std::istreambuf_iterator<char>(existing),
                std::istreambuf_iterator<char>());
        }

        outputFile = tmpfile();
        ASSERT_NE(outputFile, nullptr);

        savedStdout = dup(STDOUT_FILENO);
        ASSERT_NE(savedStdout, -1);
    }

    void TearDown() override {
        fflush(stdout);

        if (savedStdout != -1) {
            dup2(savedStdout, STDOUT_FILENO);
            close(savedStdout);
            savedStdout = -1;
        }

        if (outputFile != nullptr) {
            fclose(outputFile);
            outputFile = nullptr;
        }

        if (originalFileExisted) {
            std::ofstream restore(
                ReservationFile(),
                std::ios::binary | std::ios::trunc);

            restore << originalContents;
        } else {
            std::remove(ReservationFile());
        }
    }

    std::string CaptureReport() {
        fflush(stdout);

        if (outputFile == nullptr || savedStdout == -1) {
            return "";
        }

        if (fseek(outputFile, 0, SEEK_SET) != 0) {
            return "";
        }

        if (dup2(fileno(outputFile), STDOUT_FILENO) == -1) {
            return "";
        }

        clearerr(stdout);
        generate_system_reports();
        fflush(stdout);

        if (dup2(savedStdout, STDOUT_FILENO) == -1) {
            return "";
        }

        clearerr(stdout);

        if (fseek(outputFile, 0, SEEK_SET) != 0) {
            return "";
        }

        std::string result;
        char buffer[512];
        size_t count;

        while ((count = fread(buffer, 1, sizeof(buffer), outputFile)) > 0) {
            result.append(buffer, count);
        }

        return result;
    }
};

TEST_F(ReportTest, EmptyFile) {
    WriteReservations("");

    EXPECT_NE(
        CaptureReport().find("No reservations recorded yet"),
        std::string::npos);
}

TEST_F(ReportTest, MissingReservationFile) {
    ASSERT_EQ(std::remove(ReservationFile()), 0);

    EXPECT_NE(
        CaptureReport().find("No reservations recorded yet"),
        std::string::npos);
}

TEST_F(ReportTest, MalformedRecordIgnored) {
    WriteReservations("this is not a valid reservation record\n");

    EXPECT_NE(
        CaptureReport().find("No reservations recorded yet"),
        std::string::npos);
}

TEST_F(ReportTest, ReportsAllThreeStatuses) {
    std::string records;
    records += MakeReservation("PNR00000001", 1, 450.0);
    records += MakeReservation("PNR00000002", 2, 300.0);
    records += MakeReservation("PNR00000003", 3, 150.0);

    WriteReservations(records);

    const std::string report = CaptureReport();

    EXPECT_NE(report.find("Total Reservations Recorded : 3"),
              std::string::npos);
    EXPECT_NE(report.find("[CONFIRMED TICKETS]"), std::string::npos);
    EXPECT_NE(report.find("[WAITLISTED TICKETS]"), std::string::npos);
    EXPECT_NE(report.find("[CANCELLED TICKETS]"), std::string::npos);
    EXPECT_NE(report.find("Gross Revenue Collected"), std::string::npos);
    EXPECT_NE(report.find("Total Refunds Processed"), std::string::npos);
    EXPECT_NE(report.find("Net System Revenue"), std::string::npos);
}

TEST_F(ReportTest, UnknownStatusDoesNotAffectTotals) {
    WriteReservations(MakeReservation("PNR00000004", 4, 500.0));

    const std::string report = CaptureReport();

    EXPECT_NE(report.find("Total Reservations Recorded : 1"),
              std::string::npos);
    EXPECT_NE(report.find("Gross Revenue Collected     : Rs. 0.00"),
              std::string::npos);
    EXPECT_NE(report.find("Total Refunds Processed     : Rs. 0.00"),
              std::string::npos);
}

TEST_F(ReportTest, MaximumReservationCapacity) {
    std::string records;

    for (int i = 0; i < 1000; ++i) {
        char pnr[15];
        std::snprintf(pnr, sizeof(pnr), "PNR%08d", i);
        records += MakeReservation(pnr, 1, 100.0);
    }

    WriteReservations(records);

    const std::string report = CaptureReport();

    EXPECT_NE(report.find("Total Reservations Recorded : 1000"),
              std::string::npos);
    EXPECT_NE(report.find("[CONFIRMED TICKETS]"), std::string::npos);
}
