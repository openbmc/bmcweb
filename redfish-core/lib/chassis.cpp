#include "../../http/logging.hpp"
#include "../../include/async_resp_class_decl.hpp"
#include "../../include/dbus_singleton.hpp"
#include "../include/error_messages.hpp"
#include "../include/registries/privilege_registry.hpp"
#include "../include/utils/collection.hpp"
#include "../include/utils/json_utils.hpp"
#include "health_class_decl.hpp"
#include "chassis_class_decl.hpp"
#include "app_class_decl.hpp"
#include "led.hpp"
#include <charconv>

#include "pcie.hpp"
#include "processor.hpp"
#include "roles.hpp"
#include "service_root.hpp"
#include "storage.hpp"

using VariantType = std::variant<bool, std::string, uint64_t, uint32_t>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;

using PropertiesType = boost::container::flat_map<std::string, VariantType>;

namespace redfish
{

inline void getChassisState(std::shared_ptr<bmcweb::AsyncResp> aResp) {
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::variant<std::string>& chassisState) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                messages::internalError(aResp->res);
                return;
            }

            const std::string* s = std::get_if<std::string>(&chassisState);
            BMCWEB_LOG_DEBUG << "Chassis state: " << *s;
            if (s != nullptr)
            {
                // Verify Chassis State
                if (*s == "xyz.openbmc_project.State.Chassis.PowerState.On")
                {
                    aResp->res.jsonValue["PowerState"] = "On";
                    aResp->res.jsonValue["Status"]["State"] = "Enabled";
                }
                else if (*s ==
                         "xyz.openbmc_project.State.Chassis.PowerState.Off")
                {
                    aResp->res.jsonValue["PowerState"] = "Off";
                    aResp->res.jsonValue["Status"]["State"] = "StandbyOffline";
                }
            }
        },
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");
}


void requestRoutesChassisCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/")
        .privileges(redfish::privileges::getChassisCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ChassisCollection.ChassisCollection";
                asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Chassis";
                asyncResp->res.jsonValue["Name"] = "Chassis Collection";

                collection_util::getCollectionMembers(
                    asyncResp, "/redfish/v1/Chassis",
                    {"xyz.openbmc_project.Inventory.Item.Board",
                     "xyz.openbmc_project.Inventory.Item.Chassis"});
            });
}

