#include "event_log.hpp"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish::event_log
{
namespace
{

TEST(RedfishEventLog, GetUniqueEntryIDSuccess)
{
    bool success = false;
    std::string entryID;
    std::string example = "2000-01-02T03:04:05";
    success = getUniqueEntryID(example, entryID);

    ASSERT_EQ(success, true);

    // assert the prefix since the specific number can depend on timezone
    ASSERT_TRUE(entryID.starts_with("946"));
}

TEST(RedfishEventLog, GetUniqueEntryIDUnique)
{
    bool success = false;
    std::string entryID1;
    std::string entryID2;
    std::string example = "2000-08-02T03:04:05";

    success = getUniqueEntryID(example, entryID1);
    ASSERT_EQ(success, true);
    success = getUniqueEntryID(example, entryID2);
    ASSERT_EQ(success, true);

    // when calling a second time with the same argument
    // there should be an underscore
    ASSERT_TRUE(entryID2.contains("_"));

    // only one '_' allowed
    ASSERT_THAT(entryID2, testing::MatchesRegex("^[0-9]+_[0-9]+$"));
}

TEST(RedfishEventLog, GetUniqueEntryIDIndex)
{
    std::string entryID1;
    std::string entryID2;
    std::string entryID3;
    std::string example = "2000-08-02T03:04:05";

    getUniqueEntryID(example, entryID1);
    getUniqueEntryID(example, entryID2);
    getUniqueEntryID(example, entryID3);

    const size_t index = entryID2.find('_');

    ASSERT_NE(index, std::string::npos);

    const long n1 = std::stol(entryID2.substr(index + 1));

    // unique index for repeated timestamp is >= 0
    ASSERT_GE(n1, 0);

    const long n2 = std::stol(entryID3.substr(entryID3.find('_') + 1));

    // unique index is monotonic increasing
    ASSERT_TRUE(n2 > n1);
}

TEST(RedfishEventLog, GetEventLogParamsSuccess)
{
    int status = 0;
    std::string logEntry = "32938 3,hello";

    std::string timestamp;
    std::string messageID;
    std::vector<std::string> messageArgs;

    status = getEventLogParams(logEntry, timestamp, messageID, messageArgs);

    ASSERT_EQ(status, 0);

    ASSERT_EQ(timestamp, "32938");
    ASSERT_EQ(messageID, "3");
    ASSERT_EQ(messageArgs.size(), 1);
    ASSERT_EQ(messageArgs[0], "hello");
}

TEST(RedfishEventLog, GetEventLogParamsFailNoTimestamp)
{
    int status = 0;
    std::string logEntry = "3,hello";

    std::string timestamp;
    std::string messageID;
    std::vector<std::string> messageArgs;

    status = getEventLogParams(logEntry, timestamp, messageID, messageArgs);

    ASSERT_EQ(status, -EINVAL);
}

TEST(RedfishEventLog, GetEventLogParamsFailNoComma)
{
    int status = 0;
    std::string logEntry = "malformed";

    std::string timestamp;
    std::string messageID;
    std::vector<std::string> messageArgs;

    status = getEventLogParams(logEntry, timestamp, messageID, messageArgs);

    ASSERT_EQ(status, -EINVAL);
}

TEST(RedfishEventLog, FormatEventLogEntrySuccess)
{
    int status = 0;
    uint64_t eventId = 0;
    std::string logEntryID = "23849423_3";
    std::string messageID = "OpenBMC.0.1.PowerSupplyFanFailed";
    std::vector<std::string_view> messageArgs = {"PSU 1", "FAN 2"};
    std::string timestamp = "my-timestamp";
    std::string customText = "customText";

    nlohmann::json::object_t logEntryJson;
    status = formatEventLogEntry(eventId, logEntryID, messageID, messageArgs,
                                 timestamp, customText, logEntryJson);

    ASSERT_EQ(status, 0);

    ASSERT_TRUE(logEntryJson.contains("EventId"));
    ASSERT_EQ(logEntryJson["EventId"], "0");

    ASSERT_TRUE(logEntryJson.contains("Message"));
    ASSERT_EQ(logEntryJson["Message"], "Power supply PSU 1 fan FAN 2 failed.");

    ASSERT_TRUE(logEntryJson.contains("MessageId"));
    ASSERT_EQ(logEntryJson["MessageId"], "OpenBMC.0.5.PowerSupplyFanFailed");

    ASSERT_TRUE(logEntryJson.contains("MessageArgs"));
    ASSERT_EQ(logEntryJson["MessageArgs"].size(), 2);
    ASSERT_EQ(logEntryJson["MessageArgs"][0], "PSU 1");
    ASSERT_EQ(logEntryJson["MessageArgs"][1], "FAN 2");

    ASSERT_TRUE(logEntryJson.contains("EventTimestamp"));

    // May need to fix this, it should not pass like this.
    ASSERT_EQ(logEntryJson["EventTimestamp"], "my-timestamp");

    ASSERT_TRUE(logEntryJson.contains("Context"));
    ASSERT_EQ(logEntryJson["Context"], "customText");
}

TEST(RedfishEventLog, FormatEventLogEntryFail)
{
    int status = 0;
    uint64_t eventId = 0;
    std::string logEntryID = "malformed";
    std::string messageID;
    std::vector<std::string_view> messageArgs;
    std::string timestamp;
    std::string customText;

    nlohmann::json::object_t logEntryJson;
    status = formatEventLogEntry(eventId, logEntryID, messageID, messageArgs,
                                 timestamp, customText, logEntryJson);

    ASSERT_EQ(status, -1);
}

} // namespace
} // namespace redfish::event_log
