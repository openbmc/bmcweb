// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/sw_utils.hpp"

#include <boost/beast/http/verb.hpp>

#include <format>
#include <functional>
#include <memory>
#include <string>

namespace redfish
{
using BaseBIOSTable = std::map<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string, bool>,
        std::variant<int64_t, std::string, bool>,
        std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                               std::string>>>>;

enum BaseBiosTableIndex
{
    baseBiosAttrType = 0,
    baseBiosReadonlyStatus,
    baseBiosDisplayName,
    baseBiosDescription,
    baseBiosMenuPath,
    baseBiosCurrValue,
    baseBiosDefaultValue,
    baseBiosBoundValues
};

static std::string getBiosAttrType(const std::string& attrType)
{
    std::string type;
    if (attrType ==
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration")
    {
        type = "Enumeration";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String")
    {
        type = "String";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Password")
    {
        type = "Password";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer")
    {
        type = "Integer";
    }
    else if (attrType ==
             "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Boolean")
    {
        type = "Boolean";
    }
    else
    {
        type = "UNKNOWN";
    }
    return type;
}

static void getAttributes(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& biosService)
{
    dbus::utility::getProperty<BaseBIOSTable>(
        biosService, "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable",
        [asyncResp](const boost::system::error_code& ec,
                    const BaseBIOSTable& baseBiosTable) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("D-Bus Property Get error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            if (baseBiosTable.empty())
            {
                BMCWEB_LOG_ERROR("No BIOS attributes found");
                return;
            }
            nlohmann::json& attributesJson =
                asyncResp->res.jsonValue["Attributes"];
            for (const auto& attrIt : baseBiosTable)
            {
                const std::string& attr = attrIt.first;
                std::string attrType = getBiosAttrType(
                    std::string(std::get<BaseBiosTableIndex::baseBiosAttrType>(
                        attrIt.second)));
                if ((attrType == "String") || (attrType == "Enumeration"))
                {
                    const std::string* attrCurrValue = std::get_if<std::string>(
                        &std::get<BaseBiosTableIndex::baseBiosCurrValue>(
                            attrIt.second));
                    if (attrCurrValue != nullptr)
                    {
                        attributesJson.emplace(attr, *attrCurrValue);
                    }
                    else
                    {
                        attributesJson.emplace(attr, std::string(""));
                    }
                }
                else if ((attrType == "Integer") || (attrType == "Boolean"))
                {
                    const int64_t* attrCurrValue = std::get_if<int64_t>(
                        &std::get<BaseBiosTableIndex::baseBiosCurrValue>(
                            attrIt.second));
                    if (attrCurrValue != nullptr)
                    {
                        if (attrType == "Boolean")
                        {
                            if (*attrCurrValue)
                            {
                                attributesJson.emplace(attr, true);
                            }
                            else
                            {
                                attributesJson.emplace(attr, false);
                            }
                        }
                        else
                        {
                            attributesJson.emplace(attr, *attrCurrValue);
                        }
                    }
                    else
                    {
                        if (attrType == "Boolean")
                        {
                            attributesJson.emplace(attr, false);
                        }
                        else
                        {
                            attributesJson.emplace(attr, 0);
                        }
                    }
                }
                else
                {
                    BMCWEB_LOG_ERROR("Attribute type not supported");
                }
            }
        });
}

static void getBiosAttributes(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/bios_config/manager",
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.BIOSConfig.Manager"},
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetObject& objInfo) {
            if (ec || objInfo.empty())
            {
                BMCWEB_LOG_ERROR("Failed to get bios attributes: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string& biosService = objInfo.front().first;
            getAttributes(asyncResp, biosService);
        });
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
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"]["target"] =
        std::format("/redfish/v1/Systems/{}/Bios/Actions/Bios.ResetBios",
                    BMCWEB_REDFISH_SYSTEM_URI_NAME);
    asyncResp->res.jsonValue["Attributes"] = nlohmann::json({});
    getBiosAttributes(asyncResp);
    // Get the ActiveSoftwareImage and SoftwareImages
    sw_util::populateSoftwareInformation(asyncResp, sw_util::biosPurpose, "",
                                         true);
}

inline void requestRoutesBiosService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/Bios/")
        .privileges(redfish::privileges::getBios)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleBiosServiceGet, std::ref(app)));
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
