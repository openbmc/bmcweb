#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <resource_messages.hpp>

#include <memory>
#include <string>
#include <variant>

namespace crow
{
namespace dbus_monitor
{

static std::shared_ptr<sdbusplus::bus::match::match> matchHostStateChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchBMCStateChange;
static std::shared_ptr<sdbusplus::bus::match::match>
    matchVMIIPEnabledPropChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchVMIIPChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchVMIEthIntfPropChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchDumpCreatedSignal;
static std::shared_ptr<sdbusplus::bus::match::match> matchDumpDeletedSignal;
static std::shared_ptr<sdbusplus::bus::match::match> matchBIOSAttrUpdate;
static std::shared_ptr<sdbusplus::bus::match::match> matchBootProgressChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchEventLogCreated;
static std::shared_ptr<sdbusplus::bus::match::match> matchPostCodeChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchPlatformSAIChange;
static std::shared_ptr<sdbusplus::bus::match::match> matchPartitionSAIChange;

static uint64_t postCodeCounter = 0;

void registerHostStateChangeSignal();
void registerBMCStateChangeSignal();
void registerVMIIPEnabledPropChangeSignal();
void registerVMIIPChangeSignal();
void registerVMIEthIntfPropChangeSignal();
void registerDumpCreatedSignal();
void registerDumpDeletedSignal();
void registerBIOSAttrUpdateSignal();
void registerBootProgressChangeSignal();
void registerEventLogCreatedSignal();
void registerPostCodeChangeSignal();
void registerSAIStateChangeSignal();

inline void sendEventOnEthIntf(const std::string& origin)
{
    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceChanged(), origin, "EthernetInterface");
}

inline void vmiNwPropertyChange(sdbusplus::message::message& msg)
{
    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR(
            "BMC Hypervisor Network properties changed Signal error");
        return;
    }

    std::string objPath = msg.get_path();

    if (!(objPath.starts_with(
            "/xyz/openbmc_project/network/hypervisor/eth0")) &&
        !(objPath.starts_with("/xyz/openbmc_project/network/hypervisor/eth1")))
    {
        return;
    }

    std::string infName;
    if (objPath.find("/eth0") != std::string::npos)
    {
        infName = "eth0";
    }
    else if (objPath.find("/eth1") != std::string::npos)
    {
        infName = "eth1";
    }
    else
    {
        BMCWEB_LOG_ERROR("Unsupported Interface name");
        return;
    }

    std::string origin = std::format(
        "/redfish/v1/Systems/hypervisor/EthernetInterfaces/{}", infName);
    BMCWEB_LOG_DEBUG("Pushing the VMI Nw property change event for IP "
                     "property with origin: {}",
                     origin);
    sendEventOnEthIntf(origin);
}

inline void bmcStatePropertyChange(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("BMC state change match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("BMC state property changed Signal error");
        return;
    }
    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);
    auto find = values.find("CurrentBMCState");
    if (find == values.end())
    {
        return;
    }
    std::string* type = std::get_if<std::string>(&(find->second));
    if (type != nullptr)
    {
        // Push an event
        std::string origin = "/redfish/v1/Managers/bmc";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "Manager");
    }
}

inline void hostStatePropertyChange(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("Host state change match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("Host state property changed Signal error");
        return;
    }
    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);
    auto find = values.find("CurrentHostState");
    if (find == values.end())
    {
        return;
    }
    std::string* type = std::get_if<std::string>(&(find->second));
    if (type != nullptr)
    {
        if (*type == "xyz.openbmc_project.State.Host.HostState.Off")
        {
            // reset the postCodeCounter
            postCodeCounter = 0;
            BMCWEB_LOG_CRITICAL(
                "INFO: Host is powered off. Reset the postcode counter to: {}",
                postCodeCounter);
        }
        // Push an event
        std::string origin = "/redfish/v1/Systems/system";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "ComputerSystem");
    }
}

inline void bootProgressPropertyChange(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("BootProgress change match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("BootProgress property changed Signal error");
        return;
    }
    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);
    auto find = values.find("BootProgress");
    if (find == values.end())
    {
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
}

inline void postCodePropertyChange(sdbusplus::message::message& msg)
{
    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("PostCode property changed Signal error");
        return;
    }
    std::string postcodeEntryID =
        std::format("B1-{}", std::to_string(++postCodeCounter));
    BMCWEB_LOG_DEBUG("Current post code: {}", postcodeEntryID);
    // Push an event
    std::string eventOrigin = std::format(
        "/redfish/v1/Systems/system/LogServices/PostCodes/Entries/{}",
        postcodeEntryID);
    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceCreated(), eventOrigin, "ComputerSystem");
}

inline void saiStateChangeSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("System Attention Indicator State change match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("SAI State change signal error");
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

void registerHostStateChangeSignal()
{
    BMCWEB_LOG_DEBUG("Host state change signal - Register");

    matchHostStateChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/host0',"
        "arg0='xyz.openbmc_project.State.Host'",
        hostStatePropertyChange);
}

