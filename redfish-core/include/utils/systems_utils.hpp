#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "str_utility.hpp"

#include <array>
#include <string_view>

namespace redfish
{

namespace systems_utils
{

inline void getManagedHostProperty(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const boost::system::error_code& ec,
    const std::string& systemId, const std::string& serviceName,
    std::function<void(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const size_t computerSystemIndex)>& callback)
{
    if (ec)
    {
        if (ec.value() == boost::system::errc::io_error)
        {
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        BMCWEB_LOG_ERROR("DBus method call failed with error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }
    if (systemId.empty() || serviceName.empty())
    {
        BMCWEB_LOG_WARNING("System not found");
        messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                   systemName);
        return;
    }

    sdbusplus::asio::getProperty<uint64_t>(
        *crow::connections::systemBus, serviceName, systemId,
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost", "HostIndex",
        [req, asyncResp{asyncResp}, systemName,
         callback](const boost::system::error_code& ec2,
                   const size_t computerSystemIndex) {
        if (ec2)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec2);
            messages::resourceNotFound(asyncResp->res, "ComputerSystem",
                                       systemName);
            return;
        }

        BMCWEB_LOG_DEBUG("Found host index {}", computerSystemIndex);
        callback(req, asyncResp, systemName, computerSystemIndex);
    });
}

inline void afterGetValidSystemPaths(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const size_t computerSystemIndex)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    std::string systemId;
    std::string serviceName;
    if (ec)
    {
        getManagedHostProperty(req, asyncResp, systemName, ec, systemId,
                               serviceName, callback);
        return;
    }

    for (const auto& [path, serviceMap] : subtree)
    {
        systemId = sdbusplus::message::object_path(path).filename();
        if (path.find(systemName) != std::string::npos)
        {
            std::vector<std::string> tmp;
            bmcweb::split(tmp, systemId, '/');
            if (systemName == tmp.back())
            {
                serviceName = serviceMap.begin()->first;
                systemId = path;
                break;
            }
        }
    }
    getManagedHostProperty(req, asyncResp, systemName, ec, systemId,
                           serviceName, callback);
}

inline void getValidSystemPaths(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const size_t computerSystemIndex)>&& callback)
{
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Inventory.Decorator.ManagedHost"};
    dbus::utility::getSubTree("/xyz/openbmc_project/inventory", 0, interfaces,
                              std::bind_front(afterGetValidSystemPaths, req,
                                              asyncResp, systemName,
                                              std::move(callback)));
}

/**
 * @brief Wrapper - Retrieve the HostIndex from ManagedHost dbus interface and
 * pass it to the callback function.
 *
 * @param[in] asyncResp Shared pointer for completing asynchronous calls
 * @param[in] systemName Name of the requested System to base the dbus search on
 * @param[in] callback Function to call with the found computerSystemIndex
 *
 * @return None.
 */

inline void getComputerSystemIndex(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& systemName,
                       const size_t computerSystemIndex)>&& callback)
{
    getValidSystemPaths(req, asyncResp, systemName, std::move(callback));
}

} // namespace systems_utils
} // namespace redfish
