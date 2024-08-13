#include "async_resp.hpp"

#define LOG_SERVICES_TEST_ALTERNATIVE_BASEDIR
#include "log_services.hpp"
#include "manager_logservices_journal.hpp"

#include <systemd/sd-id128.h>

#include <cstdint>
#include <format>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(LogServicesBMCJouralTest, LogServicesBMCJouralGetReturnsError)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    sd_id128_t bootIDOut{};
    uint64_t timestampOut = 0;
    uint64_t indexOut = 0;
    uint64_t timestampIn = 1740970301UL;
    std::string badBootIDStr = "78770392794344a29f81507f3ce5e";
    std::string goodBootIDStr = "78770392794344a29f81507f3ce5e78c";
    sd_id128_t goodBootID{};

    // invalid test cases
    EXPECT_FALSE(getTimestampFromID(shareAsyncResp, "", bootIDOut, timestampOut,
                                    indexOut));
    EXPECT_FALSE(getTimestampFromID(shareAsyncResp, badBootIDStr, bootIDOut,
                                    timestampOut, indexOut));
    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", badBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));
    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", badBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));

    // obtain a goodBootID
    EXPECT_GE(sd_id128_from_string(goodBootIDStr.c_str(), &goodBootID), 0);

    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", goodBootIDStr, "InvalidNum"),
        bootIDOut, timestampOut, indexOut));

    // Success cases
    EXPECT_TRUE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}", goodBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));
    EXPECT_NE(sd_id128_equal(goodBootID, bootIDOut), 0);
    EXPECT_EQ(timestampIn, timestampOut);
    EXPECT_EQ(indexOut, 0);

    // Index of _1 is invalid. First index is omitted
    EXPECT_FALSE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}_1", goodBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));

    // Index of _2 is valid, and should return a zero index (1)
    EXPECT_TRUE(getTimestampFromID(
        shareAsyncResp, std::format("{}_{}_2", goodBootIDStr, timestampIn),
        bootIDOut, timestampOut, indexOut));
    EXPECT_NE(sd_id128_equal(goodBootID, bootIDOut), 0);
    EXPECT_EQ(timestampIn, timestampOut);
    EXPECT_EQ(indexOut, 1);
}

TEST(LogServicesPostCodeParse, PostCodeParse)
{
    uint64_t currentValue = 0;
    uint16_t index = 0;
    EXPECT_TRUE(parsePostCode("B1-2", currentValue, index));
    EXPECT_EQ(currentValue, 2);
    EXPECT_EQ(index, 1);
    EXPECT_TRUE(parsePostCode("B200-300", currentValue, index));
    EXPECT_EQ(currentValue, 300);
    EXPECT_EQ(index, 200);

    EXPECT_FALSE(parsePostCode("", currentValue, index));
    EXPECT_FALSE(parsePostCode("B", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1-", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B1A-2z", currentValue, index));
    // Uint16_t max + 1
    EXPECT_FALSE(parsePostCode("B65536-1", currentValue, index));

    // Uint64_t max + 1
    EXPECT_FALSE(parsePostCode("B1-18446744073709551616", currentValue, index));

    // Negative numbers
    EXPECT_FALSE(parsePostCode("B-1-2", currentValue, index));
    EXPECT_FALSE(parsePostCode("B-1--2", currentValue, index));
}

const std::string referenceEntryID = "1722850727";
const std::string referenceEntry =
    "2024-08-05T09:38:47.632321+00:00 OpenBMC.0.1.IPMIWatchdog,Enabled";

inline std::string helperInsertFakeLogEntry()
{
    std::ofstream redfishlog(redfishLogDir / "redfish");
    EXPECT_TRUE(redfishlog.is_open());

    redfishlog << referenceEntry << '\n';
    redfishlog.close();

    return referenceEntryID;
}