void registerBMCStateChangeSignal()
{
    BMCWEB_LOG_DEBUG("BMC state change signal - Register");

    matchBMCStateChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/bmc0',"
        "arg0='xyz.openbmc_project.State.BMC'",
        bmcStatePropertyChange);
}

void registerVMIIPEnabledPropChangeSignal()
{
    BMCWEB_LOG_DEBUG("VMI IP change signal match - Registered");

    matchVMIIPEnabledPropChange = std::make_unique<
        sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0namespace='xyz.openbmc_project.Object.Enable'",
        vmiNwPropertyChange);
}

void registerVMIIPChangeSignal()
{
    matchVMIIPChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0namespace='xyz.openbmc_project.Network.IP'",
        vmiNwPropertyChange);
}

void registerVMIEthIntfPropChangeSignal()
{
    matchVMIEthIntfPropChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0namespace='xyz.openbmc_project.Network."
        "EthernetInterface'",
        vmiNwPropertyChange);
}

void registerBootProgressChangeSignal()
{
    BMCWEB_LOG_DEBUG("BootProgress change signal - Register");

    matchBootProgressChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/host0',"
        "arg0='xyz.openbmc_project.State.Boot.Progress'",
        bootProgressPropertyChange);
}

static void eventLogCreatedSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("Event Log Created - match fired");

    constexpr auto pelEntryInterface = "org.open_power.Logging.PEL.Entry";

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("Event Log Created signal error");
        return;
    }

    sdbusplus::message::object_path objPath;
    std::map<std::string,
             std::map<std::string, std::variant<std::string, bool>>>
        interfaces;

    msg.read(objPath, interfaces);

    std::string logID;
    dbus::utility::getNthStringFromPath(objPath, 4, logID);

    const auto pelProperties = interfaces.find(pelEntryInterface);
    if (pelProperties == interfaces.end())
    {
        return;
    }

    const auto hiddenProperty = pelProperties->second.find("Hidden");
    if (hiddenProperty == pelProperties->second.end())
    {
        return;
    }

    const bool* hiddenPropertyPtr =
        std::get_if<bool>(&(hiddenProperty->second));
    if (hiddenPropertyPtr == nullptr)
    {
        BMCWEB_LOG_ERROR("Failed to get Hidden property");
        return;
    }

    std::string eventOrigin;
    if (*hiddenPropertyPtr)
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/CELog/Entries/{}", logID);
        BMCWEB_LOG_DEBUG("CELog path: {}", eventOrigin);
    }
    else
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/EventLog/Entries/{}",
            logID);
        BMCWEB_LOG_DEBUG("EventLog path: {}", eventOrigin);
    }

    BMCWEB_LOG_DEBUG("Sending event for log ID: {} ", logID);
    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceCreated(), eventOrigin, "LogEntry");
}

void registerEventLogCreatedSignal()
{
    BMCWEB_LOG_DEBUG("Register EventLog Created Signal");
    matchEventLogCreated = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='InterfacesAdded',interface='org.freedesktop."
        "DBus.ObjectManager',path='/xyz/openbmc_project/logging',",
        eventLogCreatedSignal);
}

static void registerStateChangeSignal()
{
    registerHostStateChangeSignal();
    registerBMCStateChangeSignal();
    registerBootProgressChangeSignal();
}

static void registerVMIConfigChangeSignal()
{
    registerVMIIPEnabledPropChangeSignal();
    registerVMIIPChangeSignal();
    registerVMIEthIntfPropChangeSignal();
}

void registerPostCodeChangeSignal()
{
    BMCWEB_LOG_DEBUG("PostCode change signal - Register");

    matchPostCodeChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/state/boot/raw0',"
        "arg0='xyz.openbmc_project.State.Boot.Raw'",
        postCodePropertyChange);
}

inline void dumpCreatedSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("Dump Created - match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("Dump Created signal error");
        return;
    }

    std::string dumpType;
    std::string dumpId;

    dbus::utility::getNthStringFromPath(msg.get_path(), 3, dumpType);
    dbus::utility::getNthStringFromPath(msg.get_path(), 5, dumpId);

    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);

    auto find = values.find("Status");
    if (find == values.end())
    {
        BMCWEB_LOG_DEBUG("Status property not found. Continuing to listen...");
        return;
    }
    std::string* propValue = std::get_if<std::string>(&(find->second));

    if (propValue != nullptr &&
        *propValue ==
            "xyz.openbmc_project.Common.Progress.OperationStatus.Completed")
    {
        BMCWEB_LOG_DEBUG << "Sending event\n";

        std::string eventOrigin;
        // Push an event
        if (dumpType == "bmc")
        {
            eventOrigin = std::format(
                "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/{}", dumpId);
        }
        else if (dumpType == "system")
        {
            eventOrigin = std::format(
                "/redfish/v1/Systems/system/LogServices/Dump/Entries/System_{}",
                dumpId);
        }
        else if (dumpType == "resource")
        {
            eventOrigin = std::format(
                "/redfish/v1/Systems/system/LogServices/Dump/Entries/Resource_{}",
                dumpId);
        }
        else if (dumpType == "hostboot")
        {
            eventOrigin = std::format(
                "/redfish/v1/Systems/system/LogServices/Dump/Entries/Hostboot_{}",
                dumpId);
        }
        else if (dumpType == "hardware")
        {
            eventOrigin = std::format(
                "/redfish/v1/Systems/system/LogServices/Dump/Entries/Hardware_{}",
                dumpId);
        }
        else if (dumpType == "sbe")
        {
            eventOrigin = std::format(
                "/redfish/v1/Systems/system/LogServices/Dump/Entries/SBE_{}",
                dumpId);
        }
        else
        {
            BMCWEB_LOG_ERROR("Invalid dump type received when listening for "
                             "dump created signal");
            return;
        }
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceCreated(), eventOrigin, "LogEntry");
    }
}

