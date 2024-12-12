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

static constexpr std::string_view BiosConfigManagerPath =
    "/xyz/openbmc_project/bios_config/manager";
static constexpr std::string_view BiosConfigManagerInterface =
    "xyz.openbmc_project.BIOSConfig.Manager";

using AttributeValue = dbus::utility::DbusVariantType;
using BaseTableOption = std::tuple<std::string, AttributeValue, std::string>;
enum BaseTableOptionIndex
{
    BaseTableOptionIndex_BoundType = 0,
    BaseTableOptionIndex_Value,
    BaseTableOptionIndex_Name
};
using BaseTableAttribute =
    std::tuple<std::string, bool, std::string, std::string, std::string,
               AttributeValue, AttributeValue, std::vector<BaseTableOption>>;
enum BaseTableAttributeIndex
{
    BaseTableAttributeIndex_Type = 0,
    BaseTableAttributeIndex_ReadOnly,
    BaseTableAttributeIndex_Name,
    BaseTableAttributeIndex_Description,
    BaseTableAttributeIndex_Path,
    BaseTableAttributeIndex_CurrentValue,
    BaseTableAttributeIndex_DefaultValue,
    BaseTableAttributeIndex_Options
};
using BaseTable = std::map<std::string, BaseTableAttribute>;
using PendingAttribute = std::tuple<std::string, AttributeValue>;
enum PendingAttributeIndex
{
    PendingAttributeIndex_Type = 0,
    PendingAttributeIndex_Value
};
using PendingAttributes = std::map<std::string, PendingAttribute>;

inline void
    populateSettings(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@Redfish.Settings"]["@odata.type"] =
        "#Settings.v1_3_5.Settings";
    asyncResp->res.jsonValue["@Redfish.Settings"]["SupportedApplyTimes"] =
        std::vector<std::string>{"OnReset"};
    asyncResp->res
        .jsonValue["@Redfish.Settings"]["SettingsObject"]["@odata.id"] =
        std::format("/redfish/v1/Systems/{}/Bios/Settings",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
}

inline void addAttribute(nlohmann::json& attributes, const std::string& name,
                         const AttributeValue& value)
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

inline void populateRedfishFromBaseTable(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const BaseTable& baseTable)
{
    nlohmann::json& attributes = asyncResp->res.jsonValue["Attributes"];
    for (const auto& [name, baseTableAttribute] : baseTable)
    {
        addAttribute(
            attributes, name,
            std::get<BaseTableAttributeIndex_CurrentValue>(baseTableAttribute));
    }
    populateSettings(asyncResp);
}

inline void populateRedfishFromPending(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const PendingAttributes& pendingAttributes)
{
    nlohmann::json& attributes = asyncResp->res.jsonValue["Attributes"];
    for (const auto& [name, pendingAttribute] : pendingAttributes)
    {
        addAttribute(attributes, name,
                     std::get<PendingAttributeIndex_Value>(pendingAttribute));
    }
}

inline bool populatePendingFromRedfish(
    PendingAttributes& pendingAttributes, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    nlohmann::json jsonAttributes;
    if (!json_util::readJsonPatch(req, asyncResp->res, "Attributes",
                                  jsonAttributes))
    {
        BMCWEB_LOG_ERROR("Invalid JSON request.");
        return false;
    }

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
        messages::propertyValueTypeError(asyncResp->res, value, name);
        return false;
    }
    return true;
}

template <typename CallbackFunc>
inline void getBIOSManagerObject(CallbackFunc&& callback)
{
    dbus::utility::getDbusObject(
        std::string(BiosConfigManagerPath),
        std::array<std::string_view, 1>{BiosConfigManagerInterface},
        [callback = std::forward<CallbackFunc>(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                return;
            }
            if (object.size() > 1)
            {
                BMCWEB_LOG_ERROR("More than one BIOS manager object found");
                return;
            }
            callback(object.begin()->first);
        });
}

template <typename T>
inline void setBIOSManagerProperty(const std::string& propertyName,
                                   const T& propertyValue,
                                   const std::string& objectPath)
{
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, objectPath,
        std::string(BiosConfigManagerPath),
        std::string(BiosConfigManagerInterface), propertyName, propertyValue,
        [propertyName](const boost::system::error_code& ec2) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("DBus response error for setting {}: {}",
                                 propertyName, ec2);
                return;
            }
        });
}

template <typename T>
inline void getBIOSManagerProperty(const std::string& property,
                                   std::function<void(const T&)> handler,
                                   const std::string& objectPath)
{
    sdbusplus::asio::getProperty<T>(
        *crow::connections::systemBus, objectPath,
        std::string(BiosConfigManagerPath),
        std::string(BiosConfigManagerInterface), property,
        [property, handler{std::move(handler)}](
            const boost::system::error_code& ec2, const T value) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("DBus response error for {}: {}", property,
                                 ec2);
                return;
            }
            handler(value);
        });
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
    if constexpr (BMCWEB_BIOS_SETTINGS)
    {
        getBIOSManagerObject(std::bind_front(
            getBIOSManagerProperty<BaseTable>, "BaseBIOSTable",
            std::bind_front(populateRedfishFromBaseTable, asyncResp)));
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

    if constexpr (BMCWEB_BIOS_SETTINGS)
    {
        getBIOSManagerObject(std::bind_front(
            getBIOSManagerProperty<PendingAttributes>, "PendingAttributes",
            std::bind_front(populateRedfishFromPending, asyncResp)));
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

    PendingAttributes pendingAttributes;
    if (populatePendingFromRedfish(pendingAttributes, req, asyncResp))
    {
        getBIOSManagerObject(
            std::bind_front(setBIOSManagerProperty<PendingAttributes>,
                            "PendingAttributes", pendingAttributes));
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

    auto callback =
        [req, asyncResp](const PendingAttributes& currentPendingAttributes) {
            PendingAttributes pendingAttributes = currentPendingAttributes;
            if (populatePendingFromRedfish(pendingAttributes, req, asyncResp))
            {
                getBIOSManagerObject(
                    std::bind_front(setBIOSManagerProperty<PendingAttributes>,
                                    "PendingAttributes", pendingAttributes));
            }
        };
    getBIOSManagerObject(
        std::bind_front(getBIOSManagerProperty<PendingAttributes>,
                        "PendingAttributes", callback));
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
}

inline void requestRoutesPendingBios(App& app)
{
    if constexpr (BMCWEB_BIOS_SETTINGS)
    {
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