void requestRoutesChassis(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::getChassis)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& chassisId) {
            const std::array<const char*, 2> interfaces = {
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"};

            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId(std::string(chassisId))](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    // Iterate over all retrieved ObjectPaths.
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<std::string,
                                                   std::vector<std::string>>>>&
                             object : subtree)
                    {
                        const std::string& path = object.first;
                        const std::vector<
                            std::pair<std::string, std::vector<std::string>>>&
                            connectionNames = object.second;

                        if (!boost::ends_with(path, chassisId))
                        {
                            continue;
                        }

                        auto health =
                            std::make_shared<HealthPopulate>(asyncResp);

                        crow::connections::systemBus->async_method_call(
                            [health](
                                const boost::system::error_code ec2,
                                std::variant<std::vector<std::string>>& resp) {
                                if (ec2)
                                {
                                    return; // no sensors = no failures
                                }
                                std::vector<std::string>* data =
                                    std::get_if<std::vector<std::string>>(
                                        &resp);
                                if (data == nullptr)
                                {
                                    return;
                                }
                                health->inventory = std::move(*data);
                            },
                            "xyz.openbmc_project.ObjectMapper",
                            path + "/all_sensors",
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Association", "endpoints");

                        health->populate();

                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#Chassis.v1_14_0.Chassis";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Chassis/" + chassisId;
                        asyncResp->res.jsonValue["Name"] = "Chassis Collection";
                        asyncResp->res.jsonValue["ChassisType"] = "RackMount";
                        asyncResp->res.jsonValue["Actions"]["#Chassis.Reset"] =
                            {{"target", "/redfish/v1/Chassis/" + chassisId +
                                            "/Actions/Chassis.Reset"},
                             {"@Redfish.ActionInfo", "/redfish/v1/Chassis/" +
                                                         chassisId +
                                                         "/ResetActionInfo"}};
                        asyncResp->res.jsonValue["PCIeDevices"] = {
                            {"@odata.id",
                             "/redfish/v1/Systems/system/PCIeDevices"}};

                        const std::string& connectionName =
                            connectionNames[0].first;

                        const std::vector<std::string>& interfaces2 =
                            connectionNames[0].second;
                        const std::array<const char*, 2> hasIndicatorLed = {
                            "xyz.openbmc_project.Inventory.Item.Panel",
                            "xyz.openbmc_project.Inventory.Item.Board."
                            "Motherboard"};

                        const std::string assetTagInterface =
                            "xyz.openbmc_project.Inventory.Decorator."
                            "AssetTag";
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      assetTagInterface) != interfaces2.end())
                        {
                            crow::connections::systemBus->async_method_call(
                                [asyncResp, chassisId(std::string(chassisId))](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBus response error for "
                                               "AssetTag";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }

                                    const std::string* assetTag =
                                        std::get_if<std::string>(&property);
                                    if (assetTag == nullptr)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Null value returned "
                                               "for Chassis AssetTag";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res.jsonValue["AssetTag"] =
                                        *assetTag;
                                },
                                connectionName, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                assetTagInterface, "AssetTag");
                        }

                        for (const char* interface : hasIndicatorLed)
                        {
                            if (std::find(interfaces2.begin(),
                                          interfaces2.end(),
                                          interface) != interfaces2.end())
                            {
                                getIndicatorLedState(asyncResp);
                                getLocationIndicatorActive(asyncResp);
                                break;
                            }
                        }

                        const std::string locationInterface =
                            "xyz.openbmc_project.Inventory.Decorator."
                            "LocationCode";
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      locationInterface) != interfaces2.end())
                        {
                            crow::connections::systemBus->async_method_call(
                                [asyncResp, chassisId(std::string(chassisId))](
                                    const boost::system::error_code ec,
                                    const std::variant<std::string>& property) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error for "
                                               "Location";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }

                                    const std::string* value =
                                        std::get_if<std::string>(&property);
                                    if (value == nullptr)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Null value returned "
                                               "for locaton code";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res
                                        .jsonValue["Location"]["PartLocation"]
                                                  ["ServiceLabel"] = *value;
                                },
                                connectionName, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                locationInterface, "LocationCode");
                        }

                        crow::connections::systemBus->async_method_call(
                            [asyncResp, chassisId(std::string(chassisId))](
                                const boost::system::error_code /*ec2*/,
                                const std::vector<
                                    std::pair<std::string, VariantType>>&
                                    propertiesList) {
                                for (const std::pair<std::string, VariantType>&
                                         property : propertiesList)
                                {
                                    // Store DBus properties that are also
                                    // Redfish properties with same name and a
                                    // string value
                                    const std::string& propertyName =
                                        property.first;
                                    if ((propertyName == "PartNumber") ||
                                        (propertyName == "SerialNumber") ||
                                        (propertyName == "Manufacturer") ||
                                        (propertyName == "Model"))
                                    {
                                        const std::string* value =
                                            std::get_if<std::string>(
                                                &property.second);
                                        if (value != nullptr)
                                        {
                                            asyncResp->res
                                                .jsonValue[propertyName] =
                                                *value;
                                        }
                                    }
                                }
                                asyncResp->res.jsonValue["Name"] = chassisId;
                                asyncResp->res.jsonValue["Id"] = chassisId;
#ifdef BMCWEB_ALLOW_DEPRECATED_POWER_THERMAL
                                asyncResp->res.jsonValue["Thermal"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Thermal"}};
                                // Power object
                                asyncResp->res.jsonValue["Power"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Power"}};
#endif
                                // SensorCollection
                                asyncResp->res.jsonValue["Sensors"] = {
                                    {"@odata.id", "/redfish/v1/Chassis/" +
                                                      chassisId + "/Sensors"}};
                                asyncResp->res.jsonValue["Status"] = {
                                    {"State", "Enabled"},
                                };

                                asyncResp->res
                                    .jsonValue["Links"]["ComputerSystems"] = {
                                    {{"@odata.id",
                                      "/redfish/v1/Systems/system"}}};
                                asyncResp->res.jsonValue["Links"]["ManagedBy"] =
                                    {{{"@odata.id",
                                       "/redfish/v1/Managers/bmc"}}};
                                getChassisState(asyncResp);
                            },
                            connectionName, path,
                            "org.freedesktop.DBus.Properties", "GetAll",
                            "xyz.openbmc_project.Inventory.Decorator.Asset");

                        // Chassis UUID
                        const std::string uuidInterface =
                            "xyz.openbmc_project.Common.UUID";
                        if (std::find(interfaces2.begin(), interfaces2.end(),
                                      uuidInterface) != interfaces2.end())
                        {
                            crow::connections::systemBus->async_method_call(
                                [asyncResp](const boost::system::error_code ec,
                                            const std::variant<std::string>&
                                                chassisUUID) {
                                    if (ec)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "DBUS response error for "
                                               "UUID";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    const std::string* value =
                                        std::get_if<std::string>(&chassisUUID);
                                    if (value == nullptr)
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Null value returned "
                                               "for UUID";
                                        messages::internalError(asyncResp->res);
                                        return;
                                    }
                                    asyncResp->res.jsonValue["UUID"] = *value;
                                },
                                connectionName, path,
                                "org.freedesktop.DBus.Properties", "Get",
                                uuidInterface, "UUID");
                        }

                        return;
                    }

                    // Couldn't find an object with that name.  return an error
                    messages::resourceNotFound(
                        asyncResp->res, "#Chassis.v1_14_0.Chassis", chassisId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, interfaces);

            getPhysicalSecurityData(asyncResp);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/")
        .privileges(redfish::privileges::patchChassis)
        .methods(
            boost::beast::http::verb::
                patch)([](const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& param) {
            std::optional<bool> locationIndicatorActive;
            std::optional<std::string> indicatorLed;

            if (param.empty())
            {
                return;
            }

            if (!json_util::readJson(
                    req, asyncResp->res, "LocationIndicatorActive",
                    locationIndicatorActive, "IndicatorLED", indicatorLed))
            {
                return;
            }

            // TODO (Gunnar): Remove IndicatorLED after enough time has passed
            if (!locationIndicatorActive && !indicatorLed)
            {
                return; // delete this when we support more patch properties
            }
            if (indicatorLed)
            {
                asyncResp->res.addHeader(
                    boost::beast::http::field::warning,
                    "299 - \"IndicatorLED is deprecated. Use "
                    "LocationIndicatorActive instead.\"");
            }

            const std::array<const char*, 2> interfaces = {
                "xyz.openbmc_project.Inventory.Item.Board",
                "xyz.openbmc_project.Inventory.Item.Chassis"};

            const std::string& chassisId = param;

            crow::connections::systemBus->async_method_call(
                [asyncResp, chassisId, locationIndicatorActive, indicatorLed](
                    const boost::system::error_code ec,
                    const crow::openbmc_mapper::GetSubTreeType& subtree) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // Iterate over all retrieved ObjectPaths.
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<std::string,
                                                   std::vector<std::string>>>>&
                             object : subtree)
                    {
                        const std::string& path = object.first;
                        const std::vector<
                            std::pair<std::string, std::vector<std::string>>>&
                            connectionNames = object.second;

                        if (!boost::ends_with(path, chassisId))
                        {
                            continue;
                        }

                        if (connectionNames.size() < 1)
                        {
                            BMCWEB_LOG_ERROR << "Got 0 Connection names";
                            continue;
                        }

                        const std::vector<std::string>& interfaces3 =
                            connectionNames[0].second;

                        const std::array<const char*, 2> hasIndicatorLed = {
                            "xyz.openbmc_project.Inventory.Item.Panel",
                            "xyz.openbmc_project.Inventory.Item.Board."
                            "Motherboard"};
                        bool indicatorChassis = false;
                        for (const char* interface : hasIndicatorLed)
                        {
                            if (std::find(interfaces3.begin(),
                                          interfaces3.end(),
                                          interface) != interfaces3.end())
                            {
                                indicatorChassis = true;
                                break;
                            }
                        }
                        if (locationIndicatorActive)
                        {
                            if (indicatorChassis)
                            {
                                setLocationIndicatorActive(
                                    asyncResp, *locationIndicatorActive);
                            }
                            else
                            {
                                messages::propertyUnknown(
                                    asyncResp->res, "LocationIndicatorActive");
                            }
                        }
                        if (indicatorLed)
                        {
                            if (indicatorChassis)
                            {
                                setIndicatorLedState(asyncResp, *indicatorLed);
                            }
                            else
                            {
                                messages::propertyUnknown(asyncResp->res,
                                                          "IndicatorLED");
                            }
                        }
                        return;
                    }

                    messages::resourceNotFound(
                        asyncResp->res, "#Chassis.v1_14_0.Chassis", chassisId);
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetSubTree",
                "/xyz/openbmc_project/inventory", 0, interfaces);
        });
}

