// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/assembly_utils.hpp"
#include "utils/asset_utils.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

/**
 * @brief Get Location code for the given assembly.
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] serviceName - Service in which the assembly is hosted.
 * @param[in] assembly - Assembly object.
 * @param[in] assemblyJsonPtr - json-keyname on the assembly list output.
 * @return None.
 */
inline void getAssemblyLocationCode(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const std::string& assembly,
    const nlohmann::json::json_pointer& assemblyJsonPtr)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, serviceName, assembly,
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, assembly, assemblyJsonPtr](
            const boost::system::error_code& ec, const std::string& value) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {} for assembly {}",
                                     ec.value(), assembly);
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            asyncResp->res.jsonValue[assemblyJsonPtr]["Location"]
                                    ["PartLocation"]["ServiceLabel"] = value;
        });
}

inline void getAssemblyState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const auto& serviceName, const auto& assembly,
    const nlohmann::json::json_pointer& assemblyJsonPtr)
{
    asyncResp->res.jsonValue[assemblyJsonPtr]["Status"]["State"] =
        resource::State::Enabled;

    dbus::utility::getProperty<bool>(
        serviceName, assembly, "xyz.openbmc_project.Inventory.Item", "Present",
        [asyncResp, assemblyJsonPtr,
         assembly](const boost::system::error_code& ec, const bool value) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (!value)
            {
                asyncResp->res.jsonValue[assemblyJsonPtr]["Status"]["State"] =
                    resource::State::Absent;
            }
        });
}

void getAssemblyHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const auto& serviceName, const auto& assembly,
                       const nlohmann::json::json_pointer& assemblyJsonPtr)
{
    asyncResp->res.jsonValue[assemblyJsonPtr]["Status"]["Health"] =
        resource::Health::OK;

    dbus::utility::getProperty<bool>(
        serviceName, assembly,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        [asyncResp, assemblyJsonPtr](const boost::system::error_code& ec,
                                     bool functional) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (!functional)
            {
                asyncResp->res.jsonValue[assemblyJsonPtr]["Status"]["Health"] =
                    resource::Health::Critical;
            }
        });
}

inline void afterGetDbusObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& assembly,
    const nlohmann::json::json_pointer& assemblyJsonPtr,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error : {} for assembly {}", ec.value(),
                         assembly);
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [serviceName, interfaceList] : object)
    {
        for (const auto& interface : interfaceList)
        {
            if (interface == "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                asset_utils::getAssetInfo(asyncResp, serviceName, assembly,
                                          assemblyJsonPtr, true, false);
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.LocationCode")
            {
                getAssemblyLocationCode(asyncResp, serviceName, assembly,
                                        assemblyJsonPtr);
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                getAssemblyState(asyncResp, serviceName, assembly,
                                 assemblyJsonPtr);
            }
            else if (interface ==
                     "xyz.openbmc_project.State.Decorator.OperationalStatus")
            {
                getAssemblyHealth(asyncResp, serviceName, assembly,
                                  assemblyJsonPtr);
            }
        }
    }
}

/**
 * @brief Get properties for the assemblies associated to given chassis
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisId - Chassis the assemblies are associated with.
 * @param[in] assemblies - list of all the assemblies associated with the
 * chassis.
 * @return None.
 */
inline void getAssemblyProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::vector<std::string>& assemblies)
{
    BMCWEB_LOG_DEBUG("Get properties for assembly associated");

    std::size_t assemblyIndex = 0;
    for (const std::string& assembly : assemblies)
    {
        nlohmann::json::object_t item;
        item["@odata.type"] = "#Assembly.v1_6_0.AssemblyData";
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Chassis/{}/Assembly#/Assemblies/{}", chassisId,
            std::to_string(assemblyIndex));
        item["MemberId"] = std::to_string(assemblyIndex);
        item["Name"] = sdbusplus::message::object_path(assembly).filename();

        asyncResp->res.jsonValue["Assemblies"].emplace_back(item);

        nlohmann::json::json_pointer assemblyJsonPtr(
            "/Assemblies/" + std::to_string(assemblyIndex));

        dbus::utility::getDbusObject(
            assembly, assemblyInterfaces,
            std::bind_front(afterGetDbusObject, asyncResp, assembly,
                            assemblyJsonPtr));

        getLocationIndicatorActive(
            asyncResp, assembly, [asyncResp, assemblyJsonPtr](bool asserted) {
                asyncResp->res
                    .jsonValue[assemblyJsonPtr]["LocationIndicatorActive"] =
                    asserted;
            });

        nlohmann::json& assemblyArray = asyncResp->res.jsonValue["Assemblies"];
        asyncResp->res.jsonValue["Assemblies@odata.count"] =
            assemblyArray.size();

        assemblyIndex++;
    }
}

inline void afterHandleChassisAssemblyGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID, const boost::system::error_code& ec,
    const std::vector<std::string>& assemblyList)
{
    if (ec)
    {
        BMCWEB_LOG_WARNING("Chassis {} not found, ec={}", chassisID, ec);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/Assembly/Assembly.json>; rel=describedby");

    asyncResp->res.jsonValue["@odata.type"] = "#Assembly.v1_6_0.Assembly";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Chassis/{}/Assembly", chassisID);
    asyncResp->res.jsonValue["Name"] = "Assembly Collection";
    asyncResp->res.jsonValue["Id"] = "Assembly";

    asyncResp->res.jsonValue["Assemblies"] = nlohmann::json::array();
    asyncResp->res.jsonValue["Assemblies@odata.count"] = 0;

    if (!assemblyList.empty())
    {
        getAssemblyProperties(asyncResp, chassisID, assemblyList);
    }
}

/**
 * @param[in] asyncResp - Shared pointer for asynchronous calls.
 * @param[in] chassisID - Chassis to which the assemblies are
 * associated.
 *
 * @return None.
 */
inline void handleChassisAssemblyGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG("Get chassis Assembly");
    assembly_utils::getChassisAssembly(
        asyncResp, chassisID,
        std::bind_front(afterHandleChassisAssemblyGet, asyncResp, chassisID));
}

inline void handleChassisAssemblyHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    assembly_utils::getChassisAssembly(
        asyncResp, chassisID,
        [asyncResp,
         chassisID](const boost::system::error_code& ec,
                    const std::vector<std::string>& /*assemblyList*/) {
            if (ec)
            {
                BMCWEB_LOG_WARNING("Chassis {} not found, ec={}", chassisID,
                                   ec);
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisID);
                return;
            }
            asyncResp->res.addHeader(
                boost::beast::http::field::link,
                "</redfish/v1/JsonSchemas/Assembly.json>; rel=describedby");
        });
}

inline void afterHandleChassisAssemblyPatch(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID,
    std::vector<nlohmann::json::object_t>& assemblyData,
    const boost::system::error_code& ec,
    const std::vector<std::string>& assemblyList)
{
    if (ec)
    {
        BMCWEB_LOG_WARNING("Chassis {} not found, ec={}", chassisID, ec);
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisID);
        return;
    }

    if (assemblyData.size() != assemblyList.size())
    {
        BMCWEB_LOG_WARNING(
            "The actual number of Assemblies is different from the number of the input Assemblies");
        messages::invalidIndex(asyncResp->res, assemblyList.size());
        return;
    }

    std::size_t assemblyIndex = 0;
    for (nlohmann::json::object_t& item : assemblyData)
    {
        std::optional<bool> locationIndicatorActive;
        if (json_util::readJsonObject(item, asyncResp->res,
                                      "LocationIndicatorActive",
                                      locationIndicatorActive))
        {
            if (locationIndicatorActive.has_value())
            {
                const auto& assembly = assemblyList[assemblyIndex];
                setLocationIndicatorActive(asyncResp, assembly,
                                           *locationIndicatorActive);
            }
        }
        assemblyIndex++;
    }
}

inline void handleChassisAssemblyPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisID)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::vector<nlohmann::json::object_t> assemblyData;
    if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "Assemblies",
                                           assemblyData))
    {
        return;
    }

    assembly_utils::getChassisAssembly(
        asyncResp, chassisID,
        std::bind_front(afterHandleChassisAssemblyPatch, asyncResp, chassisID,
                        assemblyData));
}

inline void requestRoutesAssembly(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::headAssembly)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleChassisAssemblyHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::getAssembly)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleChassisAssemblyGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Chassis/<str>/Assembly/")
        .privileges(redfish::privileges::patchAssembly)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleChassisAssemblyPatch, std::ref(app)));
}

} // namespace redfish
