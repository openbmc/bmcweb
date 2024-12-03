#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

namespace redfish
{
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
// Timer for PCIe Topology Refresh
static std::unique_ptr<boost::asio::steady_timer> pcieTopologyRefreshTimer;
static uint countPCIeTopologyRefresh = 0;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * @brief PCIe Topology Refresh monitor. which block incoming request
 *
 * @param[in] ec          error code to be handled
 * @param[in] timer       pointer to steady timer which block call
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] countPtr   how many time this method get called.
 *
 * @return None.
 */
static void pcieTopologyRefreshWatchdog(
    const boost::system::error_code& ec, boost::asio::steady_timer* timer,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, uint* countPtr)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("steady_timer error {}", ec.value());
        messages::internalError(asyncResp->res);
        pcieTopologyRefreshTimer = nullptr;
        (*countPtr) = 0;
        return;
    }
    //  This method can block incoming requests max for 8 seconds. So, each
    //  call to this method adds a 1-second block, and the maximum call allow is
    //  8 times which makes it a total of ~8 seconds
    if ((*countPtr) >= 8)
    {
        messages::internalError(asyncResp->res);
        pcieTopologyRefreshTimer = nullptr;
        (*countPtr) = 0;
        return;
    }
    ++(*countPtr);
    sdbusplus::asio::getProperty<bool>(
        *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
        "/xyz/openbmc_project/pldm", "com.ibm.PLDM.PCIeTopology",
        "PCIeTopologyRefresh",
        [asyncResp, timer, countPtr](const boost::system::error_code& ec1,
                                     const bool pcieRefreshValue) {
            if (ec1)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}", ec1.value());
                messages::internalError(asyncResp->res);
                pcieTopologyRefreshTimer = nullptr;
                (*countPtr) = 0;
                return;
            }
            // After PCIe Topology Refresh, it sets the pcieRefreshValuePtr
            // value to false. if a value is not false, extend the time, and if
            // it is false, delete the timer and reset the counter
            if (pcieRefreshValue)
            {
                BMCWEB_LOG_INFO("pcieRefreshValuePtr time extended");
                timer->expires_at(
                    timer->expiry() + boost::asio::chrono::seconds(1));
                timer->async_wait([timer, asyncResp, countPtr](
                                      const boost::system::error_code& ec2) {
                    pcieTopologyRefreshWatchdog(ec2, timer, asyncResp,
                                                countPtr);
                });
            }
            else
            {
                BMCWEB_LOG_ERROR("pcieRefreshValuePtr value refreshed");
                pcieTopologyRefreshTimer = nullptr;
                (*countPtr) = 0;
                return;
            }
        });
};

/**
 * @brief Sets PCIe Topology Refresh state.
 *
 * @param[in] req - The request data
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] state   PCIe Topology Refresh state from request.
 *
 * @return None.
 */
inline void setPCIeTopologyRefresh(
    const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const bool state)
{
    BMCWEB_LOG_DEBUG("Set PCIe Topology Refresh status.");
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
        "/xyz/openbmc_project/pldm", "com.ibm.PLDM.PCIeTopology",
        "PCIeTopologyRefresh", state,
        [&req, asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("PCIe Topology Refresh failed.{}", ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
            countPCIeTopologyRefresh = 0;
            pcieTopologyRefreshTimer =
                std::make_unique<boost::asio::steady_timer>(*req.ioService);
            pcieTopologyRefreshTimer->expires_after(std::chrono::seconds(1));
            pcieTopologyRefreshTimer->async_wait(
                [timer = pcieTopologyRefreshTimer.get(),
                 asyncResp](const boost::system::error_code& ec1) {
                    pcieTopologyRefreshWatchdog(ec1, timer, asyncResp,
                                                &countPCIeTopologyRefresh);
                });
        });
}

/**
 * @brief Sets Save PCIe Topology Info state.
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] state   Save PCIe Topology Info state from request.
 *
 * @return None.
 */
inline void setSavePCIeTopologyInfo(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const bool state)
{
    BMCWEB_LOG_DEBUG("Set Save PCIe Topology Info status.");

    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, "xyz.openbmc_project.PLDM",
        "/xyz/openbmc_project/pldm", "com.ibm.PLDM.PCIeTopology",
        "SavePCIeTopologyInfo", state,
        [asyncResp](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Save PCIe Topology Info failed.{}",
                                 ec.value());
                messages::internalError(asyncResp->res);
                return;
            }
        });
}

} // namespace redfish