inline void dumpDeletedSignal(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("Dump Deleted - match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("Dump Deleted signal error");
        return;
    }

    std::vector<std::string> interfacesList;
    sdbusplus::message::object_path objPath;

    msg.read(objPath, interfacesList);

    std::string dumpType;
    std::string dumpId;

    dbus::utility::getNthStringFromPath(objPath, 3, dumpType);
    dbus::utility::getNthStringFromPath(objPath, 5, dumpId);

    std::string eventOrigin;

    if (dumpType == "bmc")
    {
        eventOrigin = std::format(
            "/redfish/v1/Managers/bmc/LogServices/Dump/Entries/{}", dumpId);
    }
    else if (dumpType == "system")
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/Dump/Entries/System_{}",
            dumpId);
    }
    else if (dumpType == "resource")
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/Dump/Entries/Resource_{}",
            dumpId);
    }
    else if (dumpType == "hostboot")
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/Dump/Entries/Hostboot_{}",
            dumpId);
    }
    else if (dumpType == "hardware")
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/Dump/Entries/Hardware_{}",
            dumpId);
    }
    else if (dumpType == "sbe")
    {
        eventOrigin = std::format(
            "/redfish/v1/Systems/system/LogServices/Dump/Entries/SBE_{}",
            dumpId);
    }
    else
    {
        BMCWEB_LOG_ERROR("Invalid dump type received when listening for "
                         "dump deleted signal");
        return;
    }

    redfish::EventServiceManager::getInstance().sendEvent(
        redfish::messages::resourceRemoved(), eventOrigin, "LogEntry");
}

void registerDumpCreatedSignal()
{
    BMCWEB_LOG_DEBUG("Dump Created signal - Register");

    matchDumpCreatedSignal = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0namespace='xyz.openbmc_project.Common.Progress',",
        dumpCreatedSignal);
}

void registerDumpDeletedSignal()
{
    BMCWEB_LOG_DEBUG("Dump Deleted signal - Register");

    matchDumpDeletedSignal = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='InterfacesRemoved',interface='org.freedesktop."
        "DBus.ObjectManager',path='/xyz/openbmc_project/dump',",
        dumpDeletedSignal);
}

static void registerDumpUpdateSignal()
{
    registerDumpCreatedSignal();
    registerDumpDeletedSignal();
}

inline void biosAttrUpdate(sdbusplus::message::message& msg)
{
    BMCWEB_LOG_DEBUG("BIOS attribute change match fired");

    if (msg.is_method_error())
    {
        BMCWEB_LOG_ERROR("BIOS attribute changed Signal error");
        return;
    }

    boost::container::flat_map<std::string, std::variant<std::string, uint8_t>>
        values;
    std::string objName;
    msg.read(objName, values);

    auto find = values.find("BaseBIOSTable");
    if (find == values.end())
    {
        BMCWEB_LOG_DEBUG(
            "BaseBIOSTable property not found. Continuing to listen...");
        return;
    }
    std::string* type = std::get_if<std::string>(&(find->second));
    if (type != nullptr)
    {
        BMCWEB_LOG_DEBUG("Sending event\n");
        // Push an event
        std::string origin = "/redfish/v1/Systems/system/Bios";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "Bios");
    }
}

void registerBIOSAttrUpdateSignal()
{
    BMCWEB_LOG_DEBUG("BIOS Attribute update signal match - Registered");

    matchBIOSAttrUpdate = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',arg0namespace='xyz.openbmc_project.BIOSConfig."
        "Manager'",
        biosAttrUpdate);
}

void registerSAIStateChangeSignal()
{
    BMCWEB_LOG_DEBUG(
        "Platform System attention Indicator state change signal - Register");

    matchPlatformSAIChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/led/groups/platform_system_attention_indicator',"
        "arg0='xyz.openbmc_project.Led.Group'",
        saiStateChangeSignal);

    BMCWEB_LOG_DEBUG(
        "Partition System attention Indicator state change signal - Register");

    matchPartitionSAIChange = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "type='signal',member='PropertiesChanged',interface='org.freedesktop."
        "DBus.Properties',path='/xyz/openbmc_project/led/groups/partition_system_attention_indicator',"
        "arg0='xyz.openbmc_project.Led.Group'",
        saiStateChangeSignal);
}
} // namespace dbus_monitor
} // namespace crow
