// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "dbus_utility.hpp"
#include "generated/enums/settings.hpp"
#include "logging.hpp"

#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{
namespace settings_util
{

constexpr std::string_view applyTimeInterface =
    "xyz.openbmc_project.Control.Decorator.ApplyTime";

struct OnResetControl
{
    std::string service;
    std::string path;
};

using OnResetControlCallback = std::function<void(
    const boost::system::error_code&, const std::optional<OnResetControl>&)>;

inline void afterGetApplyTime(
    const OnResetControl& control, const OnResetControlCallback& callback,
    const boost::system::error_code& ec, const std::string& applyTime)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error getting ApplyTime of {}: {}", control.path,
                         ec);
        callback(ec, std::nullopt);
        return;
    }

    if (applyTime !=
        "xyz.openbmc_project.Control.Decorator.ApplyTime.ApplyTimes.OnReset")
    {
        callback({}, std::nullopt);
        return;
    }
    callback({}, control);
}

inline void afterGetControlSubtree(
    const OnResetControlCallback& callback, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() == EBADR || ec == boost::system::errc::io_error)
        {
            callback({}, std::nullopt);
            return;
        }
        BMCWEB_LOG_ERROR("DBus error getting control object: {}", ec);
        callback(ec, std::nullopt);
        return;
    }

    if (subtree.empty())
    {
        callback({}, std::nullopt);
        return;
    }
    if (subtree.size() > 1 || subtree.front().second.size() != 1)
    {
        BMCWEB_LOG_ERROR("More than one control object or service found");
        callback(boost::system::errc::make_error_code(
                     boost::system::errc::invalid_argument),
                 std::nullopt);
        return;
    }

    const auto& [service, interfaces] = subtree.front().second.front();
    // A control without the decorator does not report its apply time.
    if (std::ranges::find(interfaces, applyTimeInterface) == interfaces.end())
    {
        callback({}, std::nullopt);
        return;
    }

    OnResetControl control{service, subtree.front().first};
    dbus::utility::getProperty<std::string>(
        control.service, control.path, std::string(applyTimeInterface),
        "ApplyTime", std::bind_front(afterGetApplyTime, control, callback));
}

// Finds the control object that implements controlInterface for the D-Bus
// object at resourcePath, and hands it to the callback only when values
// written to it are applied on reset.
inline void getOnResetControl(const std::string& resourcePath,
                              std::string_view controlInterface,
                              OnResetControlCallback&& callback)
{
    std::array<std::string_view, 1> interfaces{controlInterface};
    dbus::utility::getAssociatedSubTree(
        sdbusplus::object_path(resourcePath) / "controlled_by",
        sdbusplus::object_path("/xyz/openbmc_project/control"), 0, interfaces,
        std::bind_front(afterGetControlSubtree, std::move(callback)));
}

inline void addSettingsAnnotation(nlohmann::json& resource,
                                  const boost::urls::url& settingsUri)
{
    nlohmann::json& annotation = resource["@Redfish.Settings"];
    annotation["@odata.type"] = "#Settings.v1_3_3.Settings";
    annotation["SettingsObject"]["@odata.id"] = settingsUri;
    annotation["SupportedApplyTimes"] =
        nlohmann::json::array({settings::ApplyTime::OnReset});
}

} // namespace settings_util
} // namespace redfish
