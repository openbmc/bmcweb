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
#include "utils/bios_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <sys/types.h>

#include <boost/container/flat_map.hpp>

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <format>
#include <functional>
#include <map>
#include <memory>
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

inline bool isValidAttrJson(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const nlohmann::json& attrJson)
{
    constexpr std::array<std::string_view, 5> stringRequired{
        "AttributeName", "DisplayName", "Description", "MenuPath", "Type"};
    constexpr std::array<std::string_view, 3> integerAddition{
        "LowerBound", "UpperBound", "ScalarIncrement"};
    constexpr std::array<std::string_view, 2> stringAddition{"MinLength",
                                                             "MaxLength"};

    for (std::string_view key : stringRequired)
    {
        if (!attrJson.contains(key) || !attrJson[key].is_string())
        {
            messages::propertyMissing(asyncResp->res, key);
            return false;
        }
    }
    if (!attrJson.contains("ReadOnly") || !attrJson["ReadOnly"].is_boolean())
    {
        messages::propertyMissing(asyncResp->res, "ReadOnly");
        return false;
    }
    for (std::string_view key : {std::string_view("CurrentValue"),
                                 std::string_view("DefaultValue")})
    {
        if (!attrJson.contains(key))
        {
            messages::propertyMissing(asyncResp->res, key);
            return false;
        }
        const nlohmann::json& value = attrJson[key];
        const nlohmann::json& type = attrJson["Type"];
        bool valid =
            ((type == "Enumeration" || type == "String") &&
             value.is_string()) ||
            (type == "Integer" && value.is_number()) ||
            (type == "Boolean" && value.is_boolean()) ||
            (key == "DefaultValue" && value.is_null());
        if (!valid)
        {
            messages::propertyValueTypeError(asyncResp->res, value, key);
            return false;
        }
    }
    if (attrJson["Type"] == "Integer")
    {
        for (std::string_view key : integerAddition)
        {
            if (!attrJson.contains(key) || !attrJson[key].is_number())
            {
                messages::propertyMissing(asyncResp->res, key);
                return false;
            }
        }
    }
    if (attrJson["Type"] == "String")
    {
        for (std::string_view key : stringAddition)
        {
            if (!attrJson.contains(key) || !attrJson[key].is_number())
            {
                messages::propertyMissing(asyncResp->res, key);
                return false;
            }
        }
    }
    if (attrJson["Type"] == "Enumeration" &&
        (!attrJson.contains("Values") || !attrJson["Values"].is_array()))
    {
        messages::propertyMissing(asyncResp->res, "Values");
        return false;
    }
    return true;
}

inline void fillBiosTable(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<nlohmann::json::object_t>& baseBiosTableJson,
    const std::string& service)
{
    BiosBaseTableSet baseBiosTable;
    for (const nlohmann::json::object_t& attrMap : baseBiosTableJson)
    {
        const nlohmann::json attrJson(attrMap);
        if (!isValidAttrJson(asyncResp, attrJson))
        {
            return;
        }
        std::string attr = attrJson["AttributeName"].get<std::string>();
        std::string dispName = attrJson["DisplayName"].get<std::string>();
        std::string descr = attrJson["Description"].get<std::string>();
        std::string menuPath = attrJson["MenuPath"].get<std::string>();
        std::string type = attrJson["Type"].get<std::string>();
        bool readOnly = attrJson["ReadOnly"].get<bool>();
        std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                               std::string>>
            bounds;
        std::variant<int64_t, std::string, bool> currValue;
        std::variant<int64_t, std::string, bool> defaultValue;
        // A null DefaultValue is stored with a mismatched variant type to
        // mark it absent, matching the existing platform convention.
        const bool defaultIsNull = attrJson["DefaultValue"].is_null();

        if (type == "Enumeration" || type == "String")
        {
            currValue = attrJson["CurrentValue"].get<std::string>();
            defaultValue = defaultIsNull
                               ? std::variant<int64_t, std::string, bool>(
                                     int64_t{0})
                               : std::variant<int64_t, std::string, bool>(
                                     attrJson["DefaultValue"]
                                         .get<std::string>());
            if (type == "Enumeration")
            {
                for (const auto& value :
                     attrJson["Values"].get<std::vector<std::string>>())
                {
                    bounds.emplace_back(
                        "xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf",
                        value, "");
                }
            }
            else
            {
                bounds.emplace_back(
                    "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MinStringLength",
                    attrJson["MinLength"].get<int64_t>(), "");
                bounds.emplace_back(
                    "xyz.openbmc_project.BIOSConfig.Manager.BoundType.MaxStringLength",
                    attrJson["MaxLength"].get<int64_t>(), "");
            }
        }
        else if (type == "Integer")
        {
            currValue = attrJson["CurrentValue"].get<int64_t>();
            defaultValue =
                defaultIsNull
                    ? std::variant<int64_t, std::string, bool>(std::string{})
                    : std::variant<int64_t, std::string, bool>(
                          attrJson["DefaultValue"].get<int64_t>());
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.LowerBound",
                attrJson["LowerBound"].get<int64_t>(), "");
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.UpperBound",
                attrJson["UpperBound"].get<int64_t>(), "");
            bounds.emplace_back(
                "xyz.openbmc_project.BIOSConfig.Manager.BoundType.ScalarIncrement",
                attrJson["ScalarIncrement"].get<int64_t>(), "");
        }
        else if (type == "Boolean")
        {
            // The backend stores Boolean values as int64
            currValue =
                static_cast<int64_t>(attrJson["CurrentValue"].get<bool>());
            defaultValue =
                defaultIsNull
                    ? std::variant<int64_t, std::string, bool>(std::string{})
                    : std::variant<int64_t, std::string, bool>(
                          static_cast<int64_t>(
                              attrJson["DefaultValue"].get<bool>()));
        }
        else
        {
            messages::propertyValueIncorrect(asyncResp->res, "Type", type);
            return;
        }
        baseBiosTable.emplace(
            attr, std::make_tuple(getDbusBiosAttrType(type), readOnly,
                                  dispName, descr, menuPath, currValue,
                                  defaultValue, bounds));
    }

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, service,
        std::string(bios_utils::biosConfigManagerPath),
        std::string(bios_utils::biosConfigManagerInterface), "BaseBIOSTable",
        baseBiosTable, [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Error setting BaseBIOSTable: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });
}

/**
 * Handle PUT of the whole Bios resource: the host firmware publishes its
 * full BIOS attribute registry (names, types, bounds, current/default
 * values) via the Redfish Host Interface, which becomes the
 * BaseBIOSTable.
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
    std::vector<nlohmann::json::object_t> baseBiosTableJson;
    if (!redfish::json_util::readJsonAction(req, asyncResp->res, "Attributes",
                                            baseBiosTableJson))
    {
        messages::unrecognizedRequestBody(asyncResp->res);
        return;
    }
    bios_utils::getBIOSManagerObject(
        asyncResp, [asyncResp, baseBiosTableJson](const std::string& service) {
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
