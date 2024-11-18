#include "event_log.hpp"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <cstddef>
#include <ctime>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace redfish::event_log
{
namespace
{

TEST(RedfishEventLog, getUniqueEntryID_success)
{
    bool success = false;
    std::string entryID;
    std::string example = "2000-01-02T03:04:05";
    success = getUniqueEntryID(example, entryID);

    ASSERT_EQ(success, true);

    // assert the prefix since the specific number can depend on timezone
    ASSERT_TRUE(entryID.starts_with("946"));
}

TEST(RedfishEventLog, getUniqueEntryID_Unique)
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
    ASSERT_EQ(entryID2.find('_'), entryID2.rfind('_'));
}

TEST(RedfishEventLog, getUniqueEntryID_Index)
{
    std::string entryID1;
    std::string entryID2;
    std::string example = "2000-08-02T03:04:05";

    getUniqueEntryID(example, entryID1);
    getUniqueEntryID(example, entryID2);

    const size_t index = entryID2.find('_');

    ASSERT_NE(index, std::string::npos);

    std::string suffix = entryID2.substr(index + 1);

    const long n = std::stol(suffix);

    ASSERT_GE(n, 0);
}

TEST(RedfishEventLog, getEventLogParams_success)
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

TEST(RedfishEventLog, getEventLogParams_fail_no_timestamp)
{
    int status = 0;
    std::string logEntry = "3,hello";

    std::string timestamp;
    std::string messageID;
    std::vector<std::string> messageArgs;

    status = getEventLogParams(logEntry, timestamp, messageID, messageArgs);

    ASSERT_EQ(status, -EINVAL);
}

TEST(RedfishEventLog, getEventLogParams_fail_no_comma)
{
    int status = 0;
    std::string logEntry = "malformed";

    std::string timestamp;
    std::string messageID;
    std::vector<std::string> messageArgs;

    status = getEventLogParams(logEntry, timestamp, messageID, messageArgs);

    ASSERT_EQ(status, -EINVAL);
}

TEST(RedfishEventLog, formatEventLogEntry_success)
{
    int status = 0;
    std::string logEntryID = "23849423_3";
    std::string messageID = "OpenBMC.0.1.PowerSupplyFanFailed";
    std::vector<std::string_view> messageArgs = {"PSU 1", "FAN 2"};
    std::string timestamp = "my-timestamp";
    std::string customText = "customText";

    nlohmann::json::object_t logEntryJson;
    status = formatEventLogEntry(logEntryID, messageID, messageArgs, timestamp,
                                 customText, logEntryJson);

    ASSERT_EQ(status, 0);

    ASSERT_TRUE(logEntryJson.contains("EventId"));
    ASSERT_EQ(logEntryJson["EventId"], "23849423_3");

    ASSERT_TRUE(logEntryJson.contains("Message"));
    ASSERT_EQ(logEntryJson["Message"], "Power supply PSU 1 fan FAN 2 failed.");

    ASSERT_TRUE(logEntryJson.contains("MessageId"));
    ASSERT_EQ(logEntryJson["MessageId"], "OpenBMC.0.1.PowerSupplyFanFailed");

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

TEST(RedfishEventLog, formatEventLogEntry_fail)
{
    int status = 0;
    std::string logEntryID = "malformed";
    std::string messageID;
    std::vector<std::string_view> messageArgs;
    std::string timestamp;
    std::string customText;

    nlohmann::json::object_t logEntryJson;
    status = formatEventLogEntry(logEntryID, messageID, messageArgs, timestamp,
                                 customText, logEntryJson);

    ASSERT_EQ(status, -1);
}

} // namespace
} // namespace redfish::event_log
