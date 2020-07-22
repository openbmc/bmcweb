#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <resource_messages.hpp>

namespace crow
{
namespace dbus_monitor
{
static std::shared_ptr<sdbusplus::bus::match::match> matchHostStateChange;

void registerHostStateChangeSignal()
{
    if (matchHostStateChange)
    {
        BMCWEB_LOG_DEBUG << "Host state change signal already registered";
        return;
    }

    BMCWEB_LOG_DEBUG << "Host state change signal - Register";

    matchHostStateChange = std::make_shared<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/host0',"
        "arg0='xyz.openbmc_project.State.Host'",
        [](sdbusplus::message::message& msg) {
            BMCWEB_LOG_DEBUG << "Host state change match fired";

            if (msg.is_method_error())
            {
                BMCWEB_LOG_ERROR << "Host state property changed Signal error";
                return;
            }
            std::string iface;
            boost::container::flat_map<std::string,
                                       std::variant<std::string, uint8_t>>
                values;
            std::string objName;
            msg.read(objName, values);
            auto find = values.find("CurrentHostState");
            if (find == values.end())
            {
                registerHostStateChangeSignal();
                return;
            }
            std::string* type = std::get_if<std::string>(&(find->second));
            if (type != nullptr)
            {
                BMCWEB_LOG_DEBUG << *type;
                // Push an event
                std::string origin = "/redfish/v1/Systems/system/PowerState";
                std::string eventId = "HostStateChanged";
                redfish::EventServiceManager::getInstance().sendEvent(
                    eventId, redfish::messages::ResourceChanged(), origin);
            }
            registerHostStateChangeSignal();
        });
}

void unregisterHostStateChangeSignal()
{
    if (matchHostStateChange)
    {
        BMCWEB_LOG_DEBUG << "Host state change signal - Unregister";
        matchHostStateChange.reset();
        matchHostStateChange = nullptr;
    }
}
} // namespace dbus_monitor
} // namespace crow
