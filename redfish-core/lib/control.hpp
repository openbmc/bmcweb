#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/control.hpp"
#include "generated/enums/resource.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <functional>
#include <optional>
#include <string_view>
#include <variant>

namespace redfish
{

// Type aliases for converter functions
using ControlModeToDbusConverter = std::function<
    std::optional<dbus::utility::DbusVariantType>(control::ControlMode)>;
using DbusToControlModeConverter = std::function<
    std::optional<control::ControlMode>(const dbus::utility::DbusVariantType&)>;

// Structure to define mapping between DBus interface and Redfish Control
// properties
struct ControlInterfaceMapping
{
    std::string_view dbusInterface;
    control::ControlType controlType;
    std::string_view setPointUnits;

    // Property name mappings: DBus property name -> Redfish property name
    struct PropertyMapping
    {
        std::string_view dbusProperty;
        std::string_view redfishProperty;
    };

    std::vector<PropertyMapping> propertyMappings;

    // Function to convert Redfish ControlMode to DBus property value
    // Returns std::nullopt if the mode is not supported
    ControlModeToDbusConverter controlModeToDbusValue;

    // Function to convert DBus property value to Redfish ControlMode
    DbusToControlModeConverter dbusValueToControlMode;
};

// Define the interface mappings - add new interfaces here to extend support
inline const std::array<ControlInterfaceMapping, 1> controlInterfaceMappings = {
    {{"xyz.openbmc_project.Control.Power.Cap",
      control::ControlType::Power,
      "W",
      {{"PowerCap", "SetPoint"},
       {"MinPowerCapValue", "AllowableMin"},
       {"MaxPowerCapValue", "AllowableMax"},
       {"PowerCapEnable", "ControlMode"}},
      // ControlMode to DBus converter: For Power.Cap, only Disabled=false, all
      // others=true
      [](control::ControlMode mode)
          -> std::optional<dbus::utility::DbusVariantType> {
          if (mode == control::ControlMode::Disabled)
          {
              return false;
          }
          if (mode == control::ControlMode::Automatic ||
              mode == control::ControlMode::Override ||
              mode == control::ControlMode::Manual)
          {
              return true;
          }
          return std::nullopt; // Invalid mode
      },
      // DBus to ControlMode converter: For Power.Cap, true=Automatic,
      // false=Disabled
      [](const dbus::utility::DbusVariantType& value)
          -> std::optional<control::ControlMode> {
          const bool* boolValue = std::get_if<bool>(&value);
          if (boolValue != nullptr)
          {
              return *boolValue ? control::ControlMode::Automatic
                                : control::ControlMode::Disabled;
          }
          return std::nullopt;
      }}}};

inline std::string_view findDbusPropertyName(
    const ControlInterfaceMapping& mapping, std::string_view redfishProperty)
{
    for (const auto& propMapping : mapping.propertyMappings)
    {
        if (propMapping.redfishProperty == redfishProperty)
        {
            return propMapping.dbusProperty;
        }
    }
    return "";
}

inline std::string_view findRedfishPropertyName(
    const ControlInterfaceMapping& mapping, std::string_view dbusProperty)
{
    for (const auto& propMapping : mapping.propertyMappings)
    {
        if (propMapping.dbusProperty == dbusProperty)
        {
            return propMapping.redfishProperty;
        }
    }
    return "";
}

inline std::optional<control::ControlMode> parseControlMode(
    std::string_view controlModeStr)
{
    if (controlModeStr == "Automatic")
    {
        return control::ControlMode::Automatic;
    }
    if (controlModeStr == "Manual")
    {
        return control::ControlMode::Manual;
    }
    if (controlModeStr == "Override")
    {
        return control::ControlMode::Override;
    }
    if (controlModeStr == "Disabled")
    {
        return control::ControlMode::Disabled;
    }
    return std::nullopt;
}

inline const ControlInterfaceMapping* findControlMapping(
    const std::string& interface)
{
    for (const auto& mapping : controlInterfaceMappings)
    {
        if (mapping.dbusInterface == interface)
        {
            return &mapping;
        }
    }
    return nullptr;
}

inline void getControlPaths(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    const std::function<
        void(const dbus::utility::MapperGetSubTreePathsResponse&)>& callback)
{
    sdbusplus::message::object_path endpointPath{validChassisPath};
    endpointPath /= "all_controls";

    dbus::utility::getAssociatedSubTreePaths(
        endpointPath,
        sdbusplus::message::object_path("/xyz/openbmc_project/controls"), 0,
        std::array<std::string_view, 0>{},
        [asyncResp, callback](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreePathsResponse& controlPaths) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "DBUS response error for getControlPaths: {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                // No controls found - return empty list
                callback({});
                return;
            }
            callback(controlPaths);
        });
}

