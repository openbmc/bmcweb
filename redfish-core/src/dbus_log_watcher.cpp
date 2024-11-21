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
