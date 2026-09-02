// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "user_monitor.hpp"
#include "utils/bios_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <sys/types.h>

#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace redfish
{
using BaseTableOption =
    std::tuple<std::string, dbus::utility::DbusVariantType, std::string>;

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

using BaseTable = std::map<std::string, BaseTableAttribute>;

inline void populateRedfishFromBaseTable(crow::Response& response,
                                         const BaseTable& baseTable)
{
    nlohmann::json& attributes = response.jsonValue["Attributes"];
    for (const auto& [name, baseTableAttribute] : baseTable)
    {
        bios_utils::addAttribute(
            attributes, name,
            std::get<uint(BaseTableAttributeIndex::Type)>(baseTableAttribute),
            std::get<uint(BaseTableAttributeIndex::CurrentValue)>(
                baseTableAttribute));
    }
}

inline void handleBiosManagerObjectForGetBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath)
{
    bios_utils::getBIOSManagerProperty<BaseTable>(
        asyncResp, "BaseBIOSTable", objectPath,
        std::bind_front(populateRedfishFromBaseTable,
                        std::ref(asyncResp->res)));
}

inline void getBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    bios_utils::getBIOSManagerObject(
        asyncResp, std::bind_front(handleBiosManagerObjectForGetBiosAttributes,
                                   asyncResp));
}

/**
 * BiosService class supports handle get method for bios.
 */
inline void handleBiosServiceGet(
    crow::App& app, const crow::Request& req,
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
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_2_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        boost::urls::format(
            "/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
            BMCWEB_REDFISH_SYSTEM_URI_NAME);
    dbus::utility::checkDbusPathExists(
        "/xyz/openbmc_project/bios_config/manager", [asyncResp](int rc) {
            if (rc > 0)
            {
                getBiosAttributes(asyncResp);
            }
        });
    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
}

/*
 * D-Bus type of xyz.openbmc_project.BIOSConfig.Manager BaseBIOSTable
 * (signature a{s(sbsssvva(svs))}).
 */
using BiosBaseTableSet = boost::container::flat_map<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string, bool>,
        std::variant<int64_t, std::string, bool>,
        std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                               std::string>>>>;

inline std::string getDbusBiosAttrType(const std::string& attrType)
{
    if (attrType == "Enumeration" || attrType == "String" ||
        attrType == "Password" || attrType == "Integer" ||
        attrType == "Boolean")
    {
        return std::format(
            "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.{}",
            attrType);
    }
    return "UNKNOWN";
}

