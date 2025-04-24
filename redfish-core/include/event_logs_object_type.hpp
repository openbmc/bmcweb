// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2020 Intel Corporation

#pragma once
#include <string>
#include <vector>
namespace redfish
{
struct EventLogObjectsType
{
    std::string id;
    std::string timestamp;
    std::string messageId;
    std::vector<std::string> messageArgs;
};
} // namespace redfish