inline void
    doChassisPowerCycle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const char* busName = "xyz.openbmc_project.ObjectMapper";
    const char* path = "/xyz/openbmc_project/object_mapper";
    const char* interface = "xyz.openbmc_project.ObjectMapper";
    const char* method = "GetSubTreePaths";

    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.State.Chassis"};

    // Use mapper to get subtree paths.
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::vector<std::string>& chassisList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "[mapper] Bad D-Bus request error: " << ec;
                messages::internalError(asyncResp->res);
                return;
            }

            const char* processName = "xyz.openbmc_project.State.Chassis";
            const char* interfaceName = "xyz.openbmc_project.State.Chassis";
            const char* destProperty = "RequestedPowerTransition";
            const std::string propertyValue =
                "xyz.openbmc_project.State.Chassis.Transition.PowerCycle";
            std::string objectPath =
                "/xyz/openbmc_project/state/chassis_system0";

            /* Look for system reset chassis path */
            if ((std::find(chassisList.begin(), chassisList.end(),
                           objectPath)) == chassisList.end())
            {
                /* We prefer to reset the full chassis_system, but if it doesn't
                 * exist on some platforms, fall back to a host-only power reset
                 */
                objectPath = "/xyz/openbmc_project/state/chassis0";
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code ec) {
                    // Use "Set" method to set the property value.
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "[Set] Bad D-Bus request error: "
                                         << ec;
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    messages::success(asyncResp->res);
                },
                processName, objectPath, "org.freedesktop.DBus.Properties",
                "Set", interfaceName, destProperty,
                std::variant<std::string>{propertyValue});
        },
        busName, path, interface, method, "/", 0, interfaces);
}

