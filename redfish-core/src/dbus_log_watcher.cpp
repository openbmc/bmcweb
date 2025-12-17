#include "dbus_log_watcher.hpp"

#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "event_logs_object_type.hpp"
#include "event_service_manager.hpp"
#include "logging.hpp"
#include "telemetry_readings.hpp"
#include "utils/dbus_event_log_entry.hpp"
#include "utils/time_utils.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace redfish
{

bool DbusEventLogMonitor::eventLogObjectFromDBus(
    const dbus::utility::DBusPropertiesMap& map, EventLogObjectsType& event)
{
    std::optional<DbusEventLogEntry> optEntry =
        fillDbusEventLogEntryFromPropertyMap(map);

    if (!optEntry.has_value())
    {
        BMCWEB_LOG_ERROR(
            "Could not construct event log entry from dbus properties");
        return false;
    }
    DbusEventLogEntry& entry = optEntry.value();
    event.id = std::to_string(entry.Id);
    event.timestamp = redfish::time_utils::getDateTimeUintMs(entry.Timestamp);

    // This dbus property is not documented to contain the Redfish Message Id,
    // but can be used as such. As a temporary solution that is sufficient,
    // the event filtering code will drop the event anyways if event.messageId
    // is not valid.
    //
    // This will need resolved before
    // experimental-redfish-dbus-log-subscription is stabilized
    event.messageId = entry.Message;

    // The order of 'AdditionalData' is not what's specified in an e.g.
    // busctl call to create the Event Log Entry. So it cannot be used
    // to map to the message args. Leaving this branch here for it to be
    // implemented when the mapping is available

    return true;
}

static void dbusEventLogMatchHandlerSingleEntry(
    const dbus::utility::DBusPropertiesMap& map)
{
    std::vector<EventLogObjectsType> eventRecords;
    EventLogObjectsType& event = eventRecords.emplace_back();
    bool success = DbusEventLogMonitor::eventLogObjectFromDBus(map, event);
    if (!success)
    {
        BMCWEB_LOG_ERROR("Could not parse event log entry from dbus");
        return;
    }

    BMCWEB_LOG_DEBUG("Found Event Log Entry Id={}, Timestamp={}, Message={}",
                     event.id, event.timestamp, event.messageId);
    EventServiceManager::sendEventsToSubs(eventRecords);
}

static void onDbusEventLogCreated(sdbusplus::message_t& msg)
{
    BMCWEB_LOG_DEBUG("Handling new DBus Event Log Entry");

    sdbusplus::message::object_path objectPath;
    dbus::utility::DBusInterfacesMap interfaces;

    msg.read(objectPath, interfaces);

    for (auto& pair : interfaces)
    {
        BMCWEB_LOG_DEBUG("Found dbus interface {}", pair.first);
        if (pair.first == "xyz.openbmc_project.Logging.Entry")
        {
            const dbus::utility::DBusPropertiesMap& map = pair.second;
            dbusEventLogMatchHandlerSingleEntry(map);
        }
    }
}

const std::string propertiesMatchString =
    sdbusplus::bus::match::rules::type::signal() +
    sdbusplus::bus::match::rules::sender("xyz.openbmc_project.Logging") +
    sdbusplus::bus::match::rules::interface(
        "org.freedesktop.DBus.ObjectManager") +
    sdbusplus::bus::match::rules::path("/xyz/openbmc_project/logging") +
    sdbusplus::bus::match::rules::member("InterfacesAdded");

DbusEventLogMonitor::DbusEventLogMonitor() :
    dbusEventLogMonitor(*crow::connections::systemBus, propertiesMatchString,
                        onDbusEventLogCreated)

{}

static void getReadingsForReport(sdbusplus::message_t& msg)
{
    sdbusplus::message::object_path path(msg.get_path());
    std::string id = path.filename();
    if (id.empty())
    {
        BMCWEB_LOG_ERROR("Failed to get Id from path");
        return;
    }

    std::string interface;
    dbus::utility::DBusPropertiesMap props;
    std::vector<std::string> invalidProps;
    msg.read(interface, props, invalidProps);

    auto found = std::ranges::find_if(props, [](const auto& x) {
        return x.first == "Readings";
    });
    if (found == props.end())
    {
        BMCWEB_LOG_INFO("Failed to get Readings from Report properties");
        return;
    }

    const telemetry::TimestampReadings* readings =
        std::get_if<telemetry::TimestampReadings>(&found->second);
    if (readings == nullptr)
    {
        BMCWEB_LOG_INFO("Failed to get Readings from Report properties");
        return;
    }
    EventServiceManager::sendTelemetryReportToSubs(id, *readings);
}

const std::string telemetryMatchStr =
    "type='signal',member='PropertiesChanged',"
    "interface='org.freedesktop.DBus.Properties',"
    "arg0=xyz.openbmc_project.Telemetry.Report";

DbusTelemetryMonitor::DbusTelemetryMonitor() :
    matchTelemetryMonitor(*crow::connections::systemBus, telemetryMatchStr,
                          getReadingsForReport)
{}
} // namespace redfish
