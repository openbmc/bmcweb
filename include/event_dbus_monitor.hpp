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

void registerHostStateChangeSignal();
void registerBMCStateChangeSignal();
void registerDumpCreatedSignal();

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
            redfish::messages::ResourceChanged(), origin, "Manager");
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
            redfish::messages::ResourceChanged(), origin, "ComputerSystem");
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

inline void DumpCreatedSignal(sdbusplus::message::message& msg)
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
    std::string dumpType;

    for (auto& interface : interfacesList)
    {
        if (interface.first == "xyz.openbmc_project.Dump.Entry.BMC")
        {
            eventOrigin = "/redfish/v1/Managers/bmc/LogServices/Dump/";
            dumpType = "BMC Dump";
        }
        else if (interface.first == "xyz.openbmc_project.Dump.Entry.System")
        {
            eventOrigin = "/redfish/v1/Systems/system/LogServices/Dump/";
            dumpType = "System Dump";
        }
    }

    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceCreated(), eventOrigin, dumpType);

    registerDumpCreatedSignal();
}

void registerDumpCreatedSignal()
{
    BMCWEB_LOG_DEBUG << "Dump Created signal - Register";

    matchDumpCreatedSignal = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='InterfacesAdded',interface='org.freedesktop."
        "DBus.ObjectManager',path='/xyz/openbmc_project/dump',",
        DumpCreatedSignal);
}

} // namespace dbus_monitor
} // namespace crow