inline void getIntrusionByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                  const std::string& service,
                                  const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get intrusion status by service \n";

    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](const boost::system::error_code ec,
                                  const std::variant<std::string>& value) {
            if (ec)
            {
                // do not add err msg in redfish response, because this is not
                //     mandatory property
                BMCWEB_LOG_ERROR << "DBUS response error " << ec << "\n";
                return;
            }

            const std::string* status = std::get_if<std::string>(&value);

            if (status == nullptr)
            {
                BMCWEB_LOG_ERROR << "intrusion status read error \n";
                return;
            }

            aResp->res.jsonValue["PhysicalSecurity"] = {
                {"IntrusionSensorNumber", 1}, {"IntrusionSensor", *status}};
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Chassis.Intrusion", "Status");
}

inline void getPhysicalSecurityData(std::shared_ptr<bmcweb::AsyncResp> aResp)
{
    crow::connections::systemBus->async_method_call(
        [aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const std::vector<std::pair<
                std::string,
                std::vector<std::pair<std::string, std::vector<std::string>>>>>&
                subtree) {
            if (ec)
            {
                // do not add err msg in redfish response, because this is not
                //     mandatory property
                BMCWEB_LOG_ERROR << "DBUS error: no matched iface " << ec
                                 << "\n";
                return;
            }
            // Iterate over all retrieved ObjectPaths.
            for (const auto& object : subtree)
            {
                for (const auto& service : object.second)
                {
                    getIntrusionByService(aResp, service.first, object.first);
                    return;
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/Intrusion", 1,
        std::array<const char*, 1>{"xyz.openbmc_project.Chassis.Intrusion"});
}

void requestRoutesChassisResetAction(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Actions/Chassis.Reset/")
        .privileges(redfish::privileges::postChassis)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string&) {
                BMCWEB_LOG_DEBUG << "Post Chassis Reset.";

                std::string resetType;

                if (!json_util::readJson(req, asyncResp->res, "ResetType",
                                         resetType))
                {
                    return;
                }

                if (resetType != "PowerCycle")
                {
                    BMCWEB_LOG_DEBUG << "Invalid property value for ResetType: "
                                     << resetType;
                    messages::actionParameterNotSupported(
                        asyncResp->res, resetType, "ResetType");

                    return;
                }
                doChassisPowerCycle(asyncResp);
            });
}

void requestRoutesChassisResetActionInfo(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& chassisId)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                    {"@odata.id",
                     "/redfish/v1/Chassis/" + chassisId + "/ResetActionInfo"},
                    {"Name", "Reset Action Info"},
                    {"Id", "ResetActionInfo"},
                    {"Parameters",
                     {{{"Name", "ResetType"},
                       {"Required", true},
                       {"DataType", "String"},
                       {"AllowableValues", {"PowerCycle"}}}}}};
            });
}