inline void testcasePrepare(const std::string& dirname)
{
    redfishLogDir = dirname;
    deletedRsyslogEventLogEntriesFilename = redfishLogDir /
                                            "deleted_redfish_event_log_entries";
    resolvedRsyslogEventLogEntriesFilename =
        redfishLogDir / "resolved_redfish_event_log_entries";
    std::filesystem::create_directory(redfishLogDir);

    clearRedfishRsyslogFiles();
}

TEST(LogServicesEventLogRsyslogTest, EventLogUniqueEntryID)
{
    std::string logEntry = referenceEntry;
    std::string entryID;

    getUniqueEntryID(logEntry, entryID, true);
    EXPECT_EQ(entryID, referenceEntryID);
}

TEST(LogServicesEventLogRsyslogTest, EventLogUniqueEntryIDCreatesIndex)
{
    std::string logEntry = referenceEntry;
    std::string entryID;

    getUniqueEntryID(logEntry, entryID, true);

    // assert that index is appended for subsequent logs with the same timestamp
    getUniqueEntryID(logEntry, entryID, false);
    EXPECT_EQ(entryID, referenceEntryID + "_1");

    // assert that this index increments
    getUniqueEntryID(logEntry, entryID, false);
    EXPECT_EQ(entryID, referenceEntryID + "_2");

    // assert that the index resets when starting anew
    getUniqueEntryID(logEntry, entryID, true);
    EXPECT_EQ(entryID, referenceEntryID);
}

TEST(LogServicesEventLogRsyslogTest, EventLogClear)
{
    testcasePrepare("my-var-log-basedir-clear");

    helperInsertFakeLogEntry();

    std::vector<std::filesystem::path> redfishLogFiles = {};
    getRedfishLogFiles(redfishLogFiles);
    EXPECT_EQ(redfishLogFiles.size(), 1);

    clearRedfishRsyslogFiles();

    redfishLogFiles = {};
    getRedfishLogFiles(redfishLogFiles);
    EXPECT_EQ(redfishLogFiles.size(), 0);
}

TEST(LogServicesEventLogRsyslogTest, EventLogEntryExists)
{
    testcasePrepare("my-var-log-basedir-entry-exists");

    helperInsertFakeLogEntry();
    EXPECT_TRUE(rsyslogRedfishEventLogEntryExists(referenceEntryID));

    clearRedfishRsyslogFiles();

    EXPECT_FALSE(rsyslogRedfishEventLogEntryExists(referenceEntryID));
}

TEST(LogServicesEventLogRsyslogTest, EventLogResolveEntry)
{
    testcasePrepare("my-var-log-basedir-resolve-entry");

    helperInsertFakeLogEntry();
    EXPECT_FALSE(rsyslogRedfishEventLogEntryIsResolved(referenceEntryID));
    EXPECT_EQ(rsyslogRedfishEventLogResolvedEntries().size(), 0);

    rsyslogRedfishEventLogEntryResolve(referenceEntryID, true);

    EXPECT_TRUE(rsyslogRedfishEventLogEntryIsResolved(referenceEntryID));
    EXPECT_EQ(rsyslogRedfishEventLogResolvedEntries().size(), 1);

    rsyslogRedfishEventLogEntryResolve(referenceEntryID, false);

    EXPECT_FALSE(rsyslogRedfishEventLogEntryIsResolved(referenceEntryID));
    EXPECT_EQ(rsyslogRedfishEventLogResolvedEntries().size(), 0);
}

TEST(LogServicesEventLogRsyslogTest, EventLogDeleteEntry)
{
    testcasePrepare("my-var-log-basedir-delete-entry");

    helperInsertFakeLogEntry();

    EXPECT_TRUE(rsyslogRedfishEventLogEntryExists(referenceEntryID));
    EXPECT_FALSE(rsyslogRedfishEventLogEntryIsDeleted(referenceEntryID));

    deleteRsyslogRedfishEventLogEntry(referenceEntryID);

    EXPECT_TRUE(rsyslogRedfishEventLogEntryIsDeleted(referenceEntryID));
}

} // namespace
} // namespace redfish
