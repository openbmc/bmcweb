// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <boost/url/format.hpp>

namespace redfish
{

static constexpr std::string_view biosConfigManagerPath =
    "/xyz/openbmc_project/bios_config/manager";
static constexpr std::string_view biosConfigManagerInterface =
    "xyz.openbmc_project.BIOSConfig.Manager";

/*
BaseTableOption:
- Bound Type
- Value
- Option Name
examples: {"OneOf", "Power Off", "Power Off Enum"} or {"MaxBound", 4, ""}
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/BIOSConfig/Manager.interface.yaml#L62
*/
using BaseTableOption =
    std::tuple<std::string, dbus::utility::DbusVariantType, std::string>;
enum class BaseTableOptionIndex
{
    BoundType = 0,
    Value,
    Name
};

/*
BaseTableAttribute:
- Attribute Type
- Read Only Status
- Display Name
- Description
- Menu Path
- Current Value
- Default Value
- Array of Options
example:
{
    xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String,
    false,
    "Memory Operating Speed Selection",
    "Force specific Memory Operating Speed or use Auto setting.",
    "Advanced/Memory Configuration/Memory Operating Speed Selection",
    "0x00",
    "0x0B",
    [
        {"OneOf", "auto", "enum0"},
        {"OneOf", "2133", "enum1"},
        {"OneOf", "2400", "enum2"},
        {"OneOf", "2664", "enum3"},
        {"OneOf", "2933", "enum4"}
    ]
}
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/BIOSConfig/Manager.interface.yaml#L62
*/
using BaseTableAttribute =
    std::tuple<std::string, bool, std::string, std::string, std::string,
               dbus::utility::DbusVariantType, dbus::utility::DbusVariantType,
               std::vector<BaseTableOption>>;
enum class BaseTableAttributeIndex
{
    Type = 0,
    ReadOnly,
    Name,
    Description,
    Path,
    CurrentValue,
    DefaultValue,
    Options
};

/*
BaseTable:
A mapping of an attribute key to a BaseTableAttribute, used with Dbus
example:
{
    "BIOSSerialDebugLevel",
    {
        xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer,
        false,
        "BIOS Serial Debug level",
        "BIOS Serial Debug level during system boot.",
        "Advanced/Debug Feature Selection",
        0x00,
        0x01,
        [
            {"MinBound", 0, ""},
            {"MaxBound", 4, ""},
            {"ScalarIncrement", 1, ""}
        ]
    }
}
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/BIOSConfig/Manager.interface.yaml#L62
*/
using BaseTable = std::map<std::string, BaseTableAttribute>;

/*
PendingAttributeValue:
- Attribute Type
- Attribute Value
example:
{
    xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer,
    0x1
}
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/BIOSConfig/Manager.interface.yaml#L99
*/
using PendingAttributeValue =
    std::tuple<std::string, dbus::utility::DbusVariantType>;
enum class PendingAttributeValueIndex
{
    Type = 0,
    Value
};

/*
PendingAttributes:
A mapping of an attribute key to a PendingAttributeValue, used with Dbus
example:
{
    "DdrFreqLimit"
    {
        xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String,
        "2933"
    }
}
https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/yaml/xyz/openbmc_project/BIOSConfig/Manager.interface.yaml#L99
*/
using PendingAttributes = std::map<std::string, PendingAttributeValue>;

inline void populateSettings(crow::Response& response)
{
    nlohmann::json& redfishSettings = response.jsonValue["@Redfish.Settings"];
    redfishSettings["@odata.type"] = "#Settings.v1_3_5.Settings";
    redfishSettings["SupportedApplyTimes"] = nlohmann::json::array_t{"OnReset"};
    redfishSettings["SettingsObject"]["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Bios/Settings", BMCWEB_REDFISH_SYSTEM_URI_NAME);
}

inline void addAttribute(nlohmann::json& attributes, const std::string& name,
                         const dbus::utility::DbusVariantType& value)
{
    const std::string* strValue = std::get_if<std::string>(&value);
    if (strValue != nullptr)
    {
        attributes[name] = *strValue;
        return;
    }
    const int32_t* intValue = std::get_if<int32_t>(&value);
    const int64_t* int64Value = std::get_if<int64_t>(&value);
    if (intValue != nullptr)
    {
        attributes[name] = *intValue;
        return;
    }
    if (int64Value != nullptr)
    {
        attributes[name] = *int64Value;
        return;
    }
    BMCWEB_LOG_ERROR("Invalid type for attribute {} in base table", name);
}

inline void populateRedfishFromBaseTable(crow::Response& response,
                                         const BaseTable& baseTable)
{
    nlohmann::json& attributes = response.jsonValue["Attributes"];
    for (const auto& [name, baseTableAttribute] : baseTable)
    {
        addAttribute(attributes, name,
                     std::get<uint(BaseTableAttributeIndex::CurrentValue)>(
                         baseTableAttribute));
    }
}

inline void populateRedfishFromPending(
    crow::Response& res, const PendingAttributes& pendingAttributes)
{
    nlohmann::json& attributes = res.jsonValue["Attributes"];
    for (const auto& [name, pendingAttribute] : pendingAttributes)
    {
        addAttribute(attributes, name,
                     std::get<uint(PendingAttributeValueIndex::Value)>(
                         pendingAttribute));
    }
}

inline bool populatePendingFromRedfish(PendingAttributes& pendingAttributes,
                                       const nlohmann::json& jsonAttributes,
                                       crow::Response& response)
{
    for (const auto& [name, value] : jsonAttributes.items())
    {
        const std::string* strValue = value.get_ptr<const std::string*>();
        if (strValue != nullptr)
        {
            pendingAttributes[name] = std::make_tuple(
                // TODO: enum and passwords are also strings in Redfish,
                // need a way to dynamically determine the type, or needs to
                // be ignored by service.
                "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
                *strValue);
            continue;
        }
        const int64_t* intValue = value.get_ptr<const int64_t*>();
        if (intValue != nullptr)
        {
            pendingAttributes[name] = std::make_tuple(
                "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer",
                *intValue);
            continue;
        }

        BMCWEB_LOG_ERROR("Invalid type for attribute {} in request", name);
        messages::propertyValueTypeError(response, value, name);
        return false;
    }
    return true;
}

template <typename CallbackFunc>
inline void
    getBIOSManagerObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         CallbackFunc&& callback)
{
    dbus::utility::getDbusObject(
        std::string(biosConfigManagerPath),
        std::array<std::string_view, 1>{biosConfigManagerInterface},
        [asyncResp, callback = std::forward<CallbackFunc>(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_ERROR("Error finding BIOS Manager object {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            if (object.size() > 1)
            {
                BMCWEB_LOG_ERROR("More than one BIOS Manager object found");
                messages::internalError(asyncResp->res);
                return;
            }
            callback(object.begin()->first);
        });
}

template <typename T>
inline void setBIOSManagerProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& propertyName, const T& propertyValue,
    const std::string& objectPath)
{
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, objectPath,
        std::string(biosConfigManagerPath),
        std::string(biosConfigManagerInterface), propertyName, propertyValue,
        [asyncResp, propertyName](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBus response error for setting {}: {}",
                                 propertyName, ec);
                messages::internalError(asyncResp->res);
                return;
            }
        });
}