void requestRoutesSystemPCIeFunction(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/")
        .privileges(redfish::privileges::getPCIeFunction)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& device,
                                              const std::string& function) {
            auto getPCIeDeviceCallback = [asyncResp, device, function](
                                             const boost::system::error_code ec,
                                             boost::container::flat_map<
                                                 std::string,
                                                 std::variant<std::string>>&
                                                 pcieDevProperties) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "failed to get PCIe Device properties ec: "
                        << ec.value() << ": " << ec.message();
                    if (ec.value() ==
                        boost::system::linux_error::bad_request_descriptor)
                    {
                        messages::resourceNotFound(asyncResp->res, "PCIeDevice",
                                                   device);
                    }
                    else
                    {
                        messages::internalError(asyncResp->res);
                    }
                    return;
                }

                // Check if this function exists by looking for a device ID
                std::string devIDProperty = "Function" + function + "DeviceId";
                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties[devIDProperty]);
                    property && property->empty())
                {
                    messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                               function);
                    return;
                }

                asyncResp->res.jsonValue = {
                    {"@odata.type", "#PCIeFunction.v1_2_0.PCIeFunction"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions/" + function},
                    {"Name", "PCIe Function"},
                    {"Id", function},
                    {"FunctionId", std::stoi(function)},
                    {"Links",
                     {{"PCIeDevice",
                       {{"@odata.id",
                         "/redfish/v1/Systems/system/PCIeDevices/" +
                             device}}}}}};

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function + "DeviceId"]);
                    property)
                {
                    asyncResp->res.jsonValue["DeviceId"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function + "VendorId"]);
                    property)
                {
                    asyncResp->res.jsonValue["VendorId"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "FunctionType"]);
                    property)
                {
                    asyncResp->res.jsonValue["FunctionType"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "DeviceClass"]);
                    property)
                {
                    asyncResp->res.jsonValue["DeviceClass"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "ClassCode"]);
                    property)
                {
                    asyncResp->res.jsonValue["ClassCode"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "RevisionId"]);
                    property)
                {
                    asyncResp->res.jsonValue["RevisionId"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "SubsystemId"]);
                    property)
                {
                    asyncResp->res.jsonValue["SubsystemId"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Function" + function +
                                           "SubsystemVendorId"]);
                    property)
                {
                    asyncResp->res.jsonValue["SubsystemVendorId"] = *property;
                }
            };
            std::string escapedPath = std::string(pciePath) + "/" + device;
            dbus::utility::escapePathForDbus(escapedPath);
            crow::connections::systemBus->async_method_call(
                std::move(getPCIeDeviceCallback), pcieService, escapedPath,
                "org.freedesktop.DBus.Properties", "GetAll",
                pcieDeviceInterface);
        });
}

void requestRoutesSystemPCIeFunctionCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/")
        .privileges(redfish::privileges::getPCIeFunctionCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& device)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeFunctionCollection.PCIeFunctionCollection"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions"},
                    {"Name", "PCIe Function Collection"},
                    {"Description",
                     "Collection of PCIe Functions for PCIe Device " + device}};

                auto getPCIeDeviceCallback = [asyncResp, device](
                                                 const boost::system::error_code
                                                     ec,
                                                 boost::container::flat_map<
                                                     std::string,
                                                     std::variant<std::string>>&
                                                     pcieDevProperties) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "failed to get PCIe Device properties ec: "
                            << ec.value() << ": " << ec.message();
                        if (ec.value() ==
                            boost::system::linux_error::bad_request_descriptor)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "PCIeDevice", device);
                        }
                        else
                        {
                            messages::internalError(asyncResp->res);
                        }
                        return;
                    }

                    nlohmann::json& pcieFunctionList =
                        asyncResp->res.jsonValue["Members"];
                    pcieFunctionList = nlohmann::json::array();
                    static constexpr const int maxPciFunctionNum = 8;
                    for (int functionNum = 0; functionNum < maxPciFunctionNum;
                         functionNum++)
                    {
                        // Check if this function exists by looking for a device
                        // ID
                        std::string devIDProperty =
                            "Function" + std::to_string(functionNum) +
                            "DeviceId";
                        std::string* property = std::get_if<std::string>(
                            &pcieDevProperties[devIDProperty]);
                        if (property && !property->empty())
                        {
                            pcieFunctionList.push_back(
                                {{"@odata.id",
                                  "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions/" +
                                      std::to_string(functionNum)}});
                        }
                    }
                    asyncResp->res.jsonValue["PCIeFunctions@odata.count"] =
                        pcieFunctionList.size();
                };
                std::string escapedPath = std::string(pciePath) + "/" + device;
                dbus::utility::escapePathForDbus(escapedPath);
                crow::connections::systemBus->async_method_call(
                    std::move(getPCIeDeviceCallback), pcieService, escapedPath,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    pcieDeviceInterface);
            });
}

void requestRoutesSystemPCIeDevice(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/PCIeDevices/<str>/")
        .privileges(redfish::privileges::getPCIeDevice)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& device)

            {
                auto getPCIeDeviceCallback = [asyncResp, device](
                                                 const boost::system::error_code
                                                     ec,
                                                 boost::container::flat_map<
                                                     std::string,
                                                     std::variant<std::string>>&
                                                     pcieDevProperties) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "failed to get PCIe Device properties ec: "
                            << ec.value() << ": " << ec.message();
                        if (ec.value() ==
                            boost::system::linux_error::bad_request_descriptor)
                        {
                            messages::resourceNotFound(asyncResp->res,
                                                       "PCIeDevice", device);
                        }
                        else
                        {
                            messages::internalError(asyncResp->res);
                        }
                        return;
                    }

                    asyncResp->res.jsonValue = {
                        {"@odata.type", "#PCIeDevice.v1_4_0.PCIeDevice"},
                        {"@odata.id",
                         "/redfish/v1/Systems/system/PCIeDevices/" + device},
                        {"Name", "PCIe Device"},
                        {"Id", device}};

                    if (std::string* property = std::get_if<std::string>(
                            &pcieDevProperties["Manufacturer"]);
                        property)
                    {
                        asyncResp->res.jsonValue["Manufacturer"] = *property;
                    }

                    if (std::string* property = std::get_if<std::string>(
                            &pcieDevProperties["DeviceType"]);
                        property)
                    {
                        asyncResp->res.jsonValue["DeviceType"] = *property;
                    }

                    asyncResp->res.jsonValue["PCIeFunctions"] = {
                        {"@odata.id",
                         "/redfish/v1/Systems/system/PCIeDevices/" + device +
                             "/PCIeFunctions"}};
                };
                std::string escapedPath = std::string(pciePath) + "/" + device;
                dbus::utility::escapePathForDbus(escapedPath);
                crow::connections::systemBus->async_method_call(
                    std::move(getPCIeDeviceCallback), pcieService, escapedPath,
                    "org.freedesktop.DBus.Properties", "GetAll",
                    pcieDeviceInterface);
            });
}

void requestRoutesSystemPCIeDeviceCollection(App& app)
{
    /**
     * Functions triggers appropriate requests on DBus
     */
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/system/PCIeDevices/")
        .privileges(redfish::privileges::getPCIeDeviceCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)

            {
                asyncResp->res.jsonValue = {
                    {"@odata.type",
                     "#PCIeDeviceCollection.PCIeDeviceCollection"},
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices"},
                    {"Name", "PCIe Device Collection"},
                    {"Description", "Collection of PCIe Devices"},
                    {"Members", nlohmann::json::array()},
                    {"Members@odata.count", 0}};
                getPCIeDeviceList(asyncResp, "Members");
            });
}

}