inline std::string getControlIdFromPath(const std::string& path)
{
    sdbusplus::message::object_path objPath(path);
    return objPath.filename();
}

inline void addControlCommonProperties(crow::Response& resp,
                                       const std::string& chassisId,
                                       const std::string& controlId)
{
    resp.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Control/Control.json>; rel=describedby");
    resp.jsonValue["@odata.type"] = "#Control.v1_3_0.Control";
    resp.jsonValue["Name"] = "Control";
    resp.jsonValue["Id"] = controlId;
    resp.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
}

// Process DBus properties and populate Redfish response
inline void processControlProperties(
    nlohmann::json& jsonResponse, const ControlInterfaceMapping& mapping,
    const dbus::utility::DBusPropertiesMap& properties)
{
    jsonResponse["ControlType"] = mapping.controlType;
    jsonResponse["SetPointUnits"] = mapping.setPointUnits;

    for (const auto& [dbusProperty, dbusValue] : properties)
    {
        std::string_view redfishProperty =
            findRedfishPropertyName(mapping, dbusProperty);

        if (redfishProperty.empty())
        {
            continue; // Property not in mapping, skip it
        }

        // Handle ControlMode with converter function
        if (redfishProperty == "ControlMode" && mapping.dbusValueToControlMode)
        {
            std::optional<control::ControlMode> mode =
                mapping.dbusValueToControlMode(dbusValue);
            if (mode)
            {
                jsonResponse[std::string(redfishProperty)] = *mode;
            }
        }
        else
        {
            // For all other properties, use std::visit to convert the variant
            std::visit(
                [&jsonResponse, redfishProperty](auto&& val) {
                    jsonResponse[std::string(redfishProperty)] = val;
                },
                dbusValue);
        }
    }
}

inline void doControlCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ControlCollection/ControlCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ControlCollection.ControlCollection";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Controls", chassisId);
    asyncResp->res.jsonValue["Name"] = "Controls";
    asyncResp->res.jsonValue["Description"] =
        "The collection of Control resource instances for " + chassisId;
    asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Members@odata.count"] = 0;

    getControlPaths(
        asyncResp, *validChassisPath,
        [asyncResp, chassisId](
            const dbus::utility::MapperGetSubTreePathsResponse& controlPaths) {
            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            for (const auto& path : controlPaths)
            {
                std::string controlId = getControlIdFromPath(path);
                nlohmann::json::object_t member;
                member["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Chassis/{}/Controls/{}", chassisId, controlId);
                members.emplace_back(std::move(member));
            }
            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
        });
}

inline void handleControlCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doControlCollection, asyncResp, chassisId));
}

// After getting control object and service, fetch properties
inline void afterGetControlObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& controlPath,
    const dbus::utility::MapperGetObject& object)
{
    if (object.empty())
    {
        BMCWEB_LOG_ERROR("No service found for control path: {}", controlPath);
        messages::internalError(asyncResp->res);
        return;
    }

    addControlCommonProperties(asyncResp->res, chassisId, controlId);

    // Add RelatedItem pointing to the chassis
    nlohmann::json::object_t relatedItem;
    relatedItem["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}", chassisId);
    asyncResp->res.jsonValue["RelatedItem"] =
        nlohmann::json::array({relatedItem});

    for (const auto& [serviceName, interfaces] : object)
    {
        // Iterate through all interfaces and process known ones
        for (const std::string& interfaceName : interfaces)
        {
            const ControlInterfaceMapping* mapping =
                findControlMapping(interfaceName);
            if (mapping == nullptr)
            {
                continue;
            }

            dbus::utility::getAllProperties(
                serviceName, controlPath, interfaceName,
                [asyncResp, mapping, interfaceName](
                    const boost::system::error_code& ec,
                    const dbus::utility::DBusPropertiesMap& properties) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR(
                            "Error getting control properties for interface {}: {}",
                            interfaceName, ec);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    processControlProperties(asyncResp->res.jsonValue, *mapping,
                                             properties);
                });
        }
    }
}

