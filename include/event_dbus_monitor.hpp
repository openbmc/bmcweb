#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <resource_messages.hpp>
    
namespace crow
{   
namespace dbus_monitor
{
static std::shared_ptr<sdbusplus::bus::match::match> matchPlatformSAIChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchPartitionSAIChange;	
inline void saiStateChangeSignal(sdbusplus::message::message& msg)
{

    if (msg.is_method_error())
    {
        return;
    }

    boost::container::flat_map<std::string, std::variant<bool>> values;
    std::string objName;
    msg.read(objName, values);

    auto find = values.find("Asserted");
    if (find == values.end())
    {
        return;
    }

    // Push an event
    std::string origin = "/redfish/v1/Systems/system";
    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceChanged(), origin, "ComputerSystem");
}

void registerSAIStateChangeSignal()
{       

    matchPlatformSAIChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/led/groups/platform_system_attention_indicator',"
        "arg0='xyz.openbmc_project.Led.Group'",
        saiStateChangeSignal);
        

    matchPartitionSAIChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/led/groups/partition_system_attention_indicator',"
        "arg0='xyz.openbmc_project.Led.Group'",
        saiStateChangeSignal);
}
} // namespace dbus_monitor
} // namespace crow
