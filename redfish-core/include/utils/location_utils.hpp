#pragma once

#include "dbus_singleton.hpp"
#include "error_messages.hpp"

#include <async_resp.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string_view>

namespace redfish
{
namespace location_util
{

/**
 * @brief Check if the interface is a supported connector
 *
 * @param[in]       interface   Location type interface.
 *
 * @return true if the interface is a supported connector
 */
inline bool isConnector(std::string_view interface)
{
    return interface == "xyz.openbmc_project.Inventory.Connector.Backplane" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Bay" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Connector" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Embedded" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Slot" ||
           interface == "xyz.openbmc_project.Inventory.Connector.Socket";
}

/**
 * @brief Fill out location info of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       interface   Location type interface.
 *
 * @return location if interface is supported Connector, otherwise, return
 * std::nullopt.
 */
inline std::optional<std::string> getLocationType(std::string_view interface)
{
    if (interface == "xyz.openbmc_project.Inventory.Connector.Backplane")
    {
        return "Backplane";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Bay")
    {
        return "Bay";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Connector")
    {
        return "Connector";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Embedded")
    {
        return "Embedded";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Slot")
    {
        return "Slot";
    }
    if (interface == "xyz.openbmc_project.Inventory.Connector.Socket")
    {
        return "Socket";
    }

    return std::nullopt;
}

/**
 * @brief Fill out location code of a resource by requesting data from the
 * given D-Bus object.
 *
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       service     D-Bus service to query.
 * @param[in]       objPath     D-Bus object to query.
 * @param[in]       location    Json path of where to find the location
 *                                property.
 */
inline void getLocationCode(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            std::string_view service, std::string_view objPath,
                            const nlohmann::json::json_pointer& location)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, service.data(), objPath.data(),
        "xyz.openbmc_project.Inventory.Decorator.LocationCode", "LocationCode",
        [asyncResp, location](const boost::system::error_code ec,
                              const std::string& property) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error for Location";
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue[location]["PartLocation"]["ServiceLabel"] =
                property;
        });
}

} // namespace location_util
} // namespace redfish