inline void getControlObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId,
    const std::string& validChassisPath,
    const std::function<void(const std::string& controlPath,
                             const dbus::utility::MapperGetObject&)>& callback)
{
    getControlPaths(
        asyncResp, validChassisPath,
        [asyncResp, chassisId, controlId, callback](
            const dbus::utility::MapperGetSubTreePathsResponse& controlPaths) {
            for (const auto& path : controlPaths)
            {
                if (getControlIdFromPath(path) != controlId)
                {
                    continue;
                }

                dbus::utility::getDbusObject(
                    path, {},
                    [asyncResp, callback,
                     path](const boost::system::error_code& ec,
                           const dbus::utility::MapperGetObject& object) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR(
                                "Error getting control service for path {}: {}",
                                path, ec);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        callback(path, object);
                    });
                return;
            }
            BMCWEB_LOG_WARNING("Control {} not found in chassis {}", controlId,
                               chassisId);
            messages::resourceNotFound(asyncResp->res, "Control", controlId);
        });
}

inline void doControlGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& chassisId,
                         const std::string& controlId,
                         const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getControlObject(asyncResp, chassisId, controlId, *validChassisPath,
                     std::bind_front(afterGetControlObject, asyncResp,
                                     chassisId, controlId));
}

inline void handleControlGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        std::bind_front(doControlGet, asyncResp, chassisId, controlId));
}

// Helper function to set a DBus property after determining its type
inline void setControlSetPoint(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& controlPath,
    const std::string& matchedInterface, std::string_view dbusProperty,
    const nlohmann::json& setPoint,
    const dbus::utility::DBusPropertiesMap& properties)
{
    // Find the SetPoint property
    auto it =
        std::ranges::find_if(properties, [&dbusProperty](const auto& prop) {
            return prop.first == dbusProperty;
        });

    if (it == properties.end())
    {
        BMCWEB_LOG_ERROR("SetPoint property {} not found", dbusProperty);
        messages::internalError(asyncResp->res);
        return;
    }

    // Convert JSON to the same type and set property
    std::visit(
        [&](auto&& val) {
            using T = std::decay_t<decltype(val)>;
            T newValue = val; // Default to current

            if constexpr (std::is_arithmetic_v<T>)
            {
                if (setPoint.is_number())
                {
                    newValue = static_cast<T>(setPoint.get<double>());
                }
            }

            sdbusplus::asio::setProperty(
                *crow::connections::systemBus, serviceName, controlPath,
                matchedInterface, std::string(dbusProperty), newValue,
                [asyncResp](const boost::system::error_code& ec2) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR("Error setting SetPoint: {}", ec2);
                        messages::internalError(asyncResp->res);
                    }
                });
        },
        it->second);
}

// Helper function to handle SetPoint PATCH operation
inline void handleSetPointPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& controlPath,
    const std::string& matchedInterface, const ControlInterfaceMapping& mapping,
    const nlohmann::json& setPoint)
{
    std::string_view dbusProperty = findDbusPropertyName(mapping, "SetPoint");
    if (dbusProperty.empty())
    {
        BMCWEB_LOG_ERROR("SetPoint property not found in interface mapping");
        messages::internalError(asyncResp->res);
        return;
    }

    // Get all properties to determine the SetPoint type
    dbus::utility::getAllProperties(
        serviceName, controlPath, matchedInterface,
        [asyncResp, serviceName, controlPath, matchedInterface, dbusProperty,
         setPoint](const boost::system::error_code& ec,
                   const dbus::utility::DBusPropertiesMap& properties) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error getting properties: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            setControlSetPoint(asyncResp, serviceName, controlPath,
                               matchedInterface, dbusProperty, setPoint,
                               properties);
        });
}

// Helper function to handle ControlMode PATCH operation
inline void handleControlModePatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& controlPath,
    const std::string& matchedInterface, const ControlInterfaceMapping& mapping,
    control::ControlMode controlMode, const std::string& controlModeStr)
{
    std::string_view dbusProperty =
        findDbusPropertyName(mapping, "ControlMode");
    if (dbusProperty.empty())
    {
        BMCWEB_LOG_ERROR("ControlMode property not found in interface mapping");
        messages::internalError(asyncResp->res);
        return;
    }

    if (!mapping.controlModeToDbusValue)
    {
        BMCWEB_LOG_ERROR("ControlMode converter not defined for interface: {}",
                         matchedInterface);
        messages::propertyNotWritable(asyncResp->res, "ControlMode");
        return;
    }

    // Convert and set ControlMode
    std::optional<dbus::utility::DbusVariantType> dbusValue =
        mapping.controlModeToDbusValue(controlMode);

    if (!dbusValue)
    {
        messages::propertyValueNotInList(asyncResp->res, "ControlMode",
                                         controlModeStr);
        return;
    }

    std::visit(
        [&asyncResp, &serviceName, &controlPath, &matchedInterface,
         &dbusProperty](auto&& val) {
            sdbusplus::asio::setProperty(
                *crow::connections::systemBus, serviceName, controlPath,
                matchedInterface, std::string(dbusProperty), val,
                [asyncResp](const boost::system::error_code& ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("Error setting ControlMode: {}", ec);
                        messages::internalError(asyncResp->res);
                    }
                });
        },
        *dbusValue);
}

