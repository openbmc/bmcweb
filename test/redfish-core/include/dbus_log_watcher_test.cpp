#include "dbus_log_watcher.hpp"
#include "dbus_utility.hpp"
#include "event_logs_object_type.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

namespace redfish
{

using namespace dbus::utility;

TEST(DBusLogWatcher, EventLogObjectFromDBusWithMappingSuccess)
{
    const DBusPropertiesMap propMapStub = {
        {"AdditionalData",
         DbusVariantType(std::unordered_map<std::string, std::string>{
             {"READING_VALUE", "10.2"},
             {"SENSOR_NAME", "BIC_JI_SENSOR_MB_RETIMER_TEMP_C"},
             {"THRESHOLD_VALUE", "15.5"},
             {"UNITS", "xyz.openbmc_project.Sensor.Value.Unit.DegreesC"},
             {"_CODE_FILE",
              "/usr/src/debug/phosphor-logging/1.0+git/log_create_main.cpp"},
             {"_CODE_FUNC",
              "int generate_event(const std::string&, const nlohmann::json_abi_v3_11_3::json&)"},
             {"_CODE_LINE", "34"},
             {"_PID", "18332"}})},
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
        {"Timestamp", DbusVariantType(static_cast<uint64_t>(1638312095123))},
        {"UpdateTimestamp", DbusVariantType(static_cast<uint64_t>(3899))},
    };

    EventLogObjectsType event;

    const bool status =
        DbusEventLogMonitor::eventLogObjectFromDBus(propMapStub, event);

    EXPECT_TRUE(status);

    EXPECT_EQ(event.id, "1838");

    EXPECT_EQ(event.timestamp, "2021-11-30T22:41:35.123+00:00");

    EXPECT_EQ(event.messageId,
              "SensorEvent.0.0.ReadingBelowLowerCriticalThreshold");

    EXPECT_EQ(event.messageArgs.size(), 4);
    // SENSOR_NAME
    EXPECT_EQ(event.messageArgs[0], "BIC_JI_SENSOR_MB_RETIMER_TEMP_C");
    // READING_VALUE
    EXPECT_EQ(event.messageArgs[1], "10.2");
    // UNITS
    EXPECT_EQ(event.messageArgs[2],
              "xyz.openbmc_project.Sensor.Value.Unit.DegreesC");
    // THRESHOLD_VALUE
    EXPECT_EQ(event.messageArgs[3], "15.5");
}

TEST(DBusLogWatcher, EventLogObjectFromDBusFailMissingProperty)
{
    // missing 'Resolved'
    const DBusPropertiesMap propMapWrong = {
        {"AdditionalData",
         DbusVariantType(
             std::unordered_map<std::string, std::string>{{"KEY", "VALUE"}})},
        {"EventId", DbusVariantType("")},
        {"Id", DbusVariantType(static_cast<uint32_t>(1838))},
        {"Message", DbusVariantType("")},
        {"Resolution", DbusVariantType("")},
        {"ServiceProviderNotify", DbusVariantType("")},
        {"Severity", DbusVariantType("")},
        {"Timestamp", DbusVariantType(static_cast<uint64_t>(3832))},
        {"UpdateTimestamp", DbusVariantType(static_cast<uint64_t>(3899))},
    };

    EventLogObjectsType event;

    const bool status =
        DbusEventLogMonitor::eventLogObjectFromDBus(propMapWrong, event);

    EXPECT_FALSE(status);
}

} // namespace redfish