inline void fillBiosTable(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<nlohmann::json::object_t>& baseBiosTableJson,
    const std::string& service)
{
    BiosBaseTableSet baseBiosTable;
    for (const nlohmann::json::object_t& attrMap : baseBiosTableJson)
    {
        nlohmann::json attrJson(attrMap);
        // CurrentValue/DefaultValue types depend on the attribute Type, so
        // they are validated explicitly per type below.
        auto currentValueIt = attrJson.find("CurrentValue");
        if (currentValueIt == attrJson.end())
        {
            messages::propertyMissing(asyncResp->res, "CurrentValue");
            return;
        }
        nlohmann::json currentValueJson = std::move(*currentValueIt);
        attrJson.erase(currentValueIt);
        auto defaultValueIt = attrJson.find("DefaultValue");
        if (defaultValueIt == attrJson.end())
        {
            messages::propertyMissing(asyncResp->res, "DefaultValue");
            return;
        }
        nlohmann::json defaultValueJson = std::move(*defaultValueIt);
        attrJson.erase(defaultValueIt);

        std::string attr;
        std::string dispName;
        std::string descr;
        std::string menuPath;
        std::string type;
        bool readOnly = false;
        std::optional<std::vector<std::string>> values;
        std::optional<int64_t> lowerBound;
        std::optional<int64_t> upperBound;
        std::optional<int64_t> scalarIncrement;
        std::optional<int64_t> minLength;
        std::optional<int64_t> maxLength;
        if (!json_util::readJson(                   //
                attrJson, asyncResp->res,           //
                "AttributeName", attr,              //
                "Description", descr,               //
                "DisplayName", dispName,            //
                "LowerBound", lowerBound,           //
                "MaxLength", maxLength,             //
                "MenuPath", menuPath,               //
                "MinLength", minLength,             //
                "ReadOnly", readOnly,               //
                "ScalarIncrement", scalarIncrement, //
                "Type", type,                       //
                "UpperBound", upperBound,           //
                "Values", values))
        {
            return;
        }
        std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                               std::string>>
            bounds;
        std::variant<int64_t, std::string, bool> currValue;
        std::variant<int64_t, std::string, bool> defaultValue;
        // A null DefaultValue is stored with a mismatched variant type to
        // mark it absent, matching the existing platform convention.
        const bool defaultIsNull = defaultValueJson.is_null();

        if (type == "Enumeration" || type == "String")
        {
            const std::string* currStr =
                currentValueJson.get_ptr<const std::string*>();
            if (currStr == nullptr)
            {
                messages::propertyValueTypeError(
                    asyncResp->res, currentValueJson, "CurrentValue");
                return;
            }
            currValue = *currStr;
            if (defaultIsNull)
            {
                defaultValue = int64_t{0};
            }
            else
            {
                const std::string* defStr =
                    defaultValueJson.get_ptr<const std::string*>();
                if (defStr == nullptr)
                {
                    messages::propertyValueTypeError(
                        asyncResp->res, defaultValueJson, "DefaultValue");
                    return;
                }
                defaultValue = *defStr;
            }
            if (type == "Enumeration")
            {
                if (!values)
                {
                    messages::propertyMissing(asyncResp->res, "Values");
                    return;
                }
                for (const std::string& value : *values)
                {
                    bounds.emplace_back(
                        "xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf",
                        value, "");
                }
            }
            else
            {
                if (!minLength || !maxLength)
                {
                    messages::propertyMissing(
                        asyncResp->res, !minLength ? "MinLength" : "MaxLength");
                    return;
                }
                bounds.emplace_back(
                    "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MinStringLength",
                    *minLength, "");
                bounds.emplace_back(
                    "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MaxStringLength",
                    *maxLength, "");
            }
        }
        else if (type == "Integer")
        {
            if (!currentValueJson.is_number_integer())
            {
                messages::propertyValueTypeError(
                    asyncResp->res, currentValueJson, "CurrentValue");
                return;
            }
            currValue = currentValueJson.get<int64_t>();
            if (defaultIsNull)
            {
                defaultValue = std::string{};
            }
            else
            {
                if (!defaultValueJson.is_number_integer())
                {
                    messages::propertyValueTypeError(
                        asyncResp->res, defaultValueJson, "DefaultValue");
                    return;
                }
                defaultValue = defaultValueJson.get<int64_t>();
            }
            if (!lowerBound || !upperBound || !scalarIncrement)
            {
                messages::propertyMissing(
                    asyncResp->res,
                    !lowerBound
                        ? "LowerBound"
                        : (!upperBound ? "UpperBound" : "ScalarIncrement"));
                return;
            }
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.LowerBound",
                *lowerBound, "");
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.UpperBound",
                *upperBound, "");
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.ScalarIncrement",
                *scalarIncrement, "");
        }
        else if (type == "Boolean")
        {
            const bool* currBool = currentValueJson.get_ptr<const bool*>();
            if (currBool == nullptr)
            {
                messages::propertyValueTypeError(
                    asyncResp->res, currentValueJson, "CurrentValue");
                return;
            }
            // The backend stores Boolean values as int64
            currValue = static_cast<int64_t>(*currBool);
            if (defaultIsNull)
            {
                defaultValue = std::string{};
            }
            else
            {
                const bool* defBool = defaultValueJson.get_ptr<const bool*>();
                if (defBool == nullptr)
                {
                    messages::propertyValueTypeError(
                        asyncResp->res, defaultValueJson, "DefaultValue");
                    return;
                }
                defaultValue = static_cast<int64_t>(*defBool);
            }
        }
        else
        {
            messages::propertyValueIncorrect(asyncResp->res, "Type", type);
            return;
        }
        baseBiosTable.emplace(
            attr,
            std::make_tuple(getDbusBiosAttrType(type), readOnly, dispName,
                            descr, menuPath, currValue, defaultValue, bounds));
    }

    setDbusProperty(asyncResp, "Attributes", service,
                    sdbusplus::object_path(bios_utils::biosConfigManagerPath),
                    bios_utils::biosConfigManagerInterface, "BaseBIOSTable",
                    baseBiosTable);
}

/**
 * Handle PUT of the whole Bios resource: the host firmware publishes its
 * full BIOS attribute registry (names, types, bounds, current/default
 * values) via the Redfish Host Interface, which becomes the BaseBIOSTable.
 * The registry is owned by the host firmware, so only the Redfish Host
 * Interface bootstrap identity may publish it; other callers are rejected.
 */
inline void handleBiosServicePut(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (systemName != BMCWEB_REDFISH_SYSTEM_URI_NAME)
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }
    if (req.session == nullptr ||
        !bmcweb::isBootStrapAccount(req.session->username))
    {
        BMCWEB_LOG_ERROR("Bios PUT allowed only for bootstrap accounts");
        messages::insufficientPrivilege(asyncResp->res);
        return;
    }
    std::vector<nlohmann::json::object_t> baseBiosTableJson;
    if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "Attributes",
                                           baseBiosTableJson))
    {
        return;
    }
    bios_utils::getBIOSManagerObject(
        asyncResp,
        [asyncResp, baseBiosTableJson = std::move(baseBiosTableJson)](
            const std::string& service) {
            fillBiosTable(asyncResp, baseBiosTableJson, service);
        });
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::putBios)
        .methods(boost::beast::http::verb::put)(
            std::bind_front(handleBiosServicePut, std::ref(app)));
}

/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 *
 * Function handles POST method request.
 * Analyzes POST body message before sends Reset request data to D-Bus.
 */
inline void handleBiosResetPost(
    crow::App& app, const crow::Request& req,
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

    dbus::utility::async_method_call(
        asyncResp,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to reset bios: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
        },
        "xyz.openbmc_project.Software.Host.Updater0",
        "/xyz/openbmc_project/software",
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