// Process the control object after retrieval for PATCH operation
inline void afterGetControlObjectForPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& controlPath,
    const dbus::utility::MapperGetObject& object,
    const std::optional<nlohmann::json>& setPoint,
    const std::optional<control::ControlMode>& controlMode,
    const std::optional<std::string>& controlModeStr)
{
    if (object.empty())
    {
        messages::internalError(asyncResp->res);
        return;
    }

    // Find a matching interface from the DBus object and track
    // which service provides it
    const ControlInterfaceMapping* mapping = nullptr;
    std::string matchedInterface;
    std::string serviceName;

    for (const auto& [svcName, interfaces] : object)
    {
        for (const auto& interfaceName : interfaces)
        {
            const ControlInterfaceMapping* tempMapping =
                findControlMapping(interfaceName);
            if (tempMapping != nullptr)
            {
                mapping = tempMapping;
                matchedInterface = interfaceName;
                serviceName = svcName;
                break;
            }
        }
        if (mapping != nullptr)
        {
            break;
        }
    }

    if (mapping == nullptr)
    {
        BMCWEB_LOG_ERROR("No supported control interface found for path: {}",
                         controlPath);
        messages::internalError(asyncResp->res);
        return;
    }

    // Handle SetPoint
    if (setPoint)
    {
        handleSetPointPatch(asyncResp, serviceName, controlPath,
                            matchedInterface, *mapping, *setPoint);
    }

    // Handle ControlMode
    if (controlMode && controlModeStr)
    {
        handleControlModePatch(asyncResp, serviceName, controlPath,
                               matchedInterface, *mapping, *controlMode,
                               *controlModeStr);
    }
}

inline void handleControlPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& controlId)
{
    if (!setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // Parse the JSON body manually to handle SetPoint as generic JSON
    nlohmann::json jsonRequest;
    if (!json_util::processJsonFromRequest(asyncResp->res, req, jsonRequest))
    {
        return;
    }

    std::optional<nlohmann::json> setPoint;
    std::optional<std::string> controlModeStr;

    // Extract SetPoint if present
    auto setPointIt = jsonRequest.find("SetPoint");
    if (setPointIt != jsonRequest.end())
    {
        if (!setPointIt->is_number())
        {
            messages::propertyValueTypeError(asyncResp->res, setPointIt->dump(),
                                             "SetPoint");
            return;
        }
        setPoint = *setPointIt;
    }

    // Extract ControlMode if present
    auto controlModeIt = jsonRequest.find("ControlMode");
    if (controlModeIt != jsonRequest.end())
    {
        if (!controlModeIt->is_string())
        {
            messages::propertyValueTypeError(
                asyncResp->res, controlModeIt->dump(), "ControlMode");
            return;
        }
        controlModeStr = controlModeIt->get<std::string>();
    }

    // Convert string to ControlMode enum
    std::optional<control::ControlMode> controlMode;
    if (controlModeStr)
    {
        controlMode = parseControlMode(*controlModeStr);
        if (!controlMode)
        {
            messages::propertyValueNotInList(asyncResp->res, "ControlMode",
                                             *controlModeStr);
            return;
        }
    }

    if (!setPoint && !controlMode)
    {
        messages::emptyJSON(asyncResp->res);
        return;
    }

    chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId, controlId, setPoint, controlMode,
         controlModeStr](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }

            getControlObject(asyncResp, chassisId, controlId, *validChassisPath,
                             [asyncResp, setPoint, controlMode, controlModeStr](
                                 const std::string& controlPath,
                                 const dbus::utility::MapperGetObject& object) {
                                 afterGetControlObjectForPatch(
                                     asyncResp, controlPath, object, setPoint,
                                     controlMode, controlModeStr);
                             });
        });
}

inline void requestRoutesControl(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/")
        .privileges(privileges::getControlCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(privileges::getControl)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleControlGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Controls/<str>/")
        .privileges(privileges::patchControl)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleControlPatch, std::ref(app)));
}

} // namespace redfish
