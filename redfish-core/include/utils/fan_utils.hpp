#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"

#include <array>
#include <string_view>

namespace redfish
{
namespace fan_utils
{
template <typename Callback>
inline void
    getSensorDbusObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& sensorPath, Callback&& callback)
{
    auto respHandler = [asyncResp, sensorPath,
                        callback{std::forward<Callback>(callback)}](
                           const boost::system::error_code& ec,
                           const dbus::utility::MapperGetObject& object) {
        if (ec || object.empty())
        {
            BMCWEB_LOG_ERROR("D-Bus response error for getDbusObject {}",
                             ec.value());
            messages::internalError(asyncResp->res);
            return;
        }

        callback(object.begin()->first, sensorPath);
    };

    constexpr std::array<std::string_view, 1> intefaces{
        "xyz.openbmc_project.Sensor.Value"};
    dbus::utility::getDbusObject(sensorPath, intefaces, std::move(respHandler));
}

template <typename Callback>
inline void
    getFanSensorsObject(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& fanPath, Callback&& callback)
{
    auto respHandler = [asyncResp, callback{std::forward<Callback>(callback)}](
                           const boost::system::error_code& ec,
                           const dbus::utility::MapperEndPoints& sensorPaths) {
        if (ec)
        {
            if (ec.value() != EBADR)
            {
                BMCWEB_LOG_ERROR(
                    "D-Bus response error for getAssociationEndPoints {}",
                    ec.value());
                messages::internalError(asyncResp->res);
            }
            return;
        }

        if (sensorPaths.empty())
        {
            callback("", "");
            return;
        }

        for (const auto& sensorPath : sensorPaths)
        {
            getSensorDbusObject(asyncResp, sensorPath, callback);
        }
    };

    dbus::utility::getAssociationEndPoints(fanPath + "/sensors",
                                           std::move(respHandler));
}

} // namespace fan_utils
} // namespace redfish
