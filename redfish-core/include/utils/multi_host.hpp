// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"

#include <boost/url/format.hpp>
#include <boost/url/url.hpp>

#include <memory>
#include <string>

namespace redfish
{

void getManagedHostProperty(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName, const std::string& systemPath,
    const std::string& serviceName,
    std::function<void(const uint64_t computerSystemIndex)> callback);

void afterGetComputerSystemSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>& callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree);

/**
 * @brief Retrieve the index associated with the requested system based on the
 * ManagedHost interface
 *
 * @param[i] asyncResp  Async response object
 * @param[i] systemName The requested system
 * @param[i] callback   Callback to call once the index has been found
 *
 * @return None
 */
void getComputerSystemIndex(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemName,
    std::function<void(const uint64_t computerSystemIndex)>&& callback);

sdbusplus::message::object_path getHostStateObjectPath(
    uint64_t computerSystemIndex);

std::string getHostStateServiceName(uint64_t computerSystemIndex);

sdbusplus::message::object_path getChassisStateObjectPath(
    uint64_t computerSystemIndex);

std::string getChassisStateServiceName(uint64_t computerSystemIndex);

} // namespace redfish
