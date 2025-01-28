// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "logging.hpp"
#include "sessions.hpp"
#include "http_privileges.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/vector.hpp>

#include <array>
#include <bitset>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

enum class PrivilegeType
{
    BASE,
    OEM
};

/** @brief A fixed array of compile time privileges  */
constexpr std::array<std::string_view, 5> basePrivileges{
    "Login", "ConfigureManager", "ConfigureComponents", "ConfigureSelf",
    "ConfigureUsers"};

constexpr const size_t basePrivilegeCount = basePrivileges.size();

/** @brief Max number of privileges per type  */
constexpr const size_t maxPrivilegeCount = 32;

/**
 * @brief A vector of all privilege names and their indexes
 * The privilege "OpenBMCHostConsole" is added to users who are members of the
 * "hostconsole" user group. This privilege is required to access the host
 * console.
 */
constexpr std::array<std::string_view, maxPrivilegeCount> privilegeNames{
    "Login",         "ConfigureManager", "ConfigureComponents",
    "ConfigureSelf", "ConfigureUsers",   "OpenBMCHostConsole"};

using bmcweb::Privileges;

} // namespace redfish