template <typename T>
inline void getBIOSManagerProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& property, std::function<void(const T&)> handler,
    const std::string& objectPath)
{
    dbus::utility::getProperty<T>(
        *crow::connections::systemBus, objectPath,
        std::string(biosConfigManagerPath),
        std::string(biosConfigManagerInterface), property,
        [asyncResp, property, handler{std::move(handler)}](
            const boost::system::error_code& ec, const T& value) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBus response error for {}: {}", property,
                                 ec);
                messages::internalError(asyncResp->res);
                return;
            }
            handler(value);
        });
}

inline void
    handleBiosServiceHead(crow::App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Bios/Bios.json>; rel=describedby");
}

/**
 * BiosService class supports handle get method for bios.
 */
inline void
    handleBiosServiceGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    asyncResp->res.jsonValue["@odata.id"] = std::format(
        "/redfish/v1/Systems/{}/Bios", BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        std::format("/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);

    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
    if constexpr (BMCWEB_REDFISH_BIOS_SETTINGS)
    {
        populateSettings(asyncResp->res);
        if ((!BMCWEB_REDFISH_BIOS_ATTRIBUTE_REGISTRY.empty()))
        {
            asyncResp->res.jsonValue["AttributeRegistry"] =
                BMCWEB_REDFISH_BIOS_ATTRIBUTE_REGISTRY;
        }
        getBIOSManagerObject(
            asyncResp,
            std::bind_front(getBIOSManagerProperty<BaseTable>, asyncResp,
                            "BaseBIOSTable",
                            std::bind_front(populateRedfishFromBaseTable,
                                            std::ref(asyncResp->res))));
    }
}

/**
 * Pending BIOS settings will handle GET, PATCH, and PUT.
 */
inline void
    handlePendingBiosGet(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Bios";
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "Pending BIOS Configuration";
    asyncResp->res.jsonValue["Description"] =
        "Pending BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "Pending";

    if constexpr (BMCWEB_REDFISH_BIOS_SETTINGS)
    {
        getBIOSManagerObject(
            asyncResp,
            std::bind_front(getBIOSManagerProperty<PendingAttributes>,
                            asyncResp, "PendingAttributes",
                            std::bind_front(populateRedfishFromPending,
                                            std::ref(asyncResp->res))));
    }
}

inline void
    handlePendingBiosPut(crow::App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    nlohmann::json jsonAttributes;
    if (!json_util::readJsonPatch(req, asyncResp->res, "Attributes",
                                  jsonAttributes))
    {
        BMCWEB_LOG_ERROR("Invalid JSON request.");
        return;
    }

    PendingAttributes pendingAttributes;
    if (populatePendingFromRedfish(pendingAttributes, jsonAttributes,
                                   asyncResp->res))
    {
        getBIOSManagerObject(
            asyncResp,
            std::bind_front(setBIOSManagerProperty<PendingAttributes>,
                            asyncResp, "PendingAttributes", pendingAttributes));
    }
}

inline void
    handlePendingBiosPatch(crow::App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    nlohmann::json jsonAttributes;
    if (!json_util::readJsonPatch(req, asyncResp->res, "Attributes",
                                  jsonAttributes))
    {
        BMCWEB_LOG_ERROR("Invalid JSON request.");
        return;
    }

    auto callback = [jsonAttributes, asyncResp](
                        const PendingAttributes& currentPendingAttributes) {
        PendingAttributes pendingAttributes = currentPendingAttributes;
        if (populatePendingFromRedfish(pendingAttributes, jsonAttributes,
                                       asyncResp->res))
        {
            getBIOSManagerObject(
                asyncResp,
                std::bind_front(setBIOSManagerProperty<PendingAttributes>,
                                asyncResp, "PendingAttributes",
                                pendingAttributes));
        }
    };
    getBIOSManagerObject(
        asyncResp, std::bind_front(getBIOSManagerProperty<PendingAttributes>,
                                   asyncResp, "PendingAttributes", callback));
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleBiosServiceHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
}

inline void requestRoutesPendingBios(App& app)
{
    if constexpr (BMCWEB_REDFISH_BIOS_SETTINGS)
    {
        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
            .privileges(redfish::privileges::getBios)
            .methods(boost::beast::http::verb::head)(
                std::bind_front(handleBiosServiceHead, std::ref(app)));

        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
            .privileges(redfish::privileges::getBios)
            .methods(boost::beast::http::verb::get)(
                std::bind_front(handlePendingBiosGet, std::ref(app)));

        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
            .privileges(redfish::privileges::putBios)
            .methods(boost::beast::http::verb::put)(
                std::bind_front(handlePendingBiosPut, std::ref(app)));

        BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Settings")
            .privileges(redfish::privileges::patchBios)
            .methods(boost::beast::http::verb::patch)(
                std::bind_front(handlePendingBiosPatch, std::ref(app)));
    }
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void
    handleBiosResetPost(crow::App& app, const crow::Request& req,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    if constexpr (BMCWEB_EXPERIMENTAL_REDFISH_MULTI_COMPUTER_SYSTEM)
    {
        // Option currently returns no systems.  TBD
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to reset bios: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "org.open_power.Software.Host.Updater", "/xyz/openbmc_project/software",
        "xyz.openbmc_project.Common.FactoryReset", "Reset");
}

inline void requestRoutesBiosReset(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/Actions/Bios.ResetBios/")
        .privileges(redfish::privileges::postBios)
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleBiosResetPost, std::ref(app)));
}

} // namespace redfish
