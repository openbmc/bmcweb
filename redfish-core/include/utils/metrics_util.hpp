// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cmath>
#include <functional>
#include <memory>
#include <string>

namespace redfish
{
namespace metrics_util
{

// Stores a double-valued xyz.openbmc_project.Metric.Value "Value" at jsonPtr.
// A non-finite reading is reported as null; a D-Bus error leaves the property
// absent so a missing metric does not fail the enclosing resource.
inline void populateMetricsPropertyDouble(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const nlohmann::json::json_pointer& jsonPtr,
    const boost::system::error_code& ec, double value)
{
    if (ec)
    {
        BMCWEB_LOG_DEBUG(
            "DBus response error on GetProperty {} for property {}", ec,
            jsonPtr.to_string());
        return;
    }

    if (!std::isfinite(value))
    {
        asyncResp->res.jsonValue[jsonPtr] = nullptr;
        return;
    }
    asyncResp->res.jsonValue[jsonPtr] = value;
}

// Reads the double Value property of a Metric.Value object and populates
// jsonPtr with metrics-aware error handling.
inline void getMetricPropertyDouble(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& serviceName, const sdbusplus::object_path& objectPath,
    const nlohmann::json::json_pointer& jsonPtr)
{
    dbus::utility::getProperty<double>(
        serviceName, objectPath, "xyz.openbmc_project.Metric.Value", "Value",
        std::bind_front(populateMetricsPropertyDouble, asyncResp, jsonPtr));
}

} // namespace metrics_util
} // namespace redfish
