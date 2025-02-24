#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish-core/lib/log_services.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

namespace redfish
{
using namespace dbus::utility;
TEST(LogServices, fillEventLogLogEntryFromPropertyMapSuccess)
{
    if constexpr (BMCWEB_REDFISH_ALLOW_DBUS_MESSAGES_MAPPING)
    {
        const DBusPropertiesMap propMapStub = {
            {"AdditionalData",
             DbusVariantType(std::unordered_map<std::string, std::string>{
                 {"READING_VALUE", "10.2"},
                 {"SENSOR_NAME", "BIC_JI_SENSOR_MB_RETIMER_TEMP_C"},
                 {"THRESHOLD_VALUE", "15.5"},
                 {"UNITS", "xyz.openbmc_project.Sensor.Value.Unit.DegreesC"}})},
            {"EventId", DbusVariantType("")},
            {"Id", DbusVariantType(static_cast<uint32_t>(1838))},

            // use 'Message' for MessageId as per the design
            // https://github.com/openbmc/docs/blob/d886ce89fe66c128b3ab492e530ad48fa0c1b4eb/designs/event-logging.md?plain=1#L448
            {"Message",
             DbusVariantType(
                 "xyz.openbmc_project.Sensor.Threshold.ReadingBelowLowerCriticalThreshold")},
            {"Resolution", DbusVariantType("")},
            {"Resolved", DbusVariantType(true)},
            {"ServiceProviderNotify", DbusVariantType("")},
            {"Severity", DbusVariantType("")},
            {"Timestamp",
             DbusVariantType(static_cast<uint64_t>(1638312095123))},
            {"UpdateTimestamp",
             DbusVariantType(static_cast<uint64_t>(1638312099123))},
        };

        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        nlohmann::json objectToFillOut;

        fillEventLogLogEntryFromPropertyMap(asyncResp, propMapStub,
                                            objectToFillOut);

        auto logEntryID = std::to_string(1838);
        EXPECT_EQ(objectToFillOut["@odata.type"], "#LogEntry.v1_9_0.LogEntry");
        EXPECT_EQ(
            objectToFillOut["@odata.id"],
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries/1838");
        EXPECT_EQ(objectToFillOut["Name"], "System Event Log Entry");
        EXPECT_EQ(objectToFillOut["Id"], logEntryID.c_str());
        EXPECT_EQ(objectToFillOut["EntryType"], "Event");
        EXPECT_TRUE(objectToFillOut["Resolved"]);
        EXPECT_EQ(objectToFillOut["Created"], "2021-11-30T22:41:35.123+00:00");
        EXPECT_EQ(objectToFillOut["Modified"], "2021-11-30T22:41:39.123+00:00");
        EXPECT_FALSE(objectToFillOut.contains("AdditionalDataURI"));

        EXPECT_EQ(objectToFillOut["MessageId"],
                  "SensorEvent.1.0.ReadingBelowLowerCriticalThreshold");
        ASSERT_EQ(objectToFillOut["MessageArgs"].size(), 4);
        // SENSOR_NAME
        EXPECT_EQ(objectToFillOut["MessageArgs"][0],
                  "BIC_JI_SENSOR_MB_RETIMER_TEMP_C");
        // READING_VALUE
        EXPECT_EQ(objectToFillOut["MessageArgs"][1], "10.2");
        // UNITS
        EXPECT_EQ(objectToFillOut["MessageArgs"][2],
                  "xyz.openbmc_project.Sensor.Value.Unit.DegreesC");
        // THRESHOLD_VALUE
        EXPECT_EQ(objectToFillOut["MessageArgs"][3], "15.5");
        EXPECT_EQ(objectToFillOut["Severity"], "Critical");
        EXPECT_EQ(
            objectToFillOut["Message"],
            "Sensor 'BIC_JI_SENSOR_MB_RETIMER_TEMP_C' reading of 10.2 (xyz.openbmc_project.Sensor.Value.Unit.DegreesC) is below the 15.5 lower critical threshold.");
        EXPECT_EQ(
            objectToFillOut["Resolution"],
            "Check the condition of the resources listed in RelatedItem.");
    }
}

TEST(LogServices, fillEventLogLogEntryFromPropertyMapWOMappingSuccess)
{
    if constexpr (!BMCWEB_REDFISH_ALLOW_DBUS_MESSAGES_MAPPING)
    {
        const DBusPropertiesMap propMapStub = {
            {"AdditionalData",
             DbusVariantType(std::unordered_map<std::string, std::string>{})},
            {"EventId", DbusVariantType("")},
            {"Id", DbusVariantType(static_cast<uint32_t>(1838))},
            {"Message",
             DbusVariantType("xyz.openbmc_project.Something.Is.Wrong")},
            {"Resolution", DbusVariantType("Resolution instructions")},
            {"Resolved", DbusVariantType(false)},
            {"ServiceProviderNotify", DbusVariantType("")},
            {"Severity",
             DbusVariantType(
                 "xyz.openbmc_project.Logging.Entry.Level.Warning")},
            {"Timestamp",
             DbusVariantType(static_cast<uint64_t>(1638312095123))},
            {"UpdateTimestamp",
             DbusVariantType(static_cast<uint64_t>(1638312099123))},
        };

        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        nlohmann::json objectToFillOut;

        fillEventLogLogEntryFromPropertyMap(asyncResp, propMapStub,
                                            objectToFillOut);

        auto logEntryID = std::to_string(1838);
        EXPECT_EQ(objectToFillOut["@odata.type"], "#LogEntry.v1_9_0.LogEntry");
        EXPECT_EQ(
            objectToFillOut["@odata.id"],
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries/1838");
        EXPECT_EQ(objectToFillOut["Name"], "System Event Log Entry");
        EXPECT_EQ(objectToFillOut["Id"], logEntryID.c_str());
        EXPECT_EQ(objectToFillOut["EntryType"], "Event");
        EXPECT_FALSE(objectToFillOut["Resolved"]);
        EXPECT_FALSE(objectToFillOut.contains("ServiceProviderNotified"));
        EXPECT_EQ(objectToFillOut["Created"], "2021-11-30T22:41:35.123+00:00");
        EXPECT_EQ(objectToFillOut["Modified"], "2021-11-30T22:41:39.123+00:00");
        EXPECT_FALSE(objectToFillOut.contains("AdditionalDataURI"));

        EXPECT_FALSE(objectToFillOut.contains("MessageId"));
        EXPECT_FALSE(objectToFillOut.contains("MessageArgs"));
        EXPECT_EQ(objectToFillOut["Severity"], "Warning");
        EXPECT_EQ(objectToFillOut["Message"],
                  "xyz.openbmc_project.Something.Is.Wrong");
        EXPECT_EQ(objectToFillOut["Resolution"], "Resolution instructions");
    }
}
} // namespace redfish
