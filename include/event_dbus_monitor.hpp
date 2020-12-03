#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <resource_messages.hpp>

namespace crow
{
namespace dbus_monitor
{

static std::shared_ptr<sdbusplus::bus::match::match> matchHostStateChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchBMCStateChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchDumpCreatedSignal;
static std::shared_ptr<sdbusplus::bus::match::match> matchDumpDeletedSignal;

void registerHostStateChangeSignal();
void registerBMCStateChangeSignal();
void registerDumpCreatedSignal();
void registerDumpDeletedSignal();

inline void BMCStatePropertyChange(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG << "BMC state change match fired";

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR << "BMC state property changed Signal error";
        return;
    }
    std::string iface;
    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);
    auto find = values.find("CurrentBMCState");
    if (find == values.end())
    {
        registerBMCStateChangeSignal();
        return;
    }
    std::string* type = std::get_if<std::string>(&(find->second));
    if (type != nullptr)
    {
        BMCWEB_LOG_DEBUG << *type;
        // Push an event
        std::string origin = "/redfish/v1/Managers/bmc";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "Manager");
    }
    registerBMCStateChangeSignal();
}

inline void HostStatePropertyChange(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG << "Host state change match fired";

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR << "Host state property changed Signal error";
        return;
    }
    std::string iface;
    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
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
        std::string origin = "/redfish/v1/Systems/system";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "ComputerSystem");
    }
    registerHostStateChangeSignal();
}

void registerHostStateChangeSignal()
{
    BMCWEB_LOG_DEBUG << "Host state change signal - Register";

    matchHostStateChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/host0',"
        "arg0='xyz.openbmc_project.State.Host'",
        HostStatePropertyChange);
}

void registerBMCStateChangeSignal()
{

    BMCWEB_LOG_DEBUG << "BMC state change signal - Register";

    matchBMCStateChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/bmc0',"
        "arg0='xyz.openbmc_project.State.BMC'",
        BMCStatePropertyChange);
}

void registerStateChangeSignal()
{
    registerHostStateChangeSignal();
    registerBMCStateChangeSignal();
}

inline void dumpCreatedSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG << "Dump Created - match fired";

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR << "Dump Created signal error";
        return;
    }

    std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, std::variant<std::string>>>>>
        interfacesList;

    sdbusplus::message::object_path objPath;

    msg.read(objPath, interfacesList);

    std::string eventOrigin;

    if ((objPath.str).find("/bmc/") != std::string::npos)
    {
        eventOrigin = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if ((objPath.str).find("/system/") != std::string::npos)
    {
        eventOrigin = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }

    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceCreated(), eventOrigin, "LogEntry");

    registerDumpCreatedSignal();
}

inline void dumpDeletedSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG << "Dump Deleted - match fired";

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR << "Dump Deleted signal error";
        return;
    }

    std::vector<std::string> interfacesList;

    sdbusplus::message::object_path objPath;

    msg.read(objPath, interfacesList);

    std::string eventOrigin;

    if ((objPath.str).find("/bmc/") != std::string::npos)
    {
        eventOrigin = "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/";
    }
    else if ((objPath.str).find("/system/") != std::string::npos)
    {
        eventOrigin = "/redfish/v1/Systems/system/LogServices/Dump/Entries/";
    }

    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceRemoved(), eventOrigin, "LogEntry");

    registerDumpDeletedSignal();
}

void registerDumpCreatedSignal()
{
    BMCWEB_LOG_DEBUG << "Dump Created signal - Register";

    matchDumpCreatedSignal = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='InterfacesAdded',interface='org.freedesktop."
        "DBus.ObjectManager',path='/xyz/openbmc_project/dump',",
        dumpCreatedSignal);
}

void registerDumpDeletedSignal()
{
    BMCWEB_LOG_DEBUG << "Dump Deleted signal - Register";

    matchDumpDeletedSignal = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='InterfacesRemoved',interface='org.freedesktop."
        "DBus.ObjectManager',path='/xyz/openbmc_project/dump',",
        dumpDeletedSignal);
}

void registerDumpUpdateSignal()
{
    registerDumpCreatedSignal();
    registerDumpDeletedSignal();
}

} // namespace dbus_monitor
} // namespace crow
