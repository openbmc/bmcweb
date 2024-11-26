#include "dbus_log_watcher.hpp"

#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "event_logs_object_type.hpp"
#include "event_service_manager.hpp"
#include "logging.hpp"
#include "metric_report.hpp"
#include "utils/dbus_event_log_entry.hpp"

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
static bool eventLogObjectFromDBus(const dbus::utility::DBusPropertiesMap& map,
                                   EventLogObjectsType& event)
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
    bool success = eventLogObjectFromDBus(map, event);
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
    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("TelemetryMonitor Signal error");
        return;
    }

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
