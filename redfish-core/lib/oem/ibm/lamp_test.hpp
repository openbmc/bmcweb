#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <array>
#include <memory>
#include <string_view>

namespace redfish
{

/**
 * @brief Retrieves lamp test state.
 *
 * @param[in] asyncResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void
    getLampTestState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("Get lamp test state");

    std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Led.Group"};
    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/led/groups/lamp_test", interfaces,
        [asyncResp](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetObject& object) {
        if (ec || object.empty())
        {
            if (ec.value() == boost::system::errc::io_error)
            {
                BMCWEB_LOG_DEBUG("lamp test not available yet!!");
                return;
            }
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *crow::connections::systemBus, object.begin()->first,
            "/xyz/openbmc_project/led/groups/lamp_test",
            "xyz.openbmc_project.Led.Group", "Asserted",
            [asyncResp](const boost::system::error_code& ec1, bool assert) {
            if (ec1)
            {
                if (ec1.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec1.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            asyncResp->res.jsonValue["Oem"]["IBM"]["@odata.type"] =
                "#OemComputerSystem.v1_0_0.IBM";
            asyncResp->res.jsonValue["Oem"]["IBM"]["LampTest"] = assert;
        });
    });
}

/**
 * @brief Sets lamp test state.
 *
 * @param[in] asyncResp   Shared pointer for generating response message.
 * @param[in] state   Lamp test state from request.
 *
 * @return None.
 */
inline void
    setLampTestState(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const bool state)
{
    BMCWEB_LOG_DEBUG("Set lamp test status.");

    std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Led.Group"};
    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/led/groups/lamp_test", interfaces,
        [asyncResp, state](const boost::system::error_code& ec,
                           const dbus::utility::MapperGetObject& object) {
        if (ec || object.empty())
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::asio::setProperty(
            *crow::connections::systemBus, object.begin()->first,
            "/xyz/openbmc_project/led/groups/lamp_test",
            "xyz.openbmc_project.Led.Group", "Asserted", state,
            [asyncResp, state](const boost::system::error_code& ec1) {
            if (ec1)
            {
                if (ec1.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error: {}", ec1.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            crow::connections::systemBus->async_method_call(
                [asyncResp](const boost::system::error_code& ec2) {
                if (ec2)
                {
                    BMCWEB_LOG_ERROR(
                        "Panel Lamp test failed with error code : {}",
                        ec2.value());
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
                "com.ibm.PanelApp", "/com/ibm/panel_app", "com.ibm.panel",
                "TriggerPanelLampTest", state);
        });
    });
}

} // namespace redfish
