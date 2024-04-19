#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <array>
#include <string_view>
#include <variant>

namespace redfish
{

static constexpr const char* usbCodeUpdateObjectPath =
    "/xyz/openbmc_project/control/service/_70hosphor_2dusb_2dcode_2dupdate";
static constexpr const char* usbCodeUpdateInterface =
    "xyz.openbmc_project.Control.Service.Attributes";

constexpr std::array<std::string_view, 1> usbCodeUpdateInterfaces = {
    usbCodeUpdateInterface};

/**
 * @brief Retrieves BMC USB code update state.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getUSBCodeUpdateState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get USB code update state");
    dbus::utility::getDbusObject(
        usbCodeUpdateObjectPath, usbCodeUpdateInterfaces,
        [asyncResp](const boost::system::error_code& ec1,
                    const dbus::utility::MapperGetObject& object) {
        if (ec1 || object.empty())
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec1);
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, object.begin()->first,
            usbCodeUpdateObjectPath, usbCodeUpdateInterface, "Enabled",
            [asyncResp](const boost::system::error_code& ec2,
                        bool usbCodeUpdateState) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("DBUS response error: {}", ec2);
                messages::internalError(asyncResp->res);
                return;
            }

            asyncResp->res.jsonValue["Oem"]["IBM"]["@odata.type"] =
                "#OemManager.IBM";
            asyncResp->res.jsonValue["Oem"]["IBM"]["@odata.id"] =
                "/redfish/v1/Managers/bmc#/Oem/IBM";
            asyncResp->res.jsonValue["Oem"]["IBM"]["USBCodeUpdateEnabled"] =
                usbCodeUpdateState;
        });
    });
}

/**
 * @brief Sets BMC USB code update state.
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] state       USB code update state from request.
 *
 * @return None.
 */
inline void
    setUSBCodeUpdateState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const bool& state)
{
    BMCWEB_LOG_DEBUG("Set USB code update status.");
    dbus::utility::getDbusObject(
        usbCodeUpdateObjectPath, usbCodeUpdateInterfaces,
        [asyncResp, state](const boost::system::error_code& ec1,
                           const dbus::utility::MapperGetObject& object) {
        if (ec1 || object.empty())
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec1);
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, object.begin()->first,
            usbCodeUpdateObjectPath, usbCodeUpdateInterface, "Enabled", state,
            [asyncResp](const boost::system::error_code& ec2) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("Can't set USB code update status. Error: {}",
                                 ec2);
                messages::internalError(asyncResp->res);
                return;
            }
            messages::success(asyncResp->res);
        });
    });
}
} // namespace redfish